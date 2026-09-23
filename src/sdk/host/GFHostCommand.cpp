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

#include <QCborValue>

#include "GFHostImpl.h"
#include "GFSDKHostApi.h"
#include "core/module/ModuleDispatchGate.h"
#include "core/module/ModuleManager.h"
#include "core/module/ModuleSdkBridge.h"
#include "core/thread/Task.h"
#include "core/thread/TaskRunnerGetter.h"
#include "private/GFHostAttribution.h"
#include "private/GFHostContext.h"
#include "private/GFHostEnter.h"
#include "private/GFHostGate.h"
#include "private/GFHostTransfer.h"
#include "private/GFSDKGpgInternal.h"
#include "private/GFSDKPrivate.h"
#include "ui/command/CommandRegistry.h"

/**
 * @file GFHostCommand.cpp
 * @brief The command group: module calls into the one Host registry.
 *
 * Nothing is decided here. The registry decides; this file only moves
 * buffers across the boundary -- taking them from one module, issuing them to
 * another -- and makes sure module code is entered the way every other entry
 * into module code is: through the dispatch gate, attributed to the module,
 * and never after the module has begun tearing down.
 */

namespace {

using GpgFrontend::GFBuffer;
using GpgFrontend::UI::CommandCaller;
using GpgFrontend::UI::CommandProvider;
using GpgFrontend::UI::CommandRegistry;

auto Registry() -> CommandRegistry& { return CommandRegistry::Instance(); }

/// Take ownership of every handle a module handed over, whatever else fails:
/// they were transferred, so a refusal must not leak them.
auto TakePayload(GFBufferRef cbor, GFBufferRef* blobs, size_t blob_count,
                 const char* what, QCborMap& out_map,
                 std::vector<gf::cmd::Blob>& out_blobs) -> bool {
  bool ok = true;

  std::optional<GFBuffer> map_bytes;
  if (cbor != nullptr) {
    map_bytes = gf_sdk_internal::TakeBufferForTransfer(cbor, what);
    ok = map_bytes.has_value();
  }

  out_blobs.clear();
  for (size_t i = 0; blobs != nullptr && i < blob_count; ++i) {
    auto b = gf_sdk_internal::TakeBufferForTransfer(blobs[i], what);
    if (!b.has_value()) {
      ok = false;
      continue;
    }
    out_blobs.push_back(GpgFrontend::UI::MakeHostBlob(std::move(*b)));
  }
  if (!ok) return false;

  out_map = QCborMap();
  if (map_bytes.has_value() && map_bytes->Size() > 0) {
    QCborParserError error{};
    const auto v = QCborValue::fromCbor(
        QByteArray::fromRawData(map_bytes->Data(),
                                static_cast<qsizetype>(map_bytes->Size())),
        &error);
    if (error.error != QCborError::NoError || !v.isMap()) return false;
    out_map = v.toMap();
  }
  return true;
}

/// Issue a result or an argument map to @p module as a buffer handle.
auto IssueMap(const QCborMap& m, const QString& module, const char* origin)
    -> GFBufferRef {
  return gf_sdk_internal::NewBufferFor(GFBuffer(QCborValue(m).toCbor()),
                                       module, origin);
}

auto IssueBlobs(const std::vector<gf::cmd::Blob>& blobs, const QString& module,
                const char* origin) -> std::vector<GFBufferRef> {
  std::vector<GFBufferRef> refs;
  refs.reserve(blobs.size());
  for (const auto& b : blobs) {
    refs.push_back(gf_sdk_internal::NewBufferFor(
        GpgFrontend::UI::BlobToGFBuffer(b), module, origin));
  }
  return refs;
}

/// The allowlist a verified module's manifest carries; nullopt if unverified.
auto AllowlistOf(const QString& module) -> std::optional<QStringList> {
  auto m = GpgFrontend::Module::ModuleManager::GetInstance().SearchModule(
      module);
  if (m == nullptr) return std::nullopt;
  const auto manifest = m->GetModuleManifest();
  if (!manifest.has_value()) return std::nullopt;
  return manifest->commands;
}

// ------------------------------------------------------------------ thunks

auto RegisterCommand(GFHostContextRef ctx, const GFCommandSpec* spec) -> int {
  GATE(ctx, 0, "command.register_command", GF_CMD_E_DENIED);
  if (spec == nullptr || spec->struct_size < sizeof(GFCommandSpec) ||
      spec->id == nullptr || spec->handler == nullptr) {
    return GF_CMD_E_BAD_ARGS;
  }

  const auto module = gf_sdk_internal::ContextModuleId(ctx);
  const auto descriptor_bytes =
      gf_sdk_internal::ReadBuffer(spec->descriptor_cbor, "command.register");
  if (!descriptor_bytes.has_value()) return GF_CMD_E_BAD_ARGS;
  const auto descriptor = QCborValue::fromCbor(QByteArray(
      descriptor_bytes->Data(), static_cast<qsizetype>(descriptor_bytes->Size())));
  if (!descriptor.isMap()) return GF_CMD_E_BAD_ARGS;

  CommandProvider p;
  p.id = QString::fromUtf8(spec->id);
  p.owner = module;
  p.descriptor = descriptor.toMap();
  p.required_caps = static_cast<uint32_t>(
      p.descriptor.value(QStringLiteral("required_caps")).toInteger());
  p.flags = static_cast<uint32_t>(
      p.descriptor.value(QStringLiteral("flags")).toInteger());

  const auto module_utf8 = module.toUtf8();
  const auto gui = (p.flags & gf::cmd::kNeedsGuiThread) != 0;
  const auto handler = spec->handler;
  const auto state = spec->state;
  auto* const user = spec->user;

  p.run = [module, module_utf8, gui, handler, user](
              const gf::cmd::CommandContext& cctx, QCborMap args,
              std::vector<gf::cmd::Blob> blobs, gf::cmd::Completer) {
    const auto call_id = static_cast<quint64>(cctx.call_id);
    gf::cmd::EncodeState st;
    auto context_map = gf::cmd::EncodeMap(cctx, st);

    auto enter = [=]() {
      const auto entered = gf_sdk_internal::EnterModule(module_utf8, [&]() {
        auto* context_ref = IssueMap(context_map, module, "command.context");
        auto* args_ref = IssueMap(args, module, "command.args");
        auto refs = IssueBlobs(blobs, module, "command.blob");
        handler(user, call_id, context_ref, args_ref, refs.data(),
                refs.size());
        // The context was only lent for the call.
        gf_host::GFBufferRelease(context_ref);
      });
      if (!entered) {
        Registry().Finish(call_id, module,
                          {GF_CMD_E_UNAVAILABLE, 0, {}, {},
                           QStringLiteral("the module is unloading")});
      }
    };

    if (gui) {
      enter();  // the registry already put us on the GUI thread
      return;
    }
    GpgFrontend::Thread::TaskRunnerGetter::GetInstance()
        .GetTaskRunner(
            GpgFrontend::Thread::TaskRunnerGetter::kTaskRunnerType_Module)
        ->PostTask(new GpgFrontend::Thread::Task(
            [enter](const GpgFrontend::DataObjectPtr&) -> int {
              enter();
              return 0;
            },
            QString("command/%1/%2").arg(module).arg(call_id)));
  };

  if (state != nullptr) {
    p.state = [module, module_utf8, state,
               user](const gf::cmd::CommandContext& cctx) -> uint32_t {
      uint32_t bits = 0;
      gf::cmd::EncodeState st;
      const auto context_map = gf::cmd::EncodeMap(cctx, st);
      gf_sdk_internal::EnterModule(module_utf8, [&]() {
        auto* context_ref = IssueMap(context_map, module, "command.context");
        bits = state(user, context_ref);
        gf_host::GFBufferRelease(context_ref);
      });
      return bits;
    };
  }

  return Registry().Register(std::move(p), AllowlistOf(module));
}

auto UnregisterCommand(GFHostContextRef ctx, const char* id) -> int {
  GATE(ctx, 0, "command.unregister_command", GF_CMD_E_DENIED);
  if (id == nullptr) return GF_CMD_E_BAD_ARGS;
  return Registry().Unregister(QString::fromUtf8(id),
                               gf_sdk_internal::ContextModuleId(ctx));
}

auto Invoke(GFHostContextRef ctx, const char* id, uint32_t /*flags*/,
            GFBufferRef args_cbor, GFBufferRef* blobs, size_t blob_count,
            GFCommandDoneFn done, void* user, uint64_t* out_call_id) -> int {
  if (out_call_id != nullptr) *out_call_id = 0;
  GATE(ctx, 0, "command.invoke", GF_CMD_E_DENIED);

  QCborMap args;
  std::vector<gf::cmd::Blob> taken;
  if (!TakePayload(args_cbor, blobs, blob_count, "command.invoke", args,
                   taken)) {
    return GF_CMD_E_BAD_ARGS;
  }
  if (id == nullptr) return GF_CMD_E_BAD_ARGS;

  CommandCaller caller;
  caller.module = gf_sdk_internal::ContextModuleId(ctx);
  caller.caps = gf_sdk_internal::ContextGranted(ctx);
  caller.source = QStringLiteral("module");

  const auto module = caller.module;
  const auto module_utf8 = module.toUtf8();
  gf::cmd::Completer completer = [module, module_utf8, done,
                                  user](gf::cmd::RawResult r) {
    if (done == nullptr) return;  // fire and forget: the result just drops
    gf_sdk_internal::EnterModule(module_utf8, [&]() {
      auto* result_ref = IssueMap(r.result, module, "command.result");
      auto refs = IssueBlobs(r.blobs, module, "command.result_blob");
      const auto error = r.error.toUtf8();
      done(user, static_cast<uint64_t>(r.call_id), r.status, result_ref,
           refs.data(), refs.size(),
           r.error.isEmpty() ? nullptr : error.constData());
    });
  };

  const auto ticket =
      Registry().Invoke(QString::fromUtf8(id), std::move(args),
                        std::move(taken), caller, {}, std::move(completer));
  if (out_call_id != nullptr) *out_call_id = ticket.call_id;
  return ticket.status;
}

auto Complete(GFHostContextRef ctx, uint64_t call_id, int status,
              GFBufferRef result_cbor, GFBufferRef* blobs, size_t blob_count,
              const char* error) -> int {
  GATE(ctx, 0, "command.complete", GF_CMD_E_DENIED);

  gf::cmd::RawResult r;
  r.status = status;
  if (!TakePayload(result_cbor, blobs, blob_count, "command.complete",
                   r.result, r.blobs)) {
    r = {GF_CMD_E_FAILED, 0, {}, {},
         QStringLiteral("the provider returned a malformed result")};
  } else if (error != nullptr) {
    r.error = QString::fromUtf8(error);
  }
  return Registry().Finish(call_id, gf_sdk_internal::ContextModuleId(ctx),
                           std::move(r));
}

auto Cancel(GFHostContextRef ctx, uint64_t call_id) -> int {
  GATE(ctx, 0, "command.cancel", GF_CMD_E_DENIED);
  return Registry().Cancel(call_id, gf_sdk_internal::ContextModuleId(ctx));
}

auto IsCancelled(GFHostContextRef ctx, uint64_t call_id) -> int {
  GATE(ctx, 0, "command.is_cancelled", 1);
  return Registry().IsCancelled(call_id) ? 1 : 0;
}

auto Describe(GFHostContextRef ctx, const char* id, GFBufferRef* out) -> int {
  GATE(ctx, 0, "command.describe", GF_CMD_E_DENIED);
  if (id == nullptr || out == nullptr) return GF_CMD_E_BAD_ARGS;
  *out = nullptr;
  const auto d = Registry().Describe(QString::fromUtf8(id));
  if (!d.has_value()) return GF_CMD_E_UNKNOWN;
  *out = IssueMap(*d, gf_sdk_internal::ContextModuleId(ctx),
                  "command.describe");
  return *out == nullptr ? GF_CMD_E_FAILED : GF_CMD_OK;
}

auto List(GFHostContextRef ctx, const char* prefix, GFStringListRef* out)
    -> int {
  GATE(ctx, 0, "command.list", GF_CMD_E_DENIED);
  if (out == nullptr) return GF_CMD_E_BAD_ARGS;
  const auto ids = Registry().List(
      prefix == nullptr ? QString() : QString::fromUtf8(prefix));
  return gf_sdk_internal::NewStringList(ids, out) == 0 ? GF_CMD_OK
                                                       : GF_CMD_E_FAILED;
}

auto QueryState(GFHostContextRef ctx, const char* id, uint32_t* out_bits)
    -> int {
  GATE(ctx, 0, "command.query_state", GF_CMD_E_DENIED);
  if (id == nullptr || out_bits == nullptr) return GF_CMD_E_BAD_ARGS;
  *out_bits = 0;
  if (!Registry().Contains(QString::fromUtf8(id))) return GF_CMD_E_UNKNOWN;

  // State is a question about the UI, answerable on its thread only.
  auto* app = QCoreApplication::instance();
  if (app != nullptr && QThread::currentThread() != app->thread()) {
    return GF_CMD_E_UNAVAILABLE;
  }
  CommandCaller caller;
  caller.module = gf_sdk_internal::ContextModuleId(ctx);
  caller.caps = gf_sdk_internal::ContextGranted(ctx);
  caller.source = QStringLiteral("module");
  *out_bits = Registry().State(QString::fromUtf8(id), caller, {});
  return GF_CMD_OK;
}

}  // namespace

namespace gf_sdk_internal {

const GFHostCommandApi kCommandApi = {
    sizeof(GFHostCommandApi), &RegisterCommand, &UnregisterCommand,
    &Invoke,                  &Complete,        &Cancel,
    &IsCancelled,             &Describe,        &List,
    &QueryState,
};

}  // namespace gf_sdk_internal
