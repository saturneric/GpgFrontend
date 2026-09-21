# GpgFrontend Module SDK — API Reference

Public C headers that define the module ABI. A module does not normally
include these directly or call their functions by name — it includes
`GFModule.h` from `src/module_runtime/include/`, which wraps this layer in
C++ types and RAII helpers. This file documents what actually crosses the
boundary; for the module-author workflow (writing, packaging, and
distributing a module) see [`modules/README.md`](../../modules/README.md).

## `GFSDKModuleApi.h` — the module ABI

The whole ABI is one bootstrap symbol (`GFModuleGetApi`, written by every
module — see `modules/README.md`) plus two tables, each starting with a
`struct_size` field written by whichever side compiled it: `GFHostApi` (what
the host gives the module — buffers, logging, GPG, UI, in groups a
capability check can gate) and `GFModuleApi` (what the module gives back —
its verified identity plus lifecycle/event hooks). Growth is append-only on
both tables: an older module handed a newer, larger `GFHostApi` still works,
because it only reads the prefix it was compiled against.

## `GFSDKBasic.h` — Core utilities

**Types** (`GFSDKBasicModel.h`): `GFCommandExecuteCallback`,
`GFCommandExecuteContext`, `GFTranslatorDataReader`, `kGfStrlenMax`. Also
declares the session/durable cache functions (`GFCacheSave`,
`GFCacheSaveWithTTL`, `GFCacheGet`, `GFDurableCacheSave`,
`GFDurableCacheGet`) and their secure (wiping-allocator) counterparts.

**Ownership rule:** an SDK function argument is borrowed — pass a pointer
straight through and keep owning it. An SDK return value is owned by the
caller — reclaim a `char*` with `GFFreeMemory`, or with `GFModule.h`'s `UDUP`
if you are inside a module. The one exception is a `char*` field inside a
struct a module hands over *whole* (`GFModuleEvent`, `GFModuleEventParam`,
`GFCommandExecuteContext`), which the module must allocate with
`GFModuleStrDup` — ownership of the whole struct is what transfers.

## `GFSDKLog.h` — Logging

Emits into the Qt logging pipeline under the `module` category. Modules call
this through `GFModule.h`'s `LOG_INFO`/`FLOG_INFO`/etc. rather than directly.

## `GFSDKModule.h` — Events and runtime values

Runtime key-value store (`GFModuleUpsertRTValue`,
`GFModuleRetrieveRTValueOrDefault`, and the `*Bool` and
`GFModuleListRTChildKeys` variants) and the low-level event dispatch entry
point (`GFModuleTriggerModuleEventCallback`). **Types** (`GFSDKModuleModel.h`):
`GFModuleEventParam`, `GFModuleEvent`. A module's own event subscription is
declared statically (see `GFEventBinding` in `modules/README.md`), not
through a call into this header.

## `GFSDKGpg.h` / `GFSDKGpgResult.h` — GPG operations

All functions take a `channel` integer; use `GFGpgCurrentGpgContextChannel()`
to obtain it from the main window.

`GFGpgSign`, `GFGpgEncrypt`, `GFGpgDecrypt`, and `GFGpgVerify`
(`GFSDKGpgResult.h`) write into an opaque `GFGpgResultRef`, released with
`GFGpgResultRelease`; `GFModule.h`'s `GFGpgResult` C++ wrapper does this for
you via RAII. Accessors (`GFGpgResultStatusOf`, `GFGpgResultError`,
`GFGpgResultData`/`GFGpgResultTakeData`, `GFGpgResultCapsuleId`,
`GFGpgResultErrorString`) are all borrowed and valid only until release.
Payloads are `GFBufferView`/`GFBufferRef` (`GFSDKBuffer.h`), not raw
`char*`, since a signature or ciphertext is exact octets rather than text.

`GFAnalyse{Encrypt,Sign,Decrypt,Verify}ResultByCapsule` and their `*InfoByCapsule`
counterparts recover the same structured result the host's own crypto dialogs
show (recipients, signatures, validity) from a `capsule_id`, engine-neutrally
— they work whether the active engine is GnuPG or rPGP, which a raw gpgme
handle would not.

