# GpgFrontend Module SDK: API Reference

## The three layers

```text
module code
    -> public SDK          src/sdk/*.h, implemented in src/sdk/api  (gf_sdk)
      -> GFHostApi         src/sdk/GFSDKHostApi.h, the primitive boundary
        -> host internals  src/sdk/host  (gf_host_api) -> gf_core, gf_ui
```

and never `public SDK -> core/ or ui/`, and never a global that the SDK reads
to find out who is calling.

Stated as one sentence: **the runtime owns state, the SDK is stateless, and
`GFHostApi` is the only path to the host.**

## `GFSDKContext.h`: what every call carries

Every public function that needs the host takes a `GFSDKContext*` as its
**first** argument. Every public function that does not need the host takes
none. There is no third case.

```c
typedef struct GFSDKContext {
  size_t struct_size;
  uint32_t abi_version;
  uint32_t reserved;
  const GFHostApi* host;   /* the only member anything is decided from */
  const char* module_id;   /* diagnostics only, never an authorization input */
} GFSDKContext;
```

`gf_sdk` holds **no** state: no bound table, no current module, no
thread-local, not even a flag recording that a capability denial has already
been reported. That last one is the visible cost of the rule, and it is worth
it: a module does most of its work on threads it started itself, and an SDK
that had to look up "who is calling" would be right inside an event handler
and wrong in the worker that handler spawned.

`gf_module_runtime` builds one context per module, in `activate()`, before
any hook runs, and owns it. The storage is never freed; it is abandoned at
teardown, because a module may still be running code on a thread it failed to
stop, and freeing it would turn a refusable call into a read of freed memory.
Both `gf_sdk` and `gf_module_runtime` are static archives linked into each
module; neither is shared with the host.

The authorization token is read from `host->context` at the point of call, so
a context cannot hold one module's token beside another's table.

## `GFSDKHostApi.h`: the primitive boundary

`GFHostApi` is minted per module from its signed manifest. A capability group
the module did not declare is NULL in its table, and its primitives refuse the
context even if reached another way.

| group       | granted by             | contents                                                                                      |
| ----------- | ---------------------- | --------------------------------------------------------------------------------------------- |
| `buffer`    | always                 | buffers, and both memory arenas behind one `arena` argument                                   |
| `log`       | always                 | `write(severity, …)`, `enabled`                                                               |
| `app`       | always                 | version, commit, Qt version, user agent, locale, flatpak, key protection                      |
| `event`     | always                 | `subscribe`, `answer`                                                                         |
| `bootstrap` | always                 | translator registration: runtime plumbing, not a permission                                   |
| `list`      | always                 | the generic string list, which `gpg` and `storage` both produce                               |
| `gpg`       | `"gpg"`                | operations, results, keys, key and recipient lists, analysis                                  |
| `pgp`       | `"pgp"`                | packet-structure inspection; no keyring, no engine                                            |
| `ui`        | `"ui"`                 | theme colors by role, the user file path                                                      |
| `command`   | `"ui"`                 | register, invoke, cancel and describe typed commands                                          |
| `script`    | `"ui"`                 | load the module's embedded UI scripts                                                         |
| `native`    | `"ui"` + `"ui.custom"` | register native widgets; a mounted widget's notifications                                     |
| `editor`    | `"editor"`             | the current document's exact octets, and what it is (never its content)                       |
| `storage`   | `"storage"`            | the module's settings group, the three caches (scoped per module), the runtime register table |
| `process`   | `"process"`            | running an external program                                                                   |

`event` is always present because it answers a different question from the
rest: the groups say what a module may actively DO, while a subscription says
what it may OBSERVE, and that is governed by the signed manifest's event
allowlist, enforced host-side in `ModuleManager::ListenEvent`. `subscribe`
answers at once, and the runtime refuses to activate a module whose declared
subscription the host refused.

Each listener an event is delivered to owes it exactly one `answer`; the host
refuses an answer from a module the event was not delivered to, and a second
one. A listener deactivated before it answers is answered for, with
`ret` = -1, and an event nobody is listening to is answered the same way, so
whoever triggered it is never left waiting. The event is forgotten once every
answer is in.

