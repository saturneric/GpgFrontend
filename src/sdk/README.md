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

## `GFSDKBuildInfo.h` — Build constants

`GF_SDK_VERSION_STR` — full version string (e.g. `"2.1.0"`), injected by CMake
at configure time. Used by `GF_MODULE_API_DEFINE_V2` to record which SDK
version the module was compiled against.
