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

#include "CommandRegistry.h"

#include <QCoreApplication>
#include <QThread>
#include <atomic>

#include "core/module/ModuleNamespace.h"
#include "sdk/GFSDKHostCommands.hpp"

namespace GpgFrontend::UI {

namespace {

class HostBlobStorage : public gf::cmd::BlobStorage {
 public:
  explicit HostBlobStorage(GFBuffer buffer) : buffer_(std::move(buffer)) {}

  [[nodiscard]] auto Data() const -> const char* override {
    return buffer_.Data();
  }
  [[nodiscard]] auto Size() const -> size_t override { return buffer_.Size(); }
  [[nodiscard]] auto Buffer() const -> const GFBuffer& { return buffer_; }

 private:
  GFBuffer buffer_;
};

auto OnGuiThread() -> bool {
  auto* app = QCoreApplication::instance();
  return app == nullptr || QThread::currentThread() == app->thread();
}

/// Run @p fn on the GUI thread, now if already there.
void OnGui(std::function<void()> fn) {
  if (OnGuiThread()) {
    fn();
    return;
  }
  QMetaObject::invokeMethod(QCoreApplication::instance(), std::move(fn),
                            Qt::QueuedConnection);
}

auto ReadUInt(const QCborMap& m, const char* key) -> uint32_t {
  const auto v = m.value(QString::fromLatin1(key));
  return v.isInteger() ? static_cast<uint32_t>(v.toInteger()) : 0U;
}

/// The argument and result shape every codec has.
struct CodecShape {
  static constexpr gf::cmd::Meta kMeta{"", "", "", "", 0, 0};
  using Args = gf::cmd::host::CodecArgs;
  using Result = gf::cmd::host::CodecResult;
};

/**
 * @brief Why @p p may not be a codec, or empty when it may (or is none).
 *
 * The Host hands a decoder the text of whatever the user decrypts, which may
 * well be plaintext: so only a module that may read the editor anyway gets
 * to be one. Off the GUI thread, a codec cannot show a dialog; with the one
 * shape, the Host never has to guess what it returns.
 */
auto CodecRefusal(const CommandProvider& p) -> QString {
  constexpr uint32_t kCodec = gf::cmd::kInputDecoder | gf::cmd::kOutputEncoder;
  if ((p.flags & kCodec) == 0) return {};
  if ((p.flags & gf::cmd::kNeedsGuiThread) != 0) {
    return QStringLiteral("a codec does not run on the GUI thread");
  }
  if (!p.owner.isEmpty() && (p.owner_caps & GF_HOST_CAP_EDITOR) == 0) {
    return QStringLiteral("a codec's module needs the editor capability");
  }
  static const auto kShape = gf::cmd::Describe<CodecShape>();
  for (const auto* key : {"args", "result"}) {
    const auto k = QString::fromLatin1(key);
    if (p.descriptor.value(k) != kShape.value(k)) {
      return QStringLiteral("its %1 are not the codec's").arg(k);
    }
  }
  return {};
}

}  // namespace

namespace {

auto Translated(const QCborMap& d, const char* key) -> QString {
  const auto text = d.value(QString::fromLatin1(key)).toString();
  if (text.isEmpty()) return text;
  auto context = d.value(QStringLiteral("tr_context")).toString();
  if (context.isEmpty()) context = QStringLiteral("GTrC");
  return QCoreApplication::translate(context.toUtf8().constData(),
                                     text.toUtf8().constData());
}

}  // namespace

auto CommandTitle(const QCborMap& descriptor) -> QString {
  return Translated(descriptor, "title");
}

auto CommandDescription(const QCborMap& descriptor) -> QString {
  return Translated(descriptor, "description");
}

auto CommandCategory(const QCborMap& descriptor) -> QString {
  return Translated(descriptor, "category");
}

auto MakeHostBlob(GFBuffer buffer) -> gf::cmd::Blob {
  return gf::cmd::Blob(std::make_shared<HostBlobStorage>(std::move(buffer)));
}

auto BlobToGFBuffer(const gf::cmd::Blob& blob) -> GFBuffer {
  if (blob.IsNull()) return {};
  if (const auto* host =
          dynamic_cast<const HostBlobStorage*>(blob.Storage().get())) {
    return host->Buffer();
  }
  // Bytes some other storage holds: copied once, into memory that is wiped.
  return GFBuffer(blob.Data(), blob.Size());
}

/**
 * @brief One call in flight.
 *
 * `delivery` is held while `done` runs, and Cancel() takes it too. That is
 * the whole of the "never after cancel returns" guarantee: a cancel that
 * races a completion either clears `done` first, or waits for it to finish.
 * Recursive, because `done` may itself cancel another call -- or this one.
 */
struct CommandRegistry::Pending {
  quint64 id = 0;
  QString caller;
  QString provider;
  std::atomic<bool> cancelled{false};

