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

#include "core/function/openpgp/OpenPGPContext.h"

#include <cassert>
#include <condition_variable>
#include <map>
#include <mutex>
#include <optional>
#include <set>

#include "core/function/GFKeyDatabase.h"
#include "core/function/basic/GpgFunctionObject.h"
#include "core/function/openpgp/ChannelHandle.h"
#include "core/function/openpgp/helper/Async.h"
#include "core/function/openpgp/traits/CommonTraits.h"
#include "core/module/ModuleManager.h"
#include "core/utils/GpgUtils.h"
#include "core/utils/MemoryUtils.h"

namespace GpgFrontend {

namespace {

void PublishChannelToRegisterTable(int channel, const QString& id,
                                   const QString& name, const QString& path,
                                   OpenPGPEngine engine) {
  const auto prefix = QString("gpgme.ctx.list.%1").arg(channel);
  Module::UpsertRTValue("core", prefix + ".channel", channel);
  Module::UpsertRTValue("core", prefix + ".database_id", id);
  Module::UpsertRTValue("core", prefix + ".database_name", name);
  Module::UpsertRTValue("core", prefix + ".database_path", path);
  // text, the same "gnupg"/"rpgp" a stored key database carries: the enum
  // itself matched no typed read, so this always came back empty
  Module::UpsertRTValue(
      "core", prefix + ".backend_type",
      QString(engine == OpenPGPEngine::kGNUPG ? "gnupg" : "rpgp"));
}

}  // namespace

namespace {

/// One open key database per directory, shared by every context on it. Two
/// Runtimes both open during a reload must not each hold their own connection
/// to the same files; the shared one is also the single place its writes are
/// serialised.
///
/// An entry whose object is being destroyed stays in the map until its
/// destructor has finished, and a new open of that directory waits for it, so
/// the old connection is always fully closed before a new one opens.
struct KeyDatabaseSessions {
  struct Entry {
    QWeakPointer<GFKeyDatabase> session;
    GFKeyDatabase* raw{nullptr};
  };

  std::mutex mutex;
  std::condition_variable closed;
  std::map<QString, Entry> open;

  /// Locations held for deletion; nothing opens them meanwhile. See
  /// ReserveKeyDatabaseLocationForDeletion().
  std::set<QString> reserved;
};

auto Sessions() -> KeyDatabaseSessions& {
  static KeyDatabaseSessions sessions;
  return sessions;
}

auto AcquireKeyDatabaseSession(const QString& path)
    -> QSharedPointer<GFKeyDatabase> {
  const auto location = KeyDatabasePhysicalLocation(path);
  auto& sessions = Sessions();

  std::unique_lock<std::mutex> lock(sessions.mutex);
  for (;;) {
    auto it = sessions.open.find(location);
    if (it == sessions.open.end()) break;
    if (auto live = it->second.session.toStrongRef(); live != nullptr) {
      return live;
    }
    // Its last holder is gone and its destructor is running: wait until it
    // has closed, rather than opening the same files beside it.
    sessions.closed.wait(lock);
  }

  // Being deleted. Refused under the same lock the reservation was taken
  // under, so no open can slip in between a deletion's checks and its work.
  if (sessions.reserved.count(location) != 0) {
    LOG_W() << "key database location is being deleted, not opening it:"
            << location;
    return nullptr;
  }

  void* mem = SMAMalloc(sizeof(GFKeyDatabase));
  if (mem == nullptr) return nullptr;
  auto* raw = new (mem) GFKeyDatabase();

  QSharedPointer<GFKeyDatabase> session(raw, [location](GFKeyDatabase* ptr) {
    ptr->~GFKeyDatabase();
    SMAFree(ptr);

    auto& sessions = Sessions();
    {
      std::lock_guard<std::mutex> lock(sessions.mutex);
      auto it = sessions.open.find(location);
      if (it != sessions.open.end() && it->second.raw == ptr) {
        sessions.open.erase(it);
      }
    }
    sessions.closed.notify_all();
  });

  // Opened under the lock, so a second context on this directory cannot pick
  // the session up half initialised. Its owner thread takes no lock of ours.
  if (!session->Init(path)) {
    lock.unlock();  // the deleter takes the lock
    return nullptr;
  }

  sessions.open[location] = {session.toWeakRef(), raw};
  return session;
}

}  // namespace

class OpenPGPContext::Impl {
 public:
  Impl(OpenPGPContext* parent, const OpenPGPContextInitArgs& args)
      : parent_(parent),
        args_(args),
        engine_(args.engine),
        key_db_name_(args.db_name),
        key_db_path_(args.db_path) {}

