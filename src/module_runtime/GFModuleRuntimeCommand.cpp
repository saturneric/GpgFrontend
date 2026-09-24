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

#include "GFModuleRuntimeCommand.h"

#include <QCborValue>
#include <QHash>
#include <QMutex>
#include <QMutexLocker>
#include <atomic>

#include "GFModuleRuntimeBoot.h"
#include "include/GFModule.h"

/**
 * @file GFModuleRuntimeCommand.cpp
 * @brief Commands, module-side: the C callbacks around gf::cmd::Binding and
 *        the bus that module code invokes through.
 */

namespace {

/// A Blob whose bytes are a buffer handle this module owns.
class ModuleBlobStorage : public gf::cmd::BlobStorage {
 public:
  explicit ModuleBlobStorage(GFBufferRef ref) : ref_(ref) {}
  ~ModuleBlobStorage() override {
    if (ref_ != nullptr) GFBufferRelease(gf::runtime::SdkContext(), ref_);
  }
  ModuleBlobStorage(const ModuleBlobStorage&) = delete;
  auto operator=(const ModuleBlobStorage&) -> ModuleBlobStorage& = delete;

  [[nodiscard]] auto Data() const -> const char* override {
    return static_cast<const char*>(
        GFBufferData(gf::runtime::SdkContext(), ref_));
  }
  [[nodiscard]] auto Size() const -> size_t override {
    return GFBufferSize(gf::runtime::SdkContext(), ref_);
  }

  /// Give the handle away. Only when nothing else shares this storage.
  auto Detach() -> GFBufferRef {
    auto* r = ref_;
    ref_ = nullptr;
    return r;
  }

