# GpgFrontend Module SDK — Developer Guide

This document explains how to write a GpgFrontend module using the C SDK
provided under `src/sdk/`. Modules are shared libraries loaded at runtime.
They can react to application events, call GPG operations, add UI widgets, and
store persistent data — all through the stable ABI defined by these headers.

---

## Table of Contents

1. [SDK Headers](#sdk-headers)
2. [Memory Management](#memory-management)
3. [Module Entry Points](#module-entry-points)
4. [Macro Helpers](#macro-helpers)
5. [Events](#events)
6. [Runtime Values](#runtime-values)
7. [Logging](#logging)
8. [Cache](#cache)
9. [GPG Operations](#gpg-operations)
10. [UI Integration](#ui-integration)
11. [Translations](#translations)
12. [Build Setup](#build-setup)
13. [Minimal Example](#minimal-example)

---

## SDK Headers

| Header               | Purpose                                                                      |
| -------------------- | ---------------------------------------------------------------------------- |
| `GFSDKBasic.h`       | Memory allocation, version queries, command execution, cache, locale         |
| `GFSDKBasicModel.h`  | Types shared by `GFSDKBasic.h` (callback types, context structs)             |
| `GFSDKLog.h`         | Logging at trace / debug / info / warn / error severity                      |
| `GFSDKModule.h`      | Event subscription, runtime key-value store, event callbacks                 |
| `GFSDKModuleModel.h` | Struct definitions for events, params, and module API function-pointer types |
| `GFSDKGpg.h`         | Sign, encrypt, decrypt, verify, key import/export                            |
| `GFSDKExtra.h`       | Version comparison, HTTP User-Agent                                          |
| `GFSDKUI.h`          | Create/show Qt widgets, access global settings, register file handlers       |
| `GFSDKUIModel.h`     | `QObjectFactory` and `MetaData` types used by the UI API                     |
| `GFSDKBuildInfo.h`   | `GF_SDK_VERSION_STR` constant injected at CMake configure time               |

Convenience module-side helpers live under `modules/include/`:

| Helper                    | Purpose                                                                                                |
| ------------------------- | ------------------------------------------------------------------------------------------------------ |
| `GFModuleDeclare.h`       | `GF_MODULE_API_DECLARE` — forward-declares all required entry points                                   |
| `GFModuleDefine.h`        | `GF_MODULE_API_DEFINE_V2` — implements the metadata entry points and wires up the event dispatch table |
| `GFModuleCommonUtils.hpp` | Inline utilities: `DUP`/`UDUP`, logging macros, event conversion, `CB_SUCC`/`CB_ERR`                   |

---

## Memory Management

All strings and buffers that cross the module boundary must go through the SDK
allocator. **Never** mix `new`/`delete` or `malloc`/`free` with SDK pointers.

### Normal memory

```c
void* GFAllocateMemory(uint32_t size);
void* GFReallocateMemory(void* ptr, uint32_t size);
void  GFFreeMemory(void* ptr);
```

### Secure memory (zeroed on free)

Use for passphrases or other sensitive data.

```c
void* GFSecAllocateMemory(uint32_t size);
void* GFSecReallocateMemory(void* ptr, uint32_t size);
void  GFSecFreeMemory(void* ptr);
```

### String duplication

The SDK passes strings as `const char*` (host-owned) or `char*`
(caller must free). Two helpers duplicate a string into SDK-allocated memory:

```c
char* GFModuleStrDup(const char* src);     // free with GFFreeMemory
char* GFModuleSecStrDup(const char* src);  // free with GFSecFreeMemory
```

In module code the shorthand macros from `GFModuleCommonUtils.hpp` are
preferred:

```cpp
DUP("hello")     // GFModuleStrDup — transfers ownership to the callee
SECDUP("secret") // GFModuleSecStrDup — same, secure allocator
UDUP(ptr)        // UnStrDup — converts char* back to QString and frees ptr
USECDUP(ptr)     // UnSecStrDup — same, frees with GFSecFreeMemory
QDUP(qstr)       // QStrDup — QString → char* (normal allocator)
QSECDUP(qstr)    // QSecStrDup — QString → char* (secure allocator)
```

### Ownership rules summary

- A function whose parameter is `char*` (not `const char*`) **takes ownership**
  and will free it. Pass `DUP(...)` at the call site.
- A function that returns `char*` gives ownership to the caller. Free it with
  `GFFreeMemory` (or `UDUP` to also convert to `QString`).
- A function that returns `const char*` gives a pointer that the caller must
  **not** free (host-managed lifetime).

---

## Module Entry Points

Every module must export the following C symbols. Use `GF_MODULE_API_DECLARE`
in the header and `GF_MODULE_API_DEFINE_V2` (or `GF_MODULE_API_DEFINE`) in the
source file to generate most of them automatically.

| Symbol                      | Called when                                                  |
| --------------------------- | ------------------------------------------------------------ |
| `GFGetModuleGFSDKVersion()` | Module is loaded — SDK version compatibility check           |
| `GFGetModuleQtEnvVersion()` | Module is loaded — Qt version compatibility check            |
| `GFGetModuleID()`           | Any time — returns the unique module ID                      |
| `GFGetModuleVersion()`      | Any time — returns the module version string                 |
| `GFGetModuleMetaData()`     | Any time — returns a linked list of metadata key/value pairs |
| `GFRegisterModule()`        | Once at load — register resources, translation readers       |
| `GFActiveModule()`          | After registration — subscribe to events with `LISTEN(...)`  |
| `GFExecuteModule(event)`    | Each time a subscribed event fires                           |
| `GFDeactivateModule()`      | Before unload — unsubscribe / stop background tasks          |
| `GFUnregisterModule()`      | After deactivation — free resources                          |

`GFExecuteModule` receives a `GFModuleEvent*`. With the V2 macro the dispatch
table handles routing; see [Events](#events).

---

## Macro Helpers

### `GFModuleDefine.h`

```cpp
// MyModule.cpp
#include "MyModule.h"
#include <GFSDKBasic.h>
#include <GFSDKLog.h>
#include "GFModuleDefine.h"

GF_MODULE_API_DEFINE_V2(
    "com.example.my_module",  // unique module ID (lower-case, dot-separated)
    "MyModule",               // display name
    "1.0.0",                  // module version
    "What this module does.", // description
    "Your Name"               // author
);
```

This macro:

- Implements `GFGetModuleGFSDKVersion`, `GFGetModuleQtEnvVersion`,
  `GFGetModuleID`, `GFGetModuleVersion`, `GFGetModuleMetaData`.
- Defines `MEvent` (`QMap<QString,QString>`) and creates the static event
  handler dispatch table.
- Implements `GFExecuteModule` to route events through `REGISTER_EVENT_HANDLER`.

### `GFModuleDeclare.h`

```cpp
// MyModule.h
#pragma once
#include "GFModuleDeclare.h"
GF_MODULE_API_DECLARE
```

This forward-declares all ten exported entry points with the correct
`GF_MODULE_EXPORT` visibility.

---

## Events

### Subscribing

Call `LISTEN("EVENT_ID")` inside `GFActiveModule`. Event IDs are
upper-case strings.

```cpp
auto GFActiveModule() -> int {
    LISTEN("MAINWINDOW_MENU_MOUNTED");
    LISTEN("APPLICATION_LOADED");
    return 0;
}
```

`LISTEN` expands to `GFModuleListenEvent(GFGetModuleID(), DUP("EVENT_ID"))`.

### Handling

Use `REGISTER_EVENT_HANDLER` at file scope. The macro stores a lambda in the
dispatch table; `GFExecuteModule` calls it when the event fires.

```cpp
REGISTER_EVENT_HANDLER(APPLICATION_LOADED, [](const MEvent& event) -> int {
    LOG_INFO("application loaded");
    // Use event["key"] to read event parameters.
    CB_SUCC(event);  // signals success and triggers the callback chain
});
```

Event parameter values are `QString`. The special keys `event_id` and
`trigger_id` are always present.

### Completing events

Every handler must call exactly one of:

```cpp
CB_SUCC(event);                  // success — sets ret=0 in callback params
CB_ERR(event, -1, "reason");     // failure — sets ret and err, then returns
CB_ERR_NO_RET(event, -1, "msg"); // failure — sets ret and err, does not return
```

These call `GFModuleTriggerModuleEventCallback` internally, which invokes the
next handler in the event chain.

### Accessing GUI objects from events

```cpp
auto* window = GFUIGetGUIObjectAs<QMainWindow>(event["main_window"]);
auto* menu   = GFUIGetGUIObjectAs<QMenu>(event["help_menu"]);
```

`GFUIGetGUIObjectAs<T>` is a typed wrapper around `GFUIGetGUIObject` from
`GFModuleCommonUtils.hpp`.

---

## Runtime Values

The runtime key-value store is a shared, in-process namespace for modules and
the host to exchange live configuration. Keys and namespaces are always stored
lower-case.

```cpp
// Write
GFModuleUpsertRTValue(DUP("my_module"), DUP("ready"), DUP("true"));
GFModuleUpsertRTValueBool(DUP("my_module"), DUP("enabled"), 1);

// Read with default
const char* v = GFModuleRetrieveRTValueOrDefault(
    DUP("my_module"), DUP("ready"), DUP("false"));
QString ready = UDUP(v);

int enabled = GFModuleRetrieveRTValueOrDefaultBool(
    DUP("my_module"), DUP("enabled"), 0);

// List children
char** keys = nullptr;
int32_t count = GFModuleListRTChildKeys(DUP("my_module"), DUP(""), &keys);
for (int i = 0; i < count; ++i) {
    QString key = UDUP(keys[i]);
}
GFFreeMemory(keys);
```

---

## Logging

```cpp
#include <GFSDKLog.h>

GFModuleLogTrace(DUP("trace message"));
GFModuleLogDebug(DUP("debug message"));
GFModuleLogInfo (DUP("info message"));
GFModuleLogWarn (DUP("warning"));
GFModuleLogError(DUP("error"));
```

With `GFModuleCommonUtils.hpp` included, use the `FLOG_*` macros for
`QString::arg`-style formatting:

```cpp
LOG_INFO("application loaded");
FLOG_DEBUG("processing event: %1", event["event_id"]);
FLOG_ERROR("failed to connect, code: %1", error_code);
```

---

## Cache

### In-memory cache (session lifetime)

```cpp
GFCacheSave(DUP("my_module.last_run"), DUP("2024-01-01"));
const char* v = GFCacheGet(DUP("my_module.last_run"));  // free with GFFreeMemory
QString last = UDUP(v);

// With time-to-live (seconds)
GFCacheSaveWithTTL(DUP("token"), DUP("abc123"), 3600);
```

### Durable cache (persists across restarts)

Values are stored as JSON strings. The key is automatically namespaced under an
internal module prefix.

```cpp
// Save a JSON document
QJsonObject obj;
obj["version"] = "2.1.0";
obj["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
GFDurableCacheSave(DUP("state"), DUP(QJsonDocument(obj).toJson()));

// Read back
const char* raw = GFDurableCacheGet(DUP("state"));
QString json_str = UDUP(raw);
auto doc = QJsonDocument::fromJson(json_str.toUtf8());
```

---

## GPG Operations

All GPG functions take a `channel` integer that selects the active GPG context.
Use `GFGpgCurrentGpgContextChannel()` to obtain the channel from the main
window, or pass a channel you obtained from a context-creation event.

### Sign

```cpp
char* key_ids[] = { DUP("FINGERPRINT") };
GFGpgSignResult* result = nullptr;
int ret = GFGpgSignData(channel, key_ids, 1, DUP(plaintext),
                        0 /*normal*/, 1 /*ascii*/, &result);
if (ret == 0) {
    QString sig = UDUP(result->signature);
    // analyse
    const char* report = nullptr;
    GFAnalyseSignResult(channel, result->gpgme_error,
                        result->gpgme_sign_result, &report);
    GFGpgFreeResult(result->gpgme_sign_result);
}
GFFreeMemory(result);
```

### Encrypt

```cpp
char* recipients[] = { DUP("FP1"), DUP("FP2") };
GFGpgEncryptionResult* result = nullptr;
int ret = GFGpgEncryptData(channel, recipients, 2,
                           DUP(plaintext), 1 /*ascii*/, &result);
if (ret == 0) {
    QString ciphertext = UDUP(result->encrypted_data);
    GFGpgFreeResult(result->gpgme_encrypt_result);
}
GFFreeMemory(result);
```

### Decrypt

```cpp
GFGpgDecryptResult* result = nullptr;
GFGpgDecryptData(channel, DUP(ciphertext), &result);
QString plain = UDUP(result->decrypted_data);
GFGpgFreeResult(result->gpgme_decrypt_result);
GFFreeMemory(result);
```

### Verify

```cpp
GFGpgVerifyResult* result = nullptr;
int ret = GFGpgVerifyData(channel, DUP(data), DUP(detached_sig), &result);
if (ret == 0) {
    const char* report = nullptr;
    GFAnalyseVerifyResult(channel, result->gpgme_error,
                          result->gpgme_verify_result, &report);
    QString analysis = UDUP(report);
    GFGpgFreeResult(result->gpgme_verify_result);
}
GFFreeMemory(result);
```

### Key export / import

```cpp
// Export a public key
char* key_data = nullptr;
int key_size = 0;
GFGpgExportKey(channel, DUP("FINGERPRINT"), 1 /*ascii*/,
               &key_data, &key_size);
QString armored = UDUP(key_data);

// Import keys from a buffer (shows the UI import dialog)
QByteArray pem = ...; // armored key block
GFGpgImportKeys(channel, parent_widget_ptr,
                pem.constData(), pem.size());
```

### Primary UID

```cpp
GFGpgKeyUID* uid = nullptr;
if (GFGpgKeyPrimaryUID(channel, DUP("FINGERPRINT"), &uid) == 0) {
    QString name    = UDUP(uid->name);
    QString email   = UDUP(uid->email);
    QString comment = UDUP(uid->comment);
    GFFreeMemory(uid);
}
```

---

## UI Integration

All Qt object creation and dialog display must happen on the main thread. The
SDK handles the thread dispatch for you.

### Creating a widget on the main thread

```cpp
// Define a factory
auto MyDialogFactory(void* data_raw_ptr) -> void* {
    auto data = ConvertVoidPtrToQVariant(data_raw_ptr);  // from GFModuleCommonUtils.hpp
    auto* dlg = new QDialog();
    // ... set up dlg using data ...
    return static_cast<void*>(dlg);
}

// Create the widget (blocks until the main thread finishes)
void* dlg_ptr = GFUICreateGUIObject(MyDialogFactory,
                                    ConvertQVariantToVoidPtr(QVariant("arg")));
// Or with the GUI_OBJECT macro:
void* dlg_ptr = GUI_OBJECT(MyDialogFactory, QVariant("arg"));
```

### Showing a dialog

```cpp
void* parent = GFUIGetGUIObject(DUP("main_window"));
GFUIShowDialog(dlg_ptr, parent);  // non-blocking; returns after scheduling show
```

### Accessing global settings

```cpp
auto* settings = qobject_cast<QSettings*>(
    static_cast<QObject*>(GFUIGlobalSettings()));
settings->setValue("my_module/key", "value");
```

### Registering a file-extension handler

When the user opens a file with the registered extension, the application emits
`<event_prefix>.OPEN` (exact event name depends on the host).

```cpp
GFUIRegisterFileExtensionHandleEvent(DUP("gpg"), DUP("MY_MODULE_FILE_OPEN"));
```

---

## Translations

Use `DEFINE_TRANSLATIONS_STRUCTURE` and `REGISTER_TRANS_READER` to wire up Qt
translations from embedded `.qm` resources.

```cpp
// In the .cpp file, before GFRegisterModule:
DEFINE_TRANSLATIONS_STRUCTURE(ModuleMyModule);

auto GFRegisterModule() -> int {
    REGISTER_TRANS_READER();
    return 0;
}
```

Wrap translatable strings with `QCoreApplication::translate("GTrC", "...")` or
the `GC_TR(...)` macro. The `GTrC` context is created automatically by
`DEFINE_TRANSLATIONS_STRUCTURE`.

---

## Build Setup

Modules are built inside the GpgFrontend superproject. A minimal
`CMakeLists.txt` for a module looks like:

```cmake
set(MODULE_ID "com.example.my_module")

file(GLOB_RECURSE MODULE_SOURCE "*.cpp" "*.h")

register_module(${MODULE_ID}
    LIBRARY_TARGET
    ${MODULE_SOURCE}
)

target_link_libraries(${LIBRARY_TARGET} PRIVATE
    gpgfrontend_sdk          # SDK link target provided by the superproject
    Qt6::Widgets
    Qt6::Network
)

# Optional: embed translations
qt_add_translations(${LIBRARY_TARGET}
    TS_FILES ts/ModuleMyModule.en_US.ts
)
```

Key CMake points:

- `GPGFRONTEND_BUILD_MODULES=ON` must be set in the superproject.
- Qt 5 builds skip module compilation; target Qt 6.
- `CMAKE_AUTOMOC`, `CMAKE_AUTORCC`, and `CMAKE_AUTOUIC` are enabled
  automatically by `register_module`.
- SDK headers are provided via the superproject include path; you do not need
  to specify them manually.

---

## Minimal Example

The following is a complete, self-contained module that logs a message when the
application finishes loading.

**MyModule.h**

```cpp
#pragma once
#include "GFModuleDeclare.h"
GF_MODULE_API_DECLARE
```

**MyModule.cpp**

```cpp
#include "MyModule.h"

#include <GFSDKBasic.h>
#include <GFSDKLog.h>

#include "GFModuleDefine.h"
#include "GFModuleCommonUtils.hpp"

GF_MODULE_API_DEFINE_V2(
    "com.example.my_module",
    "MyModule",
    "1.0.0",
    "Logs a greeting when the app loads.",
    "Your Name"
);

auto GFRegisterModule() -> int { return 0; }

auto GFActiveModule() -> int {
    LISTEN("APPLICATION_LOADED");
    return 0;
}

REGISTER_EVENT_HANDLER(APPLICATION_LOADED, [](const MEvent& event) -> int {
    LOG_INFO("Hello from MyModule!");
    CB_SUCC(event);
});

auto GFDeactivateModule() -> int { return 0; }
auto GFUnregisterModule() -> int { return 0; }
```

**CMakeLists.txt**

```cmake
register_module("com.example.my_module" LIBRARY_TARGET MyModule.cpp MyModule.h)
target_link_libraries(${LIBRARY_TARGET} PRIVATE Qt6::Widgets)
```