  ~Impl() {
    if (key_db_ != nullptr) {
      key_db_.clear();
    }
  }

  [[nodiscard]] auto KeyDBName() const -> QString { return key_db_name_; }

  [[nodiscard]] auto KeyDBId() const -> QString { return args_.db_id; }

  [[nodiscard]] auto KeyDBPath() const -> QString { return key_db_path_; }

  auto Engine() -> OpenPGPEngine { return engine_; }

  auto KeyDatabase() -> QSharedPointer<GFKeyDatabase> {
    assert(engine_ == OpenPGPEngine::kRPGP);
    return engine_ == OpenPGPEngine::kRPGP ? key_db_ : nullptr;
  }

  auto EngineVersion() -> QString {
    // only check if the operation is routable, without checking if it's fully
    // runnable, to avoid looping call.
    return RunRegisteredRoutableForward<GetEngineVersionOpTag>(*parent_);
  }

  auto CommonInitialize() -> bool {
    bool good = true;
    if (key_db_name_.isEmpty()) {
      LOG_E() << "key database name is empty";
      good = false;
      return false;
    }

    QFileInfo db_path_info(args_.db_path);
    if (!db_path_info.exists() || !db_path_info.isDir()) {
      LOG_E() << "key database path is not exists or is not a directory: "
              << args_.db_path;
      good = false;
      return false;
    }

    key_db_path_ = db_path_info.absoluteFilePath();
    assert(!key_db_path_.isEmpty());

    // Published only once Init() has started its owner thread, so no other
    // thread sees the database half set up. Shared with any other context on
    // the same directory, which has already initialised it.
    auto key_db = AcquireKeyDatabaseSession(key_db_path_);
    good = key_db != nullptr;
    key_db_ = key_db;

    if (!good) {
      LOG_E() << "OpenPGPContext common initialization failed, channel: "
              << parent_->GetChannel() << ", key db name: " << key_db_name_
              << ", key db path: " << key_db_path_
              << ", engine: " << ConvertOpenPGPEngine2String(engine_);
    }

    return good;
  }

  auto Args() -> OpenPGPContextInitArgs { return args_; }

 private:
  OpenPGPContext* parent_;
  OpenPGPContextInitArgs args_{};  ///<

  OpenPGPEngine engine_;