Key lookup and metadata: `GFGpgPublicKey`, `GFGpgKeyPrimaryUID`,
`GFGpgImportKeys`, `GFGpgExportKey`. Searching keys or inspecting an
encrypted message's recipients returns a list handle from `GFSDKGpgList.h`
(`GFGpgFindKeys` → `GFGpgKeyBriefListRef`, `GFGpgSniffRecipients` →
`GFGpgRecipientListRef`) — see that section below.

## `GFSDKPgp.h` — Structure inspection

Reads OpenPGP data as a packet structure without a keyring, an engine, or a
channel — deliberately separate from `GFSDKGpg.h`, which addresses a keyring
through a channel and nothing here does. Backs the `m_pgp_inspect` module.

## `GFSDKGpgList.h` — Opaque result collections

Opaque, distinctly-typed handles for collections the GPG calls return, each
with one release function and per-index accessors, replacing a family of
per-type `Free*` array walkers that all did the same thing:

- `GFGpgFindKeys(channel, email, &list)` → `GFGpgKeyBriefListRef`, sized with
  `GFGpgKeyBriefListCount` and read per-index with
  `GFGpgKeyBriefFingerprint`/`KeyId`/`Uid`/`Usability`/`CanSign`/`CanEncrypt`/
  etc., released with `GFGpgKeyBriefListRelease`.
- `GFGpgSniffRecipients(channel, in, &list)` → `GFGpgRecipientListRef`, sized
  with `GFGpgRecipientListCount` and read per-index with
  `GFGpgRecipientKeyId`/`Fingerprint`/`Uid`/`HasSecret`/`Hidden`/etc.,
  released with `GFGpgRecipientListRelease`. `HasSecret` is the field that
  answers "can this message be opened here" — a public key alone cannot
  decrypt. Reads only the PKESK packets, so nothing is decrypted and no
  passphrase is requested.
- `GFGpgListAddresses(channel, secret_only, &list)` → `GFStringListRef`,
  sized with `GFStringListCount` and read per-index with `GFStringListAt`,
  released with `GFStringListRelease`.

## `GFSDKModuleAttribution.h` — Handle bookkeeping

Internal to the SDK: records which module a given handle was issued to, so
that unloading a module can reclaim anything it leaked rather than leaving a
dangling allocation. Not something a module calls directly.

## `GFSDKExtra.h` — Small standalone helpers

`GFCompareSoftwareVersion(current, latest)` — semantic-version comparison,
used by the update-check flow.

## `GFSDKUI.h` — Host UI

**Types** (`GFSDKUIModel.h`): `QObjectFactory`.

Widgets may only be created and shown on the main thread; `GFUICreateGUIObject`
and `GFUIShowDialog` dispatch there for you. `GFUIGetGUIObject` resolves the
opaque handles the host passes in event parameters; `GFModule.h`'s
`GFEvent::RequireGui<T>()` does that resolution and produces the right
`GFEventResult` failure in one call.

`GFUIRegisterSettingsPage(page_id, section_id, title, keywords, factory, data)`
adds a module-owned page to the Settings dialog; `GFUIUnregisterSettingsPage`
removes it, and must be called from the module's `on_deactivate` hook so the
registry never holds a factory belonging to an unloaded module. The dialog is
rebuilt on every open, so `factory` is called once per dialog and must return
a fresh, unparented widget. `title` and `keywords` are untranslated `GC_TR(...)`
source strings, which the host translates in the `GTrC` context while it
builds the dialog, so a language change is picked up without re-registering.
`section_id` is one of `application`, `keys_engines`, `features`, `system`;
anything else becomes its own section after those.

`GFUIGlobalSettings()` exposes the shared `QSettings` instance for reading
and writing persistent, module-namespaced settings.

## `GFSDKBuildInfo.h` — Build constants

`GF_SDK_VERSION_STR` — full version string (e.g. `"2.1.0"`), injected by CMake
at configure time.

## Packaging and trust

The SDK is only the runtime ABI. What ships is a signed `.gfmodule` package —
a manifest, its Ed25519 signature, and (for an external module) the build key
that signature was made with — verified against the compiled-in trust root
before any module code runs. None of that is an SDK header; it lives in
[`src/core/module/`](../core/module) and is described from the module
author's side in [`modules/README.md`](../../modules/README.md#packaging-signing--distribution).
