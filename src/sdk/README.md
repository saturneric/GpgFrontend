# GpgFrontend Module SDK — API Reference

Public C headers that modules link against. For the developer guide, build
setup, and code examples see [`modules/README.md`](../../modules/README.md).

## `GFSDKBasic.h` — Core utilities

**Types** (`GFSDKBasicModel.h`): `GFCommandExecuteCallback`,
`GFCommandExecuteContext`, `GFTranslatorDataReader`, `kGfStrlenMax`.

**Ownership rule:** a `char*` parameter takes ownership (pass `DUP(...)`); a
`char*` return value is caller-owned (free with `GFFreeMemory`); a `const char*`
return value is host-managed (do not free).

## `GFSDKLog.h` — Logging

Emits into the Qt logging pipeline under the `module` category.

## `GFSDKModule.h` — Events and runtime values

**Types** (`GFSDKModuleModel.h`): `GFModuleMetaData`, `GFModuleEventParam`,
`GFModuleEvent`, and the ten `GFModuleAPI*` function-pointer types that define
the module ABI.

## `GFSDKGpg.h` — GPG operations

All functions take a `channel` integer. Use `GFGpgCurrentGpgContextChannel()`
to obtain it from the main window.

**Result structs:** `GFGpgSignResult`, `GFGpgEncryptionResult`,
`GFGpgDecryptResult`, `GFGpgVerifyResult`, `GFGpgKeyUID`.
Free GPGME handles with `GFGpgFreeResult` before freeing the enclosing struct.

## `GFSDKUI.h` — Host UI

**Types** (`GFSDKUIModel.h`): `QObjectFactory`.

Widgets may only be created and shown on the main thread; `GFUICreateGUIObject`
and `GFUIShowDialog` dispatch there for you. `GFUIGetGUIObject` resolves the
opaque handles the host passes in event parameters.

`GFUIRegisterSettingsPage(page_id, section_id, title, keywords, factory, data)`
adds a module-owned page to the Settings dialog; `GFUIUnregisterSettingsPage`
removes it, and must be called from `GFDeactivateModule()` so the registry never
holds a factory belonging to an unloaded module. The dialog is rebuilt on every
open, so `factory` is called once per dialog and must return a fresh, unparented
widget. `title` and `keywords` are untranslated `GC_TR(...)` source strings, which
the host translates in the `GTrC` context while it builds the dialog, so a
language change is picked up without re-registering. `section_id` is one of
`application`, `keys_engines`, `features`, `system`; anything else becomes its own
section after those.

## `GFSDKBuildInfo.h` — Build constants

`GF_SDK_VERSION_STR` — full version string (e.g. `"2.1.0"`), injected by CMake
at configure time. Used by `GF_MODULE_API_DEFINE_V2` to record which SDK
version the module was compiled against.