  QString key_db_name_;
  QString key_db_path_;
  QSharedPointer<GFKeyDatabase> key_db_;
};

OpenPGPContext::OpenPGPContext(int channel)
    : SingletonFunctionObject<OpenPGPContext>(channel),
      p_(SecureCreateUniqueObject<Impl>(this, OpenPGPContextInitArgs{})) {}

OpenPGPContext::OpenPGPContext(const OpenPGPContextInitArgs& args, int channel)
    : SingletonFunctionObject<OpenPGPContext>(channel),
      p_(SecureCreateUniqueObject<Impl>(this, args)) {}

auto OpenPGPContext::Good() const -> bool { return good_; }

OpenPGPContext::~OpenPGPContext() = default;

auto OpenPGPContext::KeyDBName() const -> QString { return p_->KeyDBName(); }

auto OpenPGPContext::KeyDBId() const -> QString { return p_->KeyDBId(); }

auto OpenPGPContext::KeyDBPath() const -> QString { return p_->KeyDBPath(); }

auto OpenPGPContext::EngineHomePath() const -> QString { return KeyDBPath(); }

auto OpenPGPContext::Engine() const -> OpenPGPEngine { return p_->Engine(); }

auto OpenPGPContext::EngineVersion() const -> QString {
  return p_->EngineVersion();
}

auto OpenPGPContext::KeyDatabase() -> QSharedPointer<GFKeyDatabase> {
  return p_->KeyDatabase();
}

auto OpenPGPContext::init(const OpenPGPContextInitArgs&) -> bool {
  LOG_D() << "OpenPGPContext init with engine: "
          << ConvertOpenPGPEngine2String(Engine())
          << ", key db name: " << KeyDBName()
          << ", key db path: " << KeyDBPath() << ", channel: " << GetChannel();
  return true;
}

auto OpenPGPContext::Initialize() -> bool {
  auto args = p_->Args();

  // First do the common initialization, which is shared by different engine
  // implementation, then call the engine specific initialization after the
  // common initialization
  good_ = p_->CommonInitialize() && init(args);

  // Recorded either way: a database that failed to open still holds its
  // handle, so the ones after it keep theirs and a caller holding this one is
  // told it is unavailable rather than handed a placeholder.
  RegisterChannelHandle(GetChannel(), args.db_id, good_, args.generation);
  DescribeChannelHandle(
      GetChannel(), args.db_name, args.db_path,
      args.engine == OpenPGPEngine::kGNUPG ? u"gnupg"_s : u"rpgp"_s,
      args.order);

  // Published for the Runtime in use. A context built for a candidate that is
  // not yet published stays out of the register table, so a module never sees
  // a list that is half the old Runtime and half the new one; the whole list is
  // rewritten when the candidate is published.
  if (good_) {
    if (args.generation == CurrentRuntimeGeneration()) {
      PublishChannelToRegisterTable(GetChannel(), args.db_id, args.db_name,
                                    args.db_path, args.engine);
    }
  } else {
    LOG_E() << "OpenPGPContext initialization failed, channel: " << GetChannel()
            << ", key db name: " << KeyDBName()
            << ", key db path: " << KeyDBPath()
            << ", engine: " << ConvertOpenPGPEngine2String(Engine());
  }
  return Good();
}

namespace {

/// The channel handle registry. Process-wide and small: one entry per context
/// ever built, read on every operation that names a channel. Its mutex is the
/// admission lock: leases are granted and handles retired under it, so the two
/// can never interleave.
struct ChannelHandleRegistry {
  std::mutex mutex;
  std::map<int, ChannelHandleInfo> handles;
  std::set<int> reclaim_requested;
  int highest_issued{-1};
  int highest_generation{kInitialRuntimeGeneration};
  int current_generation{kInitialRuntimeGeneration};
  std::optional<int> default_handle;
  std::function<void(int)> reclaimer;
};

auto Registry() -> ChannelHandleRegistry& {
  static ChannelHandleRegistry registry;
  return registry;
}

/// Under the lock: whether @p handle is now due for reclamation, marking it so
/// it is reclaimed only once.
auto TakeReclaimLocked(ChannelHandleRegistry& registry, int handle) -> bool {
  auto it = registry.handles.find(handle);
  if (it == registry.handles.end()) return false;
  if (it->second.state != ChannelHandleState::kRETIRED) return false;
  if (it->second.leases > 0) return false;
  return registry.reclaim_requested.insert(handle).second;
}

void RunReclaimer(const std::function<void(int)>& reclaimer, int handle) {
  if (reclaimer) reclaimer(handle);
}

}  // namespace

struct EngineLease::Token {
  int handle{-1};
  bool operation{false};

  Token(int h, bool op) : handle(h), operation(op) {}
  Token(const Token&) = delete;
  auto operator=(const Token&) -> Token& = delete;

