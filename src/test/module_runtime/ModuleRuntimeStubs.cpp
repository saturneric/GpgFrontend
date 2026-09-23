/**
 * Copyright (C) 2021-2024 Saturneric <eric@bktus.com>
 *
 * This file is part of GpgFrontend.
 *
 * GpgFrontend is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * GpgFrontend is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GpgFrontend. If not, see <https://www.gnu.org/licenses/>.
 *
 * The initial version of the source code is inherited from
 * the gpg4usb project, which is under GPL-3.0-or-later.
 *
 * All the source code of GpgFrontend was modified and released by
 * Saturneric <eric@bktus.com> starting on May 12, 2021.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include "ModuleRuntimeStubs.h"

#include <GFSDKBuildInfo.h>
#include <GFSDKHostApi.h>
#include <GFSDKLog.h>
#include <GFSDKTypes.h>

#include <QByteArray>
#include <QMap>
#include <QString>
#include <QThread>
#include <cstdint>
#include <cstdlib>
#include <cstring>

/**
 * @file ModuleRuntimeStubs.cpp
 * @brief The host, reduced to a GFHostApi that records what it was asked.
 *
 * ## What changed here, and why it is a better test
 *
 * This file used to define the SDK's exported C symbols by hand -- seventeen
 * of them -- because the runtime declared them and left them undefined for a
 * module to resolve. That made the stubs a dependency fence, and it also
 * meant the thing under test was the runtime's PLUMBING and never the SDK
 * surface a module actually calls.
 *
 * The runtime now DEFINES the whole SDK on top of a capability table, so the
 * host to fake is that table. Every GFSDK* call a test makes runs the real
 * wrapper code and lands here, which is what lets a test assert the property
 * that matters: that the high-level helpers are implemented purely in terms
 * of the primitives, and that a wrapper whose group was withheld refuses
 * instead of calling through a null pointer.
 *
 * ## The context
 *
 * A single static record, whose address is the context. The real host
 * validates that pointer against its registry; nothing here needs to, because
 * a test binary has exactly one module. What the tests DO use it for is
 * thread independence: the same context is passed from a worker thread and
 * the call is served, which is the whole reason authorization stopped living
 * in thread-local state.
 */

