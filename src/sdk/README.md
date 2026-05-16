# GpgFrontend Module SDK — API Reference

Public C headers that modules link against. For the developer guide, build
setup, and code examples see [`modules/README.md`](../../modules/README.md).

---

## `GFSDKBasic.h` — Core utilities

**Types** (`GFSDKBasicModel.h`): `GFCommandExecuteCallback`,
`GFCommandExecuteContext`, `GFTranslatorDataReader`, `kGfStrlenMax`.

| Function | Description |
|---|---|
| `GFAllocateMemory(size)` | Allocate from the module allocator. |
| `GFReallocateMemory(ptr, size)` | Resize a module-allocated block. |
| `GFFreeMemory(ptr)` | Free a module-allocated block. |
| `GFSecAllocateMemory(size)` | Allocate secure memory (zeroed on free). |
| `GFSecReallocateMemory(ptr, size)` | Resize a secure block. |
| `GFSecFreeMemory(ptr)` | Free a secure block. |
| `GFModuleStrDup(src)` | Duplicate a string into module memory; caller frees. |
| `GFModuleSecStrDup(src)` | Duplicate a string into secure memory; caller frees. |
| `GFProjectVersion()` | Application version string (e.g. `"2.1.0"`). |
| `GFProjectGitCommitHash()` | Abbreviated git commit hash of the build. |
| `GFQtEnvVersion()` | Qt version the application was built against. |
| `GFAppActiveLocale()` | Active locale name (e.g. `"en_US"`). |
| `GFAppRegisterTranslatorReader(id, reader)` | Register a translation data callback. |
| `GFExecuteCommandSync(cmd, argc, argv, cb, data)` | Run a command synchronously; invoke callback with result. |
| `GFExecuteCommandBatchSync(contexts, count)` | Run multiple commands concurrently; wait for all. |
| `GFCacheSave(key, value)` | Save to the session cache. |
| `GFCacheSaveWithTTL(key, value, ttl)` | Save to the session cache with expiry (seconds). |
| `GFCacheGet(key)` | Read from the session cache. |
| `GFDurableCacheSave(key, json)` | Save a JSON value to the persistent cache. |
| `GFDurableCacheGet(key)` | Read a JSON value from the persistent cache. |
| `GFIsFlatpakENV()` | `true` when running inside a Flatpak sandbox. |

**Ownership rule:** a `char*` parameter takes ownership (pass `DUP(...)`); a
`char*` return value is caller-owned (free with `GFFreeMemory`); a `const char*`
return value is host-managed (do not free).

---

## `GFSDKLog.h` — Logging

Emits into the Qt logging pipeline under the `module` category.

| Function | Level |
|---|---|
| `GFModuleLogTrace(msg)` | Trace |
| `GFModuleLogDebug(msg)` | Debug |
| `GFModuleLogInfo(msg)` | Info |
| `GFModuleLogWarn(msg)` | Warning |
| `GFModuleLogError(msg)` | Error |

---

## `GFSDKModule.h` — Events and runtime values

**Types** (`GFSDKModuleModel.h`): `GFModuleMetaData`, `GFModuleEventParam`,
`GFModuleEvent`, and the ten `GFModuleAPI*` function-pointer types that define
the module ABI.

| Function | Description |
|---|---|
| `GFModuleListenEvent(module_id, event_id)` | Subscribe the module to an event (upper-case ID). |
| `GFModuleUpsertRTValue(ns, key, value)` | Write a string runtime value (keys stored lower-case). |
| `GFModuleUpsertRTValueBool(ns, key, value)` | Write a boolean runtime value. |
| `GFModuleRetrieveRTValueOrDefault(ns, key, default)` | Read a string runtime value. |
| `GFModuleRetrieveRTValueOrDefaultBool(ns, key, default)` | Read a boolean runtime value. |
| `GFModuleListRTChildKeys(ns, key, out)` | List child keys under a prefix; caller frees the array. |
| `GFModuleTriggerModuleEventCallback(event, module_id, argv)` | Invoke the next handler in the event callback chain. |

---

## `GFSDKGpg.h` — GPG operations

All functions take a `channel` integer. Use `GFGpgCurrentGpgContextChannel()`
to obtain it from the main window.

**Result structs:** `GFGpgSignResult`, `GFGpgEncryptionResult`,
`GFGpgDecryptResult`, `GFGpgVerifyResult`, `GFGpgKeyUID`.
Free GPGME handles with `GFGpgFreeResult` before freeing the enclosing struct.

| Function | Description |
|---|---|
| `GFGpgSignData(channel, key_ids, count, data, mode, ascii, result)` | Sign data. Returns 0 on success. |
| `GFGpgEncryptData(channel, key_ids, count, data, ascii, result)` | Encrypt for recipients. Returns 0 on success. |
| `GFGpgDecryptData(channel, data, result)` | Decrypt data. Returns 0 on success. |
| `GFGpgVerifyData(channel, data, signature, result)` | Verify a signature. Returns 0 on success. |
| `GFGpgPublicKey(channel, key_id, ascii)` | Export a public key block; caller frees. |
| `GFGpgKeyPrimaryUID(channel, key_id, uid)` | Get the primary UID; caller frees struct. |
| `GFGpgImportKeys(channel, parent, data, size)` | Import keys (shows the UI import dialog). |
| `GFGpgExportKey(channel, key_id, ascii, data, size)` | Export a key to a caller-owned buffer. |
| `GFGpgCurrentGpgContextChannel()` | Active GPG channel from the main window; -1 if unavailable. |
| `GFGpgFreeResult(r)` | Decrement the ref-count of a GPGME result handle. |
| `GFAnalyseEncryptResult(channel, err, result, report)` | Produce a human-readable encryption result report. |
| `GFAnalyseSignResult(channel, err, result, report)` | Produce a signing result report. |
| `GFAnalyseDecryptResult(channel, err, result, report)` | Produce a decryption result report. |
| `GFAnalyseVerifyResult(channel, err, result, report)` | Produce a verification result report. |

---

## `GFSDKUI.h` — UI integration

**Types** (`GFSDKUIModel.h`): `QObjectFactory`, `MetaData`.

| Function | Description |
|---|---|
| `GFUICreateGUIObject(factory, data)` | Create a QObject on the main thread; blocks until done. |
| `GFUIGetGUIObject(id)` | Look up a registered GUI object by string ID. |
| `GFUIShowDialog(dialog, parent)` | Show a QDialog on the main thread (non-blocking). |
| `GFUIGlobalSettings()` | Pointer to the application-wide QSettings; do not delete. |
| `GFUIRegisterFileExtensionHandleEvent(ext, prefix)` | Map a file extension to an event prefix. |

---

## `GFSDKExtra.h` — Utilities

| Function | Description |
|---|---|
| `GFCompareSoftwareVersion(current, latest)` | Compare two version strings; returns negative / 0 / positive. |
| `GFHttpRequestUserAgent()` | HTTP User-Agent string; do not free. |

---

## `GFSDKBuildInfo.h` — Build constants

`GF_SDK_VERSION_STR` — full version string (e.g. `"2.1.0"`), injected by CMake
at configure time. Used by `GF_MODULE_API_DEFINE_V2` to record which SDK
version the module was compiled against.