  ~Token() {
    auto& registry = Registry();
    std::function<void(int)> reclaimer;
    bool due = false;
    {
      std::lock_guard<std::mutex> lock(registry.mutex);
      if (auto it = registry.handles.find(handle);
          it != registry.handles.end() && it->second.leases > 0) {
        --it->second.leases;
        if (operation && it->second.operations > 0) --it->second.operations;
      }
      due = TakeReclaimLocked(registry, handle);
      if (due) reclaimer = registry.reclaimer;
    }
    if (due) RunReclaimer(reclaimer, handle);
  }
};

auto EngineLease::Handle() const -> int {
  return token_ == nullptr ? -1 : token_->handle;
}

auto EngineLease::IsHeld() const -> bool { return token_ != nullptr; }

void EngineLease::Release() { token_.reset(); }

void PublishRuntimeToRegisterTable() {
  Module::RemoveRTValue("core", "gpgme.ctx.list");
  for (const auto& info : ListRuntimeChannels()) {
    if (info.state != ChannelHandleState::kAVAILABLE) continue;
    PublishChannelToRegisterTable(
        info.handle, info.database_id, info.name, info.path,
        info.backend_type == "gnupg" ? OpenPGPEngine::kGNUPG
                                     : OpenPGPEngine::kRPGP);
  }
}

auto RegisterChannelHandle(int handle, const QString& database_id,
                           bool available, int generation) -> bool {
  if (handle < 0) return false;

  auto& registry = Registry();
  std::lock_guard<std::mutex> lock(registry.mutex);

  auto& slot = registry.handles[handle];
  if (slot.state != ChannelHandleState::kUNKNOWN) {
    if (slot.state == ChannelHandleState::kRETIRED) {
      LOG_E() << "refusing to reuse a retired channel handle:" << handle;
      return false;
    }
    if (slot.database_id != database_id) {
      LOG_E() << "refusing to give channel handle" << handle
              << "to another database:" << slot.database_id << "->"
              << database_id;
      return false;
    }
  }

  slot.handle = handle;
  slot.database_id = database_id;
  slot.generation = generation;
  slot.state = available ? ChannelHandleState::kAVAILABLE
                         : ChannelHandleState::kUNAVAILABLE;
  registry.highest_issued = std::max(registry.highest_issued, handle);
  registry.highest_generation =
      std::max(registry.highest_generation, generation);
  return true;
}

void DescribeChannelHandle(int handle, const QString& name, const QString& path,
                           const QString& backend_type, int order) {
  auto& registry = Registry();
  std::lock_guard<std::mutex> lock(registry.mutex);
  auto it = registry.handles.find(handle);
  if (it == registry.handles.end()) return;
  it->second.name = name;
  it->second.path = path;
  it->second.backend_type = backend_type;
  it->second.order = order;
}

auto RetireChannelHandle(int handle) -> bool {
  auto& registry = Registry();
  std::function<void(int)> reclaimer;
  bool due = false;
  {
    std::lock_guard<std::mutex> lock(registry.mutex);
    auto it = registry.handles.find(handle);
    if (it == registry.handles.end()) return false;
    it->second.state = ChannelHandleState::kRETIRED;
    due = TakeReclaimLocked(registry, handle);
    if (due) reclaimer = registry.reclaimer;
  }
  if (due) RunReclaimer(reclaimer, handle);
  return true;
}

auto LookupChannelHandle(int handle) -> ChannelHandleInfo {
  auto& registry = Registry();
  std::lock_guard<std::mutex> lock(registry.mutex);

  auto it = registry.handles.find(handle);
  if (it == registry.handles.end()) {
    ChannelHandleInfo unknown;
    unknown.handle = handle;
    return unknown;
  }
  return it->second;
}

auto IsLiveChannelHandle(int handle) -> bool {
  return LookupChannelHandle(handle).state == ChannelHandleState::kAVAILABLE;
}

auto IsRetiredChannelHandle(int handle) -> bool {
  return LookupChannelHandle(handle).state == ChannelHandleState::kRETIRED;
}

auto AllocateChannelHandle() -> int {
  auto& registry = Registry();
  std::lock_guard<std::mutex> lock(registry.mutex);
  return ++registry.highest_issued;
}

void SetDefaultChannelHandle(int handle) {
  auto& registry = Registry();
  std::lock_guard<std::mutex> lock(registry.mutex);
  registry.default_handle = handle;
}

auto DefaultChannelHandle() -> int {
  auto& registry = Registry();
  std::lock_guard<std::mutex> lock(registry.mutex);
  return registry.default_handle.value_or(kGpgFrontendDefaultChannel);
}

auto UsableDefaultChannelHandle() -> int {
  const auto preferred = DefaultChannelHandle();
  if (IsLiveChannelHandle(preferred)) return preferred;
  for (const auto& info : ListRuntimeChannels()) {
    if (info.state == ChannelHandleState::kAVAILABLE) return info.handle;
  }
  return preferred;
}

auto TryAcquireEngineLease(int handle, EngineAccessError* why, bool operation)
    -> std::optional<EngineLease> {
  auto& registry = Registry();
  std::lock_guard<std::mutex> lock(registry.mutex);

  auto it = registry.handles.find(handle);
  const auto state = it == registry.handles.end() ? ChannelHandleState::kUNKNOWN
                                                  : it->second.state;
  if (state != ChannelHandleState::kAVAILABLE) {
    if (why != nullptr) {
      *why = state == ChannelHandleState::kRETIRED ? EngineAccessError::kRETIRED
             : state == ChannelHandleState::kUNAVAILABLE
                 ? EngineAccessError::kUNAVAILABLE
                 : EngineAccessError::kUNKNOWN;
    }
    return {};
  }

  ++it->second.leases;
  if (operation) ++it->second.operations;
  EngineLease lease;
  lease.token_ = std::make_shared<EngineLease::Token>(handle, operation);
  return lease;
}

auto SetRetiredChannelReclaimer(std::function<void(int)> reclaimer)
    -> std::function<void(int)> {
  auto& registry = Registry();
  std::lock_guard<std::mutex> lock(registry.mutex);
  std::swap(registry.reclaimer, reclaimer);
  return reclaimer;
}

auto CurrentRuntimeGeneration() -> int {
  auto& registry = Registry();
  std::lock_guard<std::mutex> lock(registry.mutex);
  return registry.current_generation;
}

auto AllocateRuntimeGeneration() -> int {
  auto& registry = Registry();
  std::lock_guard<std::mutex> lock(registry.mutex);
  return ++registry.highest_generation;
}

namespace {

/// Under the lock: retire every handle of @p generation, collecting the ones
/// already due for reclamation into @p due.
auto RetireGenerationLocked(ChannelHandleRegistry& registry, int generation,
                            QList<int>& due) -> QList<int> {
  QList<int> retired;
  for (auto& [handle, info] : registry.handles) {
    if (info.generation != generation) continue;
    if (info.state == ChannelHandleState::kRETIRED) continue;
    info.state = ChannelHandleState::kRETIRED;
    retired.push_back(handle);
  }
  for (const auto handle : retired) {
    if (TakeReclaimLocked(registry, handle)) due.push_back(handle);
  }
  return retired;
}

}  // namespace

auto RetireRuntimeGeneration(int generation) -> QList<int> {
  auto& registry = Registry();
  QList<int> retired;
  QList<int> due;
  std::function<void(int)> reclaimer;
  {
    std::lock_guard<std::mutex> lock(registry.mutex);
    retired = RetireGenerationLocked(registry, generation, due);
    reclaimer = registry.reclaimer;
  }
  for (const auto handle : due) RunReclaimer(reclaimer, handle);
  return retired;
}

auto PublishRuntimeGeneration(int generation, int default_handle)
    -> QList<int> {
  auto& registry = Registry();
  QList<int> retired;
  QList<int> due;
  std::function<void(int)> reclaimer;
  {
    std::lock_guard<std::mutex> lock(registry.mutex);
    const auto previous = registry.current_generation;
    if (previous != generation) {
      retired = RetireGenerationLocked(registry, previous, due);
    }
    registry.current_generation = generation;
    registry.highest_generation =
        std::max(registry.highest_generation, generation);
    registry.default_handle = default_handle;
    reclaimer = registry.reclaimer;
  }
  for (const auto handle : due) RunReclaimer(reclaimer, handle);
  return retired;
}

auto ListRuntimeChannels(int generation) -> QList<ChannelHandleInfo> {
  auto& registry = Registry();
  QList<ChannelHandleInfo> out;
  {
    std::lock_guard<std::mutex> lock(registry.mutex);
    const auto wanted =
        generation < 0 ? registry.current_generation : generation;
    for (const auto& [handle, info] : registry.handles) {
      if (info.generation == wanted) out.push_back(info);
    }
  }
  std::stable_sort(out.begin(), out.end(),
                   [](const ChannelHandleInfo& a, const ChannelHandleInfo& b) {
                     return a.order < b.order;
                   });
  return out;
}

auto ResolveDatabaseHandle(const QString& database_id) -> int {
  auto& registry = Registry();
  std::lock_guard<std::mutex> lock(registry.mutex);

  if (database_id.isEmpty()) {
    const auto handle =
        registry.default_handle.value_or(kGpgFrontendDefaultChannel);
    auto it = registry.handles.find(handle);
    return it != registry.handles.end() &&
                   it->second.state == ChannelHandleState::kAVAILABLE
               ? handle
               : -1;
  }

  for (const auto& [handle, info] : registry.handles) {
    if (info.generation == registry.current_generation &&
        info.database_id == database_id &&
        info.state == ChannelHandleState::kAVAILABLE) {
      return handle;
    }
  }
  return -1;
}

void MarkChannelReclaimed(int handle) {
  auto& registry = Registry();
  std::lock_guard<std::mutex> lock(registry.mutex);
  auto it = registry.handles.find(handle);
  if (it != registry.handles.end()) it->second.reclaimed = true;
}

auto ReclaimChannelAgainIfReclaimed(int handle) -> bool {
  auto& registry = Registry();
  std::function<void(int)> reclaimer;
  {
    std::lock_guard<std::mutex> lock(registry.mutex);
    auto it = registry.handles.find(handle);
    if (it == registry.handles.end() || !it->second.reclaimed) return false;
    it->second.reclaimed = false;
    registry.reclaim_requested.erase(handle);
    if (!TakeReclaimLocked(registry, handle)) return false;
    reclaimer = registry.reclaimer;
  }
  RunReclaimer(reclaimer, handle);
  return true;
}

auto RetireChannelsAtLocation(const QString& path) -> QList<int> {
  const auto location = KeyDatabasePhysicalLocation(path);
  if (location.isEmpty()) return {};

  QList<int> candidates;
  {
    auto& registry = Registry();
    std::lock_guard<std::mutex> lock(registry.mutex);
    for (const auto& [handle, info] : registry.handles) {
      if (info.state == ChannelHandleState::kRETIRED) continue;
      if (KeyDatabasePhysicalLocation(info.path) == location) {
        candidates.push_back(handle);
      }
    }
  }

  QList<int> retired;
  for (const auto handle : candidates) {
    if (RetireChannelHandle(handle)) retired.push_back(handle);
  }
  return retired;
}

auto IsKeyDatabaseLocationInUse(const QString& path) -> bool {
  const auto location = KeyDatabasePhysicalLocation(path);
  if (location.isEmpty()) return false;

  {
    auto& registry = Registry();
    std::lock_guard<std::mutex> lock(registry.mutex);
    for (const auto& [handle, info] : registry.handles) {
      if (info.reclaimed || info.state == ChannelHandleState::kUNKNOWN) {
        continue;
      }
      // A handle that failed to open holds nothing, unless it is retired and
      // simply has not been swept yet.
      if (info.state == ChannelHandleState::kUNAVAILABLE) continue;
      if (KeyDatabasePhysicalLocation(info.path) == location) return true;
    }
  }

  auto& sessions = Sessions();
  std::lock_guard<std::mutex> lock(sessions.mutex);
  return sessions.open.find(location) != sessions.open.end();
}

KeyDatabaseDeletionReservation::KeyDatabaseDeletionReservation(QString location)
    : location_(std::move(location)) {}

KeyDatabaseDeletionReservation::~KeyDatabaseDeletionReservation() {
  auto& sessions = Sessions();
  std::lock_guard<std::mutex> lock(sessions.mutex);
  sessions.reserved.erase(location_);
}

auto ReserveKeyDatabaseLocationForDeletion(const QString& path)
    -> std::unique_ptr<KeyDatabaseDeletionReservation> {
  const auto location = KeyDatabasePhysicalLocation(path);
  if (location.isEmpty()) return nullptr;

  std::unique_ptr<KeyDatabaseDeletionReservation> reservation;
  {
    auto& sessions = Sessions();
    std::lock_guard<std::mutex> lock(sessions.mutex);
    if (sessions.reserved.count(location) != 0) return nullptr;
    if (sessions.open.find(location) != sessions.open.end()) return nullptr;
    sessions.reserved.insert(location);
    reservation.reset(new KeyDatabaseDeletionReservation(location));
  }

  // Taken first and checked after: from the moment it is held no session can
  // open there, so a handle still on the location is one opened before, and
  // it is seen here. Dropping the reservation releases it again.
  if (IsKeyDatabaseLocationInUse(path)) return nullptr;
  return reservation;
}

}  // namespace GpgFrontend