 private:
  GFBufferRef ref_;
};

auto ReadMap(GFBufferView view) -> QCborMap {
  auto* ctx = gf::runtime::SdkContext();
  const auto* data = static_cast<const char*>(GFBufferData(ctx, view));
  const auto size = GFBufferSize(ctx, view);
  if (data == nullptr || size == 0) return {};
  return QCborValue::fromCbor(QByteArray(data, static_cast<qsizetype>(size)))
      .toMap();
}

/// Take a handle's CBOR and release the handle.
auto TakeMap(GFBufferRef ref) -> QCborMap {
  auto m = ReadMap(ref);
  if (ref != nullptr) GFBufferRelease(gf::runtime::SdkContext(), ref);
  return m;
}

auto WrapBlobs(GFBufferRef* refs, size_t n) -> std::vector<gf::cmd::Blob> {
  std::vector<gf::cmd::Blob> blobs;
  blobs.reserve(n);
  for (size_t i = 0; refs != nullptr && i < n; ++i) {
    blobs.emplace_back(std::make_shared<ModuleBlobStorage>(refs[i]));
  }
  return blobs;
}

auto IssueMap(const QCborMap& m) -> GFBufferRef {
  const auto bytes = QCborValue(m).toCbor();
  return GFBufferNewFromBytes(gf::runtime::SdkContext(), bytes.constData(),
                              static_cast<size_t>(bytes.size()));
}

/// Handles for the Host to take. A blob held nowhere else gives its own
/// handle away; one still shared elsewhere is copied into a fresh one, so
/// the other holder's bytes are not released from under it.
auto IssueBlobs(const std::vector<gf::cmd::Blob>& blobs)
    -> std::vector<GFBufferRef> {
  std::vector<GFBufferRef> refs;
  refs.reserve(blobs.size());
  for (const auto& b : blobs) {
    auto* mine = dynamic_cast<ModuleBlobStorage*>(b.Storage().get());
    if (mine != nullptr && b.Storage().use_count() == 1) {
      refs.push_back(mine->Detach());
      continue;
    }
    refs.push_back(
        GFBufferNewFromBytes(gf::runtime::SdkContext(), b.Data(), b.Size()));
  }
  return refs;
}

auto DecodeContext(GFBufferView view, quint64 call_id)
    -> gf::cmd::CommandContext {
  gf::cmd::CommandContext ctx;
  gf::cmd::DecodeMap(ReadMap(view), {}, ctx);
  ctx.cancelled = [call_id]() {
    return GFCommandIsCancelled(gf::runtime::SdkContext(), call_id) != 0;
  };
  return ctx;
}

// ------------------------------------------------------------ providing

void HandlerTrampoline(void* user, uint64_t call_id, GFBufferView context_cbor,
                       GFBufferRef args_cbor, GFBufferRef* blobs,
                       size_t blob_count) {
  const auto* binding = static_cast<const gf::cmd::Binding*>(user);
  auto ctx = DecodeContext(context_cbor, call_id);
  auto args = TakeMap(args_cbor);
  auto wrapped = WrapBlobs(blobs, blob_count);

  binding->run(ctx, std::move(args), std::move(wrapped),
               [call_id](gf::cmd::RawResult r) {
                 auto* result = IssueMap(r.result);
                 auto refs = IssueBlobs(r.blobs);
                 const auto error = r.error.toUtf8();
                 GFCommandComplete(
                     gf::runtime::SdkContext(), call_id, r.status, result,
                     refs.data(), refs.size(),
                     r.error.isEmpty() ? nullptr : error.constData());
               });
}

auto StateTrampoline(void* user, GFBufferView context_cbor) -> uint32_t {
  const auto* binding = static_cast<const gf::cmd::Binding*>(user);
  return binding->state(DecodeContext(context_cbor, 0));
}

// ------------------------------------------------------------ invoking

struct Continuation {
  QPointer<QObject> receiver;
  bool has_receiver = false;
  gf::cmd::CommandBus::RawFn fn;
  quint64 call_id = 0;
  /// Set by Cancel() and by deactivation. A result already queued to the
  /// receiver checks it when it runs, so "no callback after cancel" holds
  /// for that one too.
  std::atomic<bool> dropped{false};
};

/// Continuations owed to this module. Keyed by the `user` pointer the Host
/// hands back, and inserted BEFORE the call is made: the Host may complete a
/// call inline, before GFCommandInvoke returns, and the continuation has to
/// be findable then. Owned here, so that the ones the Host never calls back
/// -- cancelled, or dropped at teardown -- are still freed.
struct Continuations {
  QMutex mutex;
  QHash<Continuation*, std::shared_ptr<Continuation>> live;
};

auto Owed() -> Continuations& {
  static Continuations c;
  return c;
}

void DoneTrampoline(void* user, uint64_t call_id, int status,
                    GFBufferRef result_cbor, GFBufferRef* blobs,
                    size_t blob_count, const char* error) {
  gf::cmd::RawResult r;
  r.status = status;
  r.call_id = static_cast<qint64>(call_id);
  r.result = TakeMap(result_cbor);
  r.blobs = WrapBlobs(blobs, blob_count);
  if (error != nullptr) r.error = QString::fromUtf8(error);

  std::shared_ptr<Continuation> c;
  {
    auto& owed = Owed();
    QMutexLocker locker(&owed.mutex);
    c = owed.live.take(static_cast<Continuation*>(user));
  }
  if (c == nullptr || !c->fn) return;

  if (!c->has_receiver) {
    c->fn(r);
    return;
  }
  if (c->receiver.isNull()) return;  // the receiver is gone: so is interest
  QMetaObject::invokeMethod(
      c->receiver.data(),
      [c, r = std::move(r)]() {
        if (c->dropped.load() || c->receiver.isNull()) return;
        c->fn(r);
      },
      Qt::QueuedConnection);
}

}  // namespace

namespace gf::runtime {

auto RegisterCommands(const gf::cmd::Binding* bindings, size_t count) -> bool {
  const auto& facts = Facts();
  bool ok = true;
  for (size_t i = 0; bindings != nullptr && i < count; ++i) {
    const auto& b = bindings[i];
    const auto id = QString::fromUtf8(b.id);

    // The manifest is the allowlist; the Host checks it too, but refusing
    // here says which side is wrong in the module's own log.
    if (!facts.commands.contains(id)) {
      LOG_ERROR(QString("module %1 provides command %2, which its manifest "
                        "does not declare")
                    .arg(facts.id, id));
      ok = false;
      continue;
    }

    auto* descriptor = IssueMap(b.describe());
    GFCommandSpec spec{};
    spec.struct_size = sizeof(GFCommandSpec);
    spec.id = b.id;
    spec.descriptor_cbor = descriptor;
    spec.handler = &HandlerTrampoline;
    spec.state = b.state != nullptr ? &StateTrampoline : nullptr;
    spec.user = const_cast<gf::cmd::Binding*>(&b);

    const auto rc = GFCommandRegister(SdkContext(), &spec);
    GFBufferRelease(SdkContext(), descriptor);
    if (rc != GF_CMD_OK) {
      LOG_ERROR(QString("module %1: the host refused command %2 (%3)")
                    .arg(facts.id, id)
                    .arg(rc));
      ok = false;
    }
  }
  return ok;
}

void DropContinuations() {
  auto& owed = Owed();
  QMutexLocker locker(&owed.mutex);
  for (const auto& c : std::as_const(owed.live)) c->dropped = true;
  owed.live.clear();
}

}  // namespace gf::runtime