**`network` is not in this table.** A module opens a socket through Qt, so
there is nothing for the host to mediate. It remains a legal, signed,
user-visible manifest declaration and is reported separately from the grant
mask, so the mask never claims a permission it cannot enforce.

### What the token is, and is not

`GFHostContextRef` is not an unforgeable capability. The host accepts a token
only if it finds that exact pointer in its registry of minted contexts, and
then authorizes according to the record it found. An invented or stale value
is refused without ever being dereferenced.

It does not stop a hostile module. Native code in the same process can read
another module's live context pointer and call with it, and the host treats
that call as the other module's. What the token guarantees is that an honest
module's calls are attributed and limited correctly on every thread,
including threads it started itself.

Handles are private to the module that asked for them: a buffer, result or
list presented by a different module is refused and logged.

### Lifetime

Each call that passes the gate is counted until it returns. At shutdown the
host revokes every module's grant, waits for calls already inside the host to
finish, and only then sweeps the handles the module still holds. A module
whose calls do not finish in time keeps its handles, which leak at exit
rather than being freed under a running call.

Minting a module that is still live with the same grant returns the table it
already holds. Any other mint (a reactivation, or a different grant) creates a
new context and revokes the old one. A table already handed out is never
rewritten, because other threads read it without a lock.

The grant lives exactly while the module is active. It is revoked when the
module is deactivated (after its `on_deactivate` hook), when its activation
fails, and at shutdown (after its `on_unload` hook, which still logs through
it). A thread the module failed to stop is refused from then on -- until the
module is next activated, since all of a module's threads share its one
context.

### Storage

The cache calls take octets and return octets; an embedded NUL survives, and
nothing is parsed as JSON. Keys are private to the calling module and to the
store: `GF_STORE_DURABLE` and `GF_STORE_SECURE_DURABLE` never see each other's
values, and neither do two modules. An empty value is the same as no value, and
`cache_get` reports an absent key as negative.

Values written before keys were scoped (under the old shared `__module_<key>`
name) move to the key of the first module that reads them. The old key named
no owner, so this is one-shot, not a scoping rule.

The register table (`GFStorageState*`, `gf::sdk::StateText` and friends) is a
different store: session-only, and shared. Anyone may read any namespace; a
module writes only its own, named by its id. Namespaces and keys are
lower-case. Lua's `state.*` is not this -- it is the module's persisted
settings group, the same one `gf::sdk::Setting` reaches.

## UI integration

A module has exactly two ways to put something on the screen, and neither
hands it a Host Qt object:

1. **A UI script and commands.** The module's Lua script says _where_ its
   commands are offered (a menu, the editor's context menu, the key details
   dialog) and _when_ they are enabled. The commands themselves are typed C++,
   and they carry every user-visible word.
2. **Module-owned native widgets** (`ui.custom`). The module builds a
   `QWidget`; the script mounts it into a container the Host owns: a dialog, a
   Settings page, or the view of a document tab.

The rule of thumb: **Lua coordinates UI-facing behaviour; C++ implements
substantial operations.** Lua is Turing-complete, but the Host authority it
can reach is small, typed, semantic, capability-controlled and tied to the
module's lifecycle.

| capability  | grants                                                                                                             |
| ----------- | ------------------------------------------------------------------------------------------------------------------ |
| `ui`        | the module's Lua state: `commands`, `ui.action`, `ui.subscribe`, `state`, `theme`; C++ command register and invoke |
| `ui.custom` | adds `native.*` and `ui.mount` in Lua, and native widget registration in C++. Requires `ui`                        |

There is no third level and no Host-handle escape hatch. A grant check needs
**every** capability it names: `native` is NULL in a table that has `ui` but
not `ui.custom`.

### Commands

A command is a type: an id, its presentation, and the shapes of its arguments
and result.