namespace stubs {

namespace {

/// The thread the table was built on, for the off-thread tally.
QThread* g_home_thread = nullptr;

struct FakeContext {
  const char* module_id = "com.bktus.gpgfrontend.module.test";
};

auto TheContext() -> FakeContext& {
  static FakeContext ctx;
  return ctx;
}

auto AsRef() -> GFHostContextRef {
  return reinterpret_cast<GFHostContextRef>(&TheContext());
}

void Note() {
  if (g_home_thread != nullptr && QThread::currentThread() != g_home_thread) {
    Rec().calls_off_thread++;
  }
}

/* --- buffer -------------------------------------------------------------- */

/// A buffer handle is a heap block holding its own length. Enough to be a
/// real handle with real octets, which is what the wrappers move around.
struct FakeBuffer {
  size_t size;
  char* data;
};

auto BufNew(GFHostContextRef, const void* data, size_t size) -> GFBufferRef {
  Note();
  auto* b = new FakeBuffer{size, static_cast<char*>(std::malloc(size + 1))};
  if (size != 0 && data != nullptr) std::memcpy(b->data, data, size);
  b->data[size] = '\0';
  Rec().allocations++;
  return reinterpret_cast<GFBufferRef>(b);
}

auto BufData(GFHostContextRef, GFBufferView buf) -> const void* {
  if (buf == nullptr) return nullptr;
  return reinterpret_cast<const FakeBuffer*>(buf)->data;
}

auto BufSize(GFHostContextRef, GFBufferView buf) -> size_t {
  if (buf == nullptr) return 0;
  return reinterpret_cast<const FakeBuffer*>(buf)->size;
}

void BufZeroize(GFHostContextRef, GFBufferRef buf) {
  if (buf == nullptr) return;
  auto* b = reinterpret_cast<FakeBuffer*>(buf);
  std::memset(b->data, 0, b->size);
}

void BufRelease(GFHostContextRef, GFBufferRef buf) {
  if (buf == nullptr) return;
  auto* b = reinterpret_cast<FakeBuffer*>(buf);
  std::free(b->data);
  delete b;
  Rec().frees++;
}

auto BufOutstanding(GFHostContextRef) -> size_t {
  return static_cast<size_t>(Rec().allocations - Rec().frees);
}

auto MemAlloc(GFHostContextRef, int /*arena*/, uint32_t size) -> void* {
  Note();
  Rec().allocations++;
  return std::malloc(size);
}

auto MemRealloc(GFHostContextRef, int /*arena*/, void* p, uint32_t size)
    -> void* {
  return std::realloc(p, size);
}

void MemFree(GFHostContextRef, int /*arena*/, void* p) {
  if (p == nullptr) return;
  Rec().frees++;
  std::free(p);
}

auto MemStrDup(GFHostContextRef, int /*arena*/, const char* s) -> char* {
  Note();
  if (s == nullptr) return nullptr;
  const auto n = std::strlen(s);
  auto* copy = static_cast<char*>(std::malloc(n + 1));
  std::memcpy(copy, s, n + 1);
  Rec().allocations++;
  return copy;
}

const GFHostBufferApi kBuffer = {
    sizeof(GFHostBufferApi),
    &BufNew,
    &BufData,
    &BufSize,
    &BufZeroize,
    &BufRelease,
    &BufOutstanding,
    &MemAlloc,
    &MemRealloc,
    &MemFree,
    &MemStrDup,
};

/* --- log ----------------------------------------------------------------- */

void LogWrite(GFHostContextRef, int severity, const char* /*file*/,
              int /*line*/, const char* /*function*/, const char* msg) {
  Note();
  const auto text = QString::fromUtf8(msg == nullptr ? "" : msg);
  if (severity == GF_LOG_WARN) Rec().warnings.append(text);
  if (severity == GF_LOG_ERROR) Rec().errors.append(text);
}

auto LogEnabled(GFHostContextRef, int /*severity*/) -> int { return 1; }

const GFHostLogApi kLog = {sizeof(GFHostLogApi), &LogWrite, &LogEnabled};

/* --- app ----------------------------------------------------------------- */

auto AppText(GFHostContextRef) -> const char* { return "test"; }
auto AppLocale(GFHostContextRef ctx) -> char* {
  return MemStrDup(ctx, GF_ARENA_NORMAL, "en_US");
}
auto AppZero(GFHostContextRef) -> int { return 0; }

const GFHostAppApi kApp = {
    sizeof(GFHostAppApi), &AppText, &AppText, &AppText, &AppText,
    &AppLocale,           &AppZero, &AppZero,
};

/* --- event --------------------------------------------------------------- */

auto EventSubscribe(GFHostContextRef ctx, const char* event_id) -> int {
  Note();
  Rec().listened.append(QString::fromUtf8(event_id));
  Rec().listened_as =
      QString::fromUtf8(reinterpret_cast<const FakeContext*>(ctx)->module_id);
  return 0;
}

auto EventAnswer(GFHostContextRef, const GFModuleEventAnswer* answer) -> int {
  Note();
  QMap<QString, QString> params;
  auto* node = answer == nullptr ? nullptr : answer->params;
  while (node != nullptr) {
    params.insert(QString::fromUtf8(node->name),
                  QString::fromUtf8(node->value));
    auto* next = node->next;
    // The host owns what it is handed, on every path. Freeing it here is what
    // makes the allocation/free tally in the tests mean something.
    MemFree(nullptr, GF_ARENA_NORMAL, const_cast<char*>(node->name));
    MemFree(nullptr, GF_ARENA_NORMAL, const_cast<char*>(node->value));
    MemFree(nullptr, GF_ARENA_NORMAL, node);
    node = next;
  }
  Rec().answers.append(params);
  return 0;
}

const GFHostEventApi kEvent = {sizeof(GFHostEventApi), &EventSubscribe,
                               &EventAnswer};

/* --- bootstrap ----------------------------------------------------------- */

auto RegisterTranslatorReader(GFHostContextRef, const char* id,
                              GFTranslatorDataReader reader) -> int {
  Rec().translator_registered_for = QString::fromUtf8(id);
  Rec().translator_reader = reader;
  return 0;
}

const GFHostBootstrapApi kBootstrap = {sizeof(GFHostBootstrapApi),
                                       &RegisterTranslatorReader};

/* --- list ---------------------------------------------------------------- */

auto ListCount(GFHostContextRef, GFStringListRef) -> size_t { return 0; }
auto ListAt(GFHostContextRef, GFStringListRef, size_t) -> const char* {
  return nullptr;
}
void ListRelease(GFHostContextRef, GFStringListRef) {}
auto ListOutstanding(GFHostContextRef) -> size_t { return 0; }

const GFHostListApi kList = {sizeof(GFHostListApi), &ListCount, &ListAt,
                             &ListRelease, &ListOutstanding};

/* --- ui ------------------------------------------------------------------ */

auto UiGetObject(GFHostContextRef, const char* /*id*/) -> void* {
  // No QObject registry in a runtime test: a handle never resolves, which is
  // the case GFEvent::RequireGui has to report rather than crash on.
  return nullptr;
}

auto UiCreateObject(GFHostContextRef, QObjectFactory, void*) -> void* {
  return nullptr;
}
auto UiShowDialog(GFHostContextRef, void*, void*) -> int { return 0; }
auto UiThemeColor(GFHostContextRef, int role, void*) -> uint32_t {
  // Distinct per role, so a test can tell which role a wrapper asked for --
  // the property that matters now that five colour functions share one call.
  return 0xFF000000U | static_cast<uint32_t>(role);
}
auto UiUserFilePath(GFHostContextRef ctx) -> GFBufferRef {
  return BufNew(ctx, "/tmp", 4);
}
auto UiRegSettings(GFHostContextRef, const GFUISettingsPageSpec*) -> int {
  return 0;
}
auto UiUnregSettings(GFHostContextRef, const char*) -> int { return 0; }
auto UiRegTab(GFHostContextRef, const GFUITabViewSpec*) -> int { return 0; }
auto UiUnregTab(GFHostContextRef, const char*) -> int { return 0; }
auto UiRegFileExt(GFHostContextRef, const char*, const char*) -> int {
  return 0;
}

const GFHostUiApi kUi = {
    sizeof(GFHostUiApi), &UiCreateObject, &UiGetObject,   &UiShowDialog,
    &UiThemeColor,       &UiUserFilePath, &UiRegSettings, &UiUnregSettings,
    &UiRegTab,           &UiUnregTab,     &UiRegFileExt,
};

/* --- command ------------------------------------------------------------- */

struct RegisteredCommand {
  GFCommandHandlerFn handler = nullptr;
  void* user = nullptr;
};

struct PendingCall {
  GFCommandDoneFn done = nullptr;
  void* user = nullptr;
};

auto Registered() -> QMap<QString, RegisteredCommand>& {
  static QMap<QString, RegisteredCommand> m;
  return m;
}

auto Calls() -> QMap<uint64_t, PendingCall>& {
  static QMap<uint64_t, PendingCall> m;
  return m;
}

uint64_t g_next_call = 1;

void ReleaseAll(GFHostContextRef ctx, GFBufferRef cbor, GFBufferRef* blobs,
                size_t n) {
  BufRelease(ctx, cbor);
  for (size_t i = 0; blobs != nullptr && i < n; ++i) BufRelease(ctx, blobs[i]);
}

auto CmdRegister(GFHostContextRef, const GFCommandSpec* spec) -> int {
  Registered().insert(QString::fromUtf8(spec->id),
                      RegisteredCommand{spec->handler, spec->user});
  Rec().commands_registered.append(QString::fromUtf8(spec->id));
  return GF_CMD_OK;
}

auto CmdUnregister(GFHostContextRef, const char* id) -> int {
  return Registered().remove(QString::fromUtf8(id)) > 0 ? GF_CMD_OK
                                                        : GF_CMD_E_UNKNOWN;
}

auto CmdInvoke(GFHostContextRef ctx, const char* id, uint32_t,
               GFBufferRef args, GFBufferRef* blobs, size_t n,
               GFCommandDoneFn done, void* user, uint64_t* out_call) -> int {
  Rec().commands_invoked.append(QString::fromUtf8(id));
  const auto it = Registered().constFind(QString::fromUtf8(id));
  if (it == Registered().constEnd()) {
    ReleaseAll(ctx, args, blobs, n);
    return GF_CMD_E_UNKNOWN;
  }
  const auto call = g_next_call++;
  if (out_call != nullptr) *out_call = call;
  Calls().insert(call, PendingCall{done, user});

  const QByteArray context("\xa0", 1);  // an empty CBOR map
  auto* context_buf = BufNew(ctx, context.constData(), 1);
  it->handler(it->user, call, context_buf, args, blobs, n);
  BufRelease(ctx, context_buf);
  return GF_CMD_OK;
}

auto CmdComplete(GFHostContextRef ctx, uint64_t call, int status,
                 GFBufferRef result, GFBufferRef* blobs, size_t n,
                 const char* error) -> int {
  Rec().completions++;
  const auto pending = Calls().take(call);
  if (pending.done == nullptr) {
    ReleaseAll(ctx, result, blobs, n);
    return GF_CMD_OK;
  }
  pending.done(pending.user, call, status, result, blobs, n, error);
  return GF_CMD_OK;
}

auto CmdCancel(GFHostContextRef, uint64_t call) -> int {
  return Calls().remove(call) > 0 ? GF_CMD_OK : GF_CMD_E_UNKNOWN;
}
auto CmdIsCancelled(GFHostContextRef, uint64_t call) -> int {
  return Calls().contains(call) ? 0 : 1;
}
auto CmdDescribe(GFHostContextRef, const char*, GFBufferRef*) -> int {
  return GF_CMD_E_UNKNOWN;
}
auto CmdList(GFHostContextRef, const char*, GFStringListRef*) -> int {
  return GF_CMD_E_UNKNOWN;
}
auto CmdQueryState(GFHostContextRef, const char*, uint32_t*) -> int {
  return GF_CMD_E_UNKNOWN;
}

const GFHostCommandApi kCommand = {
    sizeof(GFHostCommandApi), &CmdRegister, &CmdUnregister, &CmdInvoke,
    &CmdComplete,             &CmdCancel,   &CmdIsCancelled, &CmdDescribe,
    &CmdList,                 &CmdQueryState,
};

}  // namespace

Recorder& Rec() {
  static Recorder r;
  return r;
}

void Recorder::Reset() { *this = Recorder{}; }

auto FakeModuleId() -> const char* { return TheContext().module_id; }

auto MakeHostApi(uint32_t granted) -> GFHostApi {
  g_home_thread = QThread::currentThread();

  GFHostApi host{};
  host.struct_size = sizeof(GFHostApi);
  host.abi_version = GF_SDK_ABI_VERSION;
  host.granted = granted;
  host.module_id = TheContext().module_id;
  host.context = AsRef();

  host.buffer = &kBuffer;
  host.log = &kLog;
  host.app = &kApp;
  host.event = &kEvent;
  host.bootstrap = &kBootstrap;
  host.list = &kList;

  // Withheld unless asked for, exactly as the real mint withholds them. The
  // UI group is the only grantable one this fake implements, because it is
  // the only one the runtime itself reaches (GFEvent::RequireGui); the rest
  // exist in these tests precisely to be absent.
  host.ui = (granted & GF_HOST_CAP_UI) != 0 ? &kUi : nullptr;

  // Always present, as in the real mint. Each test starts with a host that
  // has no commands registered and no calls in flight.
  Registered().clear();
  Calls().clear();
  host.command = &kCommand;
  return host;
}

}  // namespace stubs