namespace gf::cmd {

auto MakeBlob(const void* data, size_t size) -> Blob {
  auto* ref = GFBufferNewFromBytes(gf::runtime::SdkContext(), data, size);
  if (ref == nullptr) return Blob();
  return Blob(std::make_shared<ModuleBlobStorage>(ref));
}

auto CommandBus::InvokeRaw(const char* id, QCborMap args,
                           std::vector<Blob> blobs, QObject* receiver, RawFn fn)
    -> CallTicket {
  auto* args_ref = IssueMap(args);
  auto refs = IssueBlobs(blobs);

  std::shared_ptr<Continuation> c;
  auto& owed = Owed();
  if (fn) {
    c = std::make_shared<Continuation>();
    c->receiver = receiver;
    c->has_receiver = receiver != nullptr;
    c->fn = std::move(fn);
    QMutexLocker locker(&owed.mutex);
    owed.live.insert(c.get(), c);
  }

  uint64_t call_id = 0;
  const auto status = GFCommandInvoke(
      gf::runtime::SdkContext(), id, 0, args_ref, refs.data(), refs.size(),
      c ? &DoneTrampoline : nullptr, c.get(), &call_id);

  if (c) {
    QMutexLocker locker(&owed.mutex);
    if (status != GF_CMD_OK) {
      owed.live.remove(c.get());  // refused: `done` will never run
    } else if (owed.live.contains(c.get())) {
      c->call_id = call_id;  // still pending, so it can be cancelled by id
    }
  }
  return {status, call_id};
}

auto CommandBus::InvokeDynamic(const QString& id, const QCborMap& args,
                               QObject* receiver, RawFn fn) -> CallTicket {
  const auto utf8 = id.toUtf8();
  return InvokeRaw(utf8.constData(), args, {}, receiver, std::move(fn));
}

auto CommandBus::Cancel(quint64 call_id) -> int {
  const auto rc = GFCommandCancel(gf::runtime::SdkContext(), call_id);
  auto& owed = Owed();
  QMutexLocker locker(&owed.mutex);
  for (auto it = owed.live.begin(); it != owed.live.end();) {
    if ((*it)->call_id == call_id) {
      (*it)->dropped = true;
      it = owed.live.erase(it);
    } else {
      ++it;
    }
  }
  return rc;
}

auto CommandBus::Describe(const QString& id) -> std::optional<QCborMap> {
  GFBufferRef out = nullptr;
  const auto utf8 = id.toUtf8();
  if (GFCommandDescribe(gf::runtime::SdkContext(), utf8.constData(), &out) !=
      GF_CMD_OK) {
    return std::nullopt;
  }
  return TakeMap(out);
}

auto CommandBus::List(const QString& prefix) -> QStringList {
  auto* ctx = gf::runtime::SdkContext();
  GFStringListRef list = nullptr;
  const auto utf8 = prefix.toUtf8();
  QStringList ids;
  if (GFCommandList(ctx, utf8.constData(), &list) != GF_CMD_OK) return ids;
  const auto n = GFStringListCount(ctx, list);
  for (size_t i = 0; i < n; ++i) {
    ids.append(QString::fromUtf8(GFStringListAt(ctx, list, i)));
  }
  GFStringListRelease(ctx, list);
  return ids;
}

}  // namespace gf::cmd

auto Commands() -> gf::cmd::CommandBus& {
  static gf::cmd::CommandBus bus;
  return bus;
}