```cpp
struct PublishKey {
  static constexpr gf::cmd::Meta kMeta{
      GF_MODULE_ID ".publish_key", GC_TR("Publish Public Key"),
      GC_TR("Upload this key to the key server"),
      GC_TR("Key Server Operations"), GF_HOST_CAP_GPG, gf::cmd::kLongRunning};
  struct Args {
    gf::cmd::KeyRef key;
    static constexpr auto Fields() {
      return std::make_tuple(gf::cmd::F("key", &Args::key));
    }
  };
  using Result = gf::cmd::Unit;
};

auto DoPublish(const gf::cmd::CommandContext& ctx, const PublishKey::Args& a)
    -> gf::cmd::Outcome<gf::cmd::Unit>;

const std::array<gf::cmd::Binding, 1> kCommands = {
    gf::cmd::Bind<PublishKey, &DoPublish>()};  // listed in GFModuleHooks
```

- Ids are namespaced: a module's command is its id, a dot, and one
  lower-case name without dots, and its signed manifest lists exactly the ones
  it binds (`commands` in `module.json`). The runtime checks both directions
  at activation.
- Arguments and results travel as CBOR, described once by `Describe<C>()`.
  That one descriptor is what the registry checks, what Lua converts through,
  and what these docs describe. Bulk bytes are a `gf::cmd::Blob`, carried
  beside the CBOR (`{"$blob": n}`) and handed across by reattribution, never
  copied into the payload.
- `CommandRegistry` is the **only** authority. It checks, in order: the id
  exists, `kHostOnly`, the caller holds the command's capabilities, the
  namespace and allowlist, the command's state, and the thread. A Lua script,
  a C++ module and the Host's own menus all go through the same `Invoke`.
- `Commands().Invoke<C>(args)` fires and forgets; the overload with a
  receiver and a callback delivers the result on the receiver's thread.
  `Cancel(call)` guarantees the callback does not run after it returns --
  including a result already queued to the receiver -- and so does the
  module's deactivation.
- A deactivated module is a closed caller: from its withdrawal until its next
  activation the registry refuses its invocations and registrations, whatever
  of it is still running.

Host commands (types in `GFSDKHostCommands.hpp`, titles in `src/ui`):

| id                                                                                      | arguments                                           |
| --------------------------------------------------------------------------------------- | --------------------------------------------------- |
| `org.gpgfrontend.document.new`                                                          | `type, title`                                       |
| `org.gpgfrontend.document.open`                                                         | `type, title, path, content: Blob, saved, modified` |
| `org.gpgfrontend.document.{save, save_as, close}`                                       | `target: DocumentRef`; needs `editor`               |
| `org.gpgfrontend.crypto.{encrypt, decrypt, sign, verify, encrypt_sign, decrypt_verify}` | `target: DocumentRef`                               |
| `org.gpgfrontend.keys.import`                                                           | `data: Blob`                                        |
| `org.gpgfrontend.keys.open_manager`                                                     |                                                     |
| `org.gpgfrontend.view.open`                                                             | `view: ViewRef`, the caller's own mount only        |
| `org.gpgfrontend.app.open_settings`                                                     |                                                     |
| `org.gpgfrontend.app.message`                                                           | `severity, title, text`                             |

### The UI script

Scripts are embedded at build time and signed with the module:

```cmake
gf_add_module(... LUA_SCRIPTS ui/main.lua)   # -> :/gf_module/<id>/lua/main.lua
```

The runtime loads them once, after `on_activate` and after the module's
commands are registered. A script that fails to load leaves nothing behind:
every action, subscription and mount it made is rolled back.

```lua
local publish = commands.get("com.example.module.publish_key")

ui.action {
  id = "publish",
  anchor = ui.anchor("key.details.actions"),
  command = publish,
  update = function(ctx)
    if not ctx.key then return { visible = false } end
    return { args = { key = ctx.key } }
  end,
}
```

**Lua carries no user-visible text.** Titles, descriptions and categories come
from the command's `Meta`; a mount's title, keywords, suffix and icon from the
native widget's metadata. Both are translated in C++.

**`update(ctx)` is pure.** It may read the context, command state and
`state.get`; `commands.invoke`, `state.set` and every `ui.*` registration
raise inside it. Its answer (`visible`, `enabled`, `checked`, `args`) is
validated, and an invalid one hides and disables the action. When the user
triggers the action, the Host builds a fresh context, runs `update` again,
requires it to be visible and enabled and the command to be enabled, and only
then invokes with the arguments from _that_ run. Stale arguments cannot reach a
command. `args` may be omitted and reads as an empty table.