  QRecursiveMutex delivery;
  bool finished = false;    // guarded by delivery
  gf::cmd::Completer done;  // guarded by delivery
};

auto CommandRegistry::Instance() -> CommandRegistry& {
  static CommandRegistry registry;
  return registry;
}

CommandRegistry::CommandRegistry() = default;
CommandRegistry::~CommandRegistry() = default;

auto CommandRegistry::Lookup(const QString& id)
    -> std::shared_ptr<const CommandProvider> {
  QMutexLocker locker(&mutex_);
  return providers_.value(id);
}

auto CommandRegistry::Register(CommandProvider provider,
                               const std::optional<QStringList>& allowlist)
    -> int {
  if (provider.id.isEmpty() || !provider.run) return GF_CMD_E_BAD_ARGS;

  if (provider.owner.isEmpty()) {
    // The Host's own namespace, and only the Host's.
    if (!provider.id.startsWith(QStringLiteral("org.gpgfrontend."))) {
      return GF_CMD_E_DENIED;
    }
  } else {
    if (!Module::IsOwnedName(provider.owner, provider.id)) {
      LOG_W() << "module" << provider.owner << "may not register command"
              << provider.id << ": outside its namespace";
      return GF_CMD_E_DENIED;
    }
    if (allowlist.has_value() && !allowlist->contains(provider.id)) {
      LOG_W() << "module" << provider.owner << "may not register command"
              << provider.id << ": its signed manifest does not list it";
      return GF_CMD_E_DENIED;
    }
    if ((provider.flags & gf::cmd::kHostOnly) != 0) return GF_CMD_E_DENIED;
  }

  if (const auto why = CodecRefusal(provider); !why.isEmpty()) {
    LOG_W() << "command" << provider.id << "refused as a codec:" << why;
    return GF_CMD_E_DENIED;
  }

  QMutexLocker locker(&mutex_);
  if (!provider.owner.isEmpty() && closed_.contains(provider.owner)) {
    return GF_CMD_E_UNAVAILABLE;
  }
  if (providers_.contains(provider.id)) return GF_CMD_E_DENIED;
  const auto id = provider.id;
  providers_.insert(
      id, std::make_shared<const CommandProvider>(std::move(provider)));
  return GF_CMD_OK;
}

auto CommandRegistry::RegisterHost(const gf::cmd::Binding& binding,
                                   const HostCommandText& text) -> int {
  CommandProvider p;
  p.id = QString::fromUtf8(binding.id);
  p.descriptor = binding.describe();
  p.descriptor.insert(QStringLiteral("tr_context"),
                      QString::fromLatin1(text.tr_context));
  p.descriptor.insert(QStringLiteral("title"), QString::fromUtf8(text.title));
  p.descriptor.insert(QStringLiteral("description"),
                      QString::fromUtf8(text.description));
  p.descriptor.insert(QStringLiteral("category"),
                      QString::fromUtf8(text.category));
  Unregister(p.id, {});
  p.required_caps = ReadUInt(p.descriptor, "required_caps");
  p.flags = ReadUInt(p.descriptor, "flags");
  p.run = binding.run;
  if (binding.state != nullptr) p.state = binding.state;
  return Register(std::move(p));
}

auto CommandRegistry::Unregister(const QString& id, const QString& owner)
    -> int {
  QMutexLocker locker(&mutex_);
  const auto it = providers_.constFind(id);
  if (it == providers_.constEnd()) return GF_CMD_E_UNKNOWN;
  if ((*it)->owner != owner) return GF_CMD_E_DENIED;
  providers_.erase(it);
  return GF_CMD_OK;
}

auto CommandRegistry::Invoke(const QString& id, QCborMap args,
                             std::vector<gf::cmd::Blob> blobs,
                             const CommandCaller& caller,
                             gf::cmd::CommandContext context,
                             gf::cmd::Completer done) -> Ticket {
  if (!caller.IsHost()) {
    QMutexLocker locker(&mutex_);
    if (closed_.contains(caller.module)) {
      LOG_W() << "module" << caller.module << "may not invoke" << id
              << ": it has been withdrawn";
      return {GF_CMD_E_UNAVAILABLE, 0};
    }
  }

  const auto provider = Lookup(id);
  if (provider == nullptr) return {GF_CMD_E_UNKNOWN, 0};

  if (!caller.IsHost()) {
    if ((provider->flags & gf::cmd::kHostOnly) != 0) {
      return {GF_CMD_E_DENIED, 0};
    }
    if ((caller.caps & provider->required_caps) != provider->required_caps) {
      LOG_W() << "module" << caller.module << "may not invoke" << id
              << ": it lacks a capability the command requires";
      return {GF_CMD_E_DENIED, 0};
    }
  }

  context.caller = caller.module;
  context.caller_caps = caller.caps;
  context.source = caller.source;

  // Enabled-ness is a question about the UI, answerable on its thread only.
  // A call from elsewhere is checked by the command itself when it runs.
  if (provider->state && OnGuiThread()) {
    if ((provider->state(context) & GF_CMD_STATE_ENABLED) == 0) {
      return {GF_CMD_E_DISABLED, 0};
    }
  }

  auto call = std::make_shared<Pending>();
  call->caller = caller.module;
  call->provider = provider->owner;
  call->done = std::move(done);
  {
    QMutexLocker locker(&mutex_);
    call->id = next_call_id_++;
    pending_.emplace(call->id, call);
  }

  const auto call_id = call->id;
  context.call_id = static_cast<qint64>(call_id);
  context.cancelled = [call]() { return call->cancelled.load(); };

  auto completer = [this, call_id,
                    owner = provider->owner](gf::cmd::RawResult r) {
    Finish(call_id, owner, std::move(r));
  };

  auto run = [this, provider, call, context = std::move(context),
              args = std::move(args), blobs = std::move(blobs),
              completer = std::move(completer)]() mutable {
    if (call->cancelled.load()) return;
    // The provider may have been withdrawn while this was queued; it is not
    // called after that, and the caller learns why.
    if (Lookup(provider->id) != provider) {
      Finish(call->id, provider->owner,
             {GF_CMD_E_UNAVAILABLE,
              0,
              {},
              {},
              QStringLiteral("the command's provider went away")});
      return;
    }
    provider->run(context, std::move(args), std::move(blobs),
                  std::move(completer));
  };

  if ((provider->flags & gf::cmd::kNeedsGuiThread) != 0) {
    OnGui(std::move(run));
  } else {
    run();
  }
  return {GF_CMD_OK, call_id};
}

void CommandRegistry::Deliver(const std::shared_ptr<Pending>& call,
                              gf::cmd::RawResult r) {
  const std::lock_guard<QRecursiveMutex> delivery(call->delivery);
  {
    QMutexLocker locker(&mutex_);
    pending_.erase(call->id);
  }
  if (call->cancelled.load() || call->finished) return;
  call->finished = true;
  auto done = std::move(call->done);
  call->done = nullptr;
  r.call_id = static_cast<qint64>(call->id);
  if (done) done(std::move(r));
}

auto CommandRegistry::Finish(quint64 call_id, const QString& provider,
                             gf::cmd::RawResult result) -> int {
  std::shared_ptr<Pending> call;
  {
    QMutexLocker locker(&mutex_);
    const auto it = pending_.find(call_id);
    if (it == pending_.end()) return GF_CMD_E_UNKNOWN;
    if (it->second->provider != provider) return GF_CMD_E_DENIED;
    call = it->second;
  }
  Deliver(call, std::move(result));
  return GF_CMD_OK;
}

auto CommandRegistry::Cancel(quint64 call_id, const QString& caller) -> int {
  std::shared_ptr<Pending> call;
  {
    QMutexLocker locker(&mutex_);
    const auto it = pending_.find(call_id);
    if (it == pending_.end()) return GF_CMD_E_UNKNOWN;
    if (it->second->caller != caller) return GF_CMD_E_DENIED;
    call = it->second;
  }
  call->cancelled.store(true);
  // Waits for a delivery already under way; after this, none can start.
  const std::lock_guard<QRecursiveMutex> delivery(call->delivery);
  call->done = nullptr;
  QMutexLocker locker(&mutex_);
  pending_.erase(call_id);
  return GF_CMD_OK;
}

auto CommandRegistry::IsCancelled(quint64 call_id) -> bool {
  QMutexLocker locker(&mutex_);
  const auto it = pending_.find(call_id);
  // A call that is no longer pending is over, one way or another: a handler
  // asking should stop.
  return it == pending_.end() || it->second->cancelled.load();
}

auto CommandRegistry::Describe(const QString& id) -> std::optional<QCborMap> {
  const auto p = Lookup(id);
  if (p == nullptr) return std::nullopt;
  return p->descriptor;
}

auto CommandRegistry::List(const QString& prefix) -> QStringList {
  QMutexLocker locker(&mutex_);
  QStringList ids;
  for (auto it = providers_.constBegin(); it != providers_.constEnd(); ++it) {
    if (it.key().startsWith(prefix)) ids.append(it.key());
  }
  ids.sort();
  return ids;
}

auto CommandRegistry::ProvidersWithFlag(uint32_t flags) -> QStringList {
  QMutexLocker locker(&mutex_);
  QStringList ids;
  for (auto it = providers_.constBegin(); it != providers_.constEnd(); ++it) {
    if (((*it)->flags & flags) == flags) ids.append(it.key());
  }
  ids.sort();
  return ids;
}

auto CommandRegistry::Contains(const QString& id) -> bool {
  return Lookup(id) != nullptr;
}

auto CommandRegistry::State(const QString& id, const CommandCaller& caller,
                            const gf::cmd::CommandContext& context)
    -> uint32_t {
  const auto p = Lookup(id);
  if (p == nullptr) return 0;
  if (!caller.IsHost()) {
    if ((p->flags & gf::cmd::kHostOnly) != 0) return 0;
    if ((caller.caps & p->required_caps) != p->required_caps) return 0;
  }
  if (!p->state) return GF_CMD_STATE_ENABLED | GF_CMD_STATE_VISIBLE;
  auto ctx = context;
  ctx.caller = caller.module;
  ctx.caller_caps = caller.caps;
  ctx.source = caller.source;
  return p->state(ctx);
}

auto CommandRegistry::PendingCallsOf(const QString& module) -> int {
  QMutexLocker locker(&mutex_);
  int n = 0;
  for (const auto& [id, call] : pending_) {
    if (call->caller == module || call->provider == module) ++n;
  }
  return n;
}

void CommandRegistry::RemoveAllFor(const QString& module) {
  if (module.isEmpty()) return;

  QList<std::shared_ptr<Pending>> made;
  QList<std::shared_ptr<Pending>> provided;
  {
    QMutexLocker locker(&mutex_);
    closed_.insert(module);
    for (auto it = providers_.begin(); it != providers_.end();) {
      if ((*it)->owner == module) {
        it = providers_.erase(it);
      } else {
        ++it;
      }
    }
    for (const auto& [id, call] : pending_) {
      if (call->caller == module) {
        made.append(call);
      } else if (call->provider == module) {
        provided.append(call);
      }
    }
  }

  // Calls the module made: it will never hear back.
  for (const auto& call : made) Cancel(call->id, module);

  // Calls it was serving: their callers are told, rather than left waiting.
  for (const auto& call : provided) {
    Deliver(call, {GF_CMD_E_UNAVAILABLE,
                   0,
                   {},
                   {},
                   QStringLiteral("the module providing the command was "
                                  "unloaded")});
  }
}

void CommandRegistry::Reopen(const QString& module) {
  QMutexLocker locker(&mutex_);
  closed_.remove(module);
}

}  // namespace GpgFrontend::UI
