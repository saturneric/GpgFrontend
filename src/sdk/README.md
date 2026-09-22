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

| group | granted by | contents |
|---|---|---|
| `buffer` | always | buffers, and both memory arenas behind one `arena` argument |
| `log` | always | `write(severity, …)`, `enabled` |
| `app` | always | version, commit, Qt version, user agent, locale, flatpak, key protection |
| `event` | always | `subscribe`, `answer` |
| `bootstrap` | always | translator registration: runtime plumbing, not a permission |
| `list` | always | the generic string list, which `gpg` and `storage` both produce |
| `gpg` | `"gpg"` | operations, results, keys, key and recipient lists, analysis |
| `pgp` | `"pgp"` | packet-structure inspection; no keyring, no engine |
| `ui` | `"ui"` | widgets, dialogs, theme colors by role, extension registration |
| `editor` | `"editor"` | the current document's exact octets |
| `storage` | `"storage"` | settings, the three caches (scoped per module), the runtime register table |
| `process` | `"process"` | running an external program |

`event` is always present because it answers a different question from the
rest: the groups say what a module may actively DO, while a subscription says
what it may OBSERVE, and that is governed by the signed manifest's event
allowlist, enforced host-side in `GlobalModuleContext::ListenEvent`.

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
already holds. Any other mint (a reload, or a different grant) creates a new
context and revokes the old one, so code still holding the old table is
refused. A table already handed out is never rewritten, because other threads
read it without a lock.

The grant outlives deactivation, because a module's `on_unload` hook still
calls the SDK. It is released when the module fails to activate, when it is
unloaded, and at shutdown.

### Storage

The cache calls take octets and return octets; an embedded NUL survives, and
nothing is parsed as JSON. Keys are private to the calling module and to the
store: `GF_STORE_DURABLE` and `GF_STORE_SECURE_DURABLE` never see each other's
values, and neither do two modules. An empty value is the same as no value, and
`cache_get` reports an absent key as negative.

Values written before keys were scoped (under the old shared `__module_<key>`
name) move to the module's own key the first time that module reads them.

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

Every struct here begins with `struct_size`, written by whichever side
compiled it, and grows by APPENDING only. Reordering or repurposing a field
makes an already-built module read the wrong slot with no diagnostic anywhere.

There is no `GF_SDK_EXPORT`. The public SDK is not in a shared library any
more, so there is nothing to export: a module exports `GFModuleGetApi` and
nothing else, and the host component exports nothing at all.

## Packaging and trust

The SDK is only the runtime ABI. What ships is a signed `.gfmodule` package
(a manifest, its Ed25519 signature, and for an external module the build key
that signature was made with), verified against the compiled-in trust root
before any module code runs. None of that is an SDK header; it lives in
[`src/core/module/`](../core/module) and is described from the module author's
side in [`modules/README.md`](../../modules/README.md#packaging-signing--distribution).