**Continuations.** `commands.invoke(cmd, args, function(result, err) ... end)`
never blocks. The result is checked against the command's result schema
first; a mismatch arrives as `err == "bad_result"`. Blob fields arrive as
opaque `Blob` handles that cannot be read, only passed on. A module has at
most 32 continuations outstanding.

**Subscriptions** are a closed, semantic catalog, not the module event bus:
`document.activated`, `document.state_changed`, `document.saved`,
`key_database.refreshed`, `app.ui_ready`. Any other name fails the load.

**Handles** are typed, and each type has only its named members. `Context`,
`Document` and `Key` live for one call; keep one with `:ref()`, which the Host
resolves again on use and refuses once stale. A handle from another module's
state is refused.

<!-- lua-api-reference: generated by LuaApiReference(), pinned by GFUiLuaApiTest -->

```text
Lua UI API v1

commands.get(id) -> Command                       command must exist; caps checked
commands.state(Command) -> {enabled, visible, checked}
commands.invoke(Command, args[, fn(result, err)]) -> Call | nil, status
                                                  handlers and continuations only
Command:enabled() Command:visible() Command:checked() Command.id
Call:cancel() -> bool

ui.anchor(id) -> Anchor                           menu and button anchors
ui.anchor.settings{section} ui.anchor.editor{document_type, extensions}
ui.anchor.dialog{} -> Anchor                      mount anchors
ui.action{id, anchor, command, order?, icon?, update?}     while loading
  update(ctx) -> {visible?, enabled?, checked?, args?}      pure
ui.mount{id, anchor, widget, order?} -> Mount     ui.custom; while loading
ui.subscribe{event, handler, id?}                 while loading
  events: document.activated document.state_changed document.saved
          key_database.refreshed app.ui_ready
native.widget(name) native.factory(name)          ui.custom only

state.get(key[, default]) state.set(key, value)   storage; module's own group
state.host(key)                                   storage; shared Host keys only
theme.color(role) -> 0xAARRGGBB

handle lifetimes
  Command Anchor Mount NativeWidget NativeFactory  until the module unloads
  Context Document Key                             one call only
  DocumentRef KeyRef                               until the module unloads
  Call                                             until it completes or is cancelled
  Blob                                             until used as an argument
Context: .document .key :has_selection()
Document: .type .modified :has_openpgp() :ref()
Key: .fingerprint .key_id .has_secret .channel :ref()
```

### Anchors

An anchor is a stable, Host-owned place a script can contribute to. Lua never
resolves one to a Qt object. Entries are ordered by `order`, then module id,
then action id; a duplicate full id is a load error. A deprecated anchor keeps
working for at least one minor release and warns at load; removing one is an
SDK-major change. An unknown anchor fails the load and the message lists the
valid ids.

<!-- anchor-catalog: generated by AnchorCatalogReference(), pinned by GFUiLuaApiTest -->

```text
anchor catalog v1
main.menu.file.workspace | menu | context: document | modules: many | since 1
main.menu.advanced | menu | context: document | modules: many | since 1
main.menu.help | menu | context: document | modules: many | since 1
main.menu.import_key | menu | context: none | modules: many | since 1
editor.context | menu | context: document, selection | modules: many | since 1
key.details.actions | buttons | context: key | modules: many | since 1
settings | settings | context: none | modules: many | since 1
editor | editor | context: document | modules: one | since 1
dialog | dialog | context: none | modules: many | since 1
```

Settings sections: `application`, `keys_engines`, `features`, `system`.

### Sandbox and error model

One `lua_State` per module, on the GUI thread. Loaded: `base` (without
`load`, `loadfile`, `dofile`, `require`; `collectgarbage` answers `"count"`
only; `print` goes to the module log), `string` (without `dump`), `table`,
`math` (without `random`), `utf8`. Absent: `io`, `os`, `package`, `debug`,
`coroutine`, timers, threads, files, network and processes.

Limits: 8 MiB of memory per module; 200k instructions per `update`, handler
or continuation, 2M per script load. `pcall` cannot swallow an exhausted
budget.

The Host enters Lua only through `lua_pcall`, and every binding runs its C++
in a separate, non-raising frame (`LuaBinding<&Impl>`): **no Lua error ever
jumps over a C++ frame that owns an object.** A source test forbids raising
calls outside the two boundary files, and the Lua tests run under ASan
(`scripts/run_tests.sh --asan`).

### Native widgets

```cpp
class InspectorWidget : public QWidget, public gf::ui::DialogWidget { ... };

auto OnActivate() -> GFResult {
  gf::ui::RegisterNativeWidget<InspectorWidget>(
      "inspector", {GC_TR("OpenPGP Inspector")},
      [](const QCborMap&) { return new InspectorWidget(); });
  return GFResult::Ok();
}
```

Three typed protocols, and no generic request or query:

| class                    | the Host calls                                                                                                                                                     | the widget may ask                                  |
| ------------------------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------------ | --------------------------------------------------- |
| `gf::ui::DocumentWidget` | `Load`, `Save`, `IsDirty`, `CryptoOperations`, `ApplyVerification`, `AppendText`, `AttachPublicKey`, `ApplyFont`, `PrepareSave`, `SourceLockReason`, `WipeContent` | modified, show source, run a crypto op, ops changed |
| `gf::ui::SettingsWidget` | `LoadSettings`, `ApplySettings`                                                                                                                                    | restart needed                                      |
| `gf::ui::DialogWidget`   | `Opened`, `CloseRequested`                                                                                                                                         | close                                               |

`RegisterNativeWidget` makes one instance; `RegisterNativeWidgetFactory` one
per mount (every document tab needs its own). The widget pointer goes module
to Host only. Every notification names its instance, and the Host refuses one
from a module that does not own it or of the wrong kind.

A document view never receives the Host's editor. The raw source is the
page's own switcher, which the view may ask for; `SourceLockReason` keeps it
read-only while the content is protected (a signed message, say). An editor
mount's `extensions` makes the Host open those files as that document type,
and the Host saves them through `PrepareSave`.

### Queries

- **Settings.** `gf::sdk::Setting` / `SetSetting` / `RemoveSetting` (and Lua
  `state.*`) read and write the module's own group: `modules/<id>`, or the
  group a bundled module always used. `state.host` reads the few Host keys on
  an allowlist (`network/prohibit_update_check`).
- **Theme.** `GFUIThemeColorForRole(ctx, role)`; no widget involved.
- **Current document.** `GFEditorCurrentDocument`: id, type, title, path,
  modified. Never the content.

A module's `path` and `saved` for `document.open` are ignored: a module
cannot bind a tab to a file, so what it opens is a new, unsaved document and
saving it asks where. Titles are sanitised by the Host -- no path separators,
no control characters -- because a title becomes the suggested file name.

### Teardown

When a module deactivates, in this order:

1. Its entry gate closes: no Host call enters its code from here on, queued
   ones included.
2. The Host withdraws everything it holds for the module, at once: its
   commands (calls it was serving fail, calls it made are cancelled, and it
   becomes a closed caller), its native widget registrations, its
   translations. On the GUI thread, queued: its Lua runtime stops and is torn
   down, and every live instance of its widgets is dropped by its container
   -- a dialog closes, a settings page says the module is gone, and a
   document shows its own source, editable, keeping every byte.
3. The Host waits, bounded, for calls already inside the module.
4. The module's `on_deactivate` runs. It stops the module's own threads,
   timers and network requests; it has nothing to unregister.
5. Its grant is revoked, and every event it still owed an answer to is
   answered for, with a failure.

Doing it twice does nothing. SDK handles the module still holds are swept at
shutdown, not at deactivation.

### Removed

These slots keep their place in the table and refuse, logged once per module
with the replacement:

| slot                                              | now              | instead                                   |
| ------------------------------------------------- | ---------------- | ----------------------------------------- |
| `ui.create_object`, `ui.get_object`               | NULL             | commands; a native widget                 |
| `ui.show_dialog`                                  | -1               | a dialog mount and `view.open`            |
| `ui.theme_color(widget)`                          | 0                | `theme_color_role`                        |
| `ui.register_settings_page`                       | -1               | a settings mount                          |
| `ui.register_tab_view`, `register_file_extension` | -1               | an editor mount and its `extensions`      |
| `storage.settings_root`                           | NULL             | `setting_get/set/remove`                  |
| `gpg.import_keys(parent)`                         | `parent` ignored | `gf::sdk::ImportKeys(ctx, channel, data)` |

The events that lent a module Host widgets are gone, with their trigger sites:
`MAINWINDOW_MENU_MOUNTED`, `KEY_PAIR_OPERA_MENU_CREATED`,
`ABOUT_DIALOG_TABS_MOUNTED`, `NETWORK_SETTINGS_TAB_*`,
`EDIT_TAB_TYPE_*_OP_SAVE_FILE` and `FILE_EXT_*`. `APPLICATION_LOADED` and
`REQUEST_SEARCH_PUBLIC_KEY_BY_FINGERPRINT` no longer carry a handle. The
`EDIT_TAB_TYPE_<TYPE>_OP_*` family is the six crypto operations only.

## What is NOT on the boundary

The boundary carries primitives; the convenient spellings are built on top,
module-side, and cost no ABI:

- nineteen per-field list accessors became two borrowed row structs;
- nine `GFAnalyse*` entry points became one taking an operation;
- five theme-color functions became one taking a role;
- five log severities became one `write`;
- three cache tiers became a `store` argument;
- `GFUIHumanSize` and `GFCompareSoftwareVersion` are computed in the module,
  because neither needs anything from the host. Both take no context, which is
  what "pure" means here.

Adding a public SDK helper is a change to `src/sdk/api/` and `GFSDK.hpp`, not
to this ABI.

## `GFSDK.hpp`: the C++ conveniences

Header-only, stateless, and still context-explicit. The C ABI reports absence,
takes octets and returns owned handles because those are the honest shapes for
a boundary; these take and return `QString`, apply a fallback where a caller
wants one, and release what they borrowed.

```cpp
auto version = gf::sdk::StateText(ctx, "core", "gpgme.version", "0.0.0");
auto keys    = gf::sdk::ExportKey(ctx, channel, key_id, /*ascii=*/true);
gf::sdk::SetCacheText(ctx, GF_STORE_DURABLE, "last-check", when);
```

Secrets deliberately do **not** go through the QString forms: a QString cannot
be erased. `m_email`'s credential store uses the raw buffer API for that
reason, and says so at the call site.

## Growth

`GF_SDK_ABI_VERSION` is still 4. What this generation added was appended:
`GFHostApi.command`, `.script` and `.native`; `GFHostUiApi.theme_color_role`;
`GFHostEditorApi.current_document`; `GFHostStorageApi.setting_get/set/remove`;
`GFModuleBootstrapInfo.commands`; `GFModuleHooks.commands`. A read of an
appended member is guarded by the table's `struct_size` (the SDK's internal
`GF_SDK_GROUP_HAS`), so a module built against this SDK still runs on a host
that predates it, and is refused rather than crashing.

Every struct here begins with `struct_size`, written by whichever side
compiled it, and grows by APPENDING only. Reordering or repurposing a field
makes an already-built module read the wrong slot with no diagnostic anywhere.

There is no `GF_SDK_EXPORT`. The public SDK is not in a shared library any
more, so there is nothing to export: a module exports `GFModuleGetApi` and
nothing else, and the host component exports nothing at all.

## Packaging and trust

The SDK is only the runtime ABI. What ships is a signed `.gfmodule` package
(a manifest, its Ed25519 signature, and for an external module the publisher
key that signature was made with). An integrated package is verified against
the compiled-in trust root; an external one against the key it carries, and it
loads only once the user has trusted that publisher key and enabled the module.
Both checks happen before any module code runs. External packages are made by
`gf_module_externalize` from an already verified integrated package. None of that is an SDK header; it lives in
[`src/core/module/`](../core/module) and is described from the module author's
side in [`modules/README.md`](../../modules/README.md#packaging-signing--distribution).
