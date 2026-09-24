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

#include <QByteArray>
#include <QDeadlineTimer>
#include <QHash>
#include <QMutex>
#include <QMutexLocker>
#include <QString>
#include <QWaitCondition>

#include "GFSDKBuildInfo.h"
#include "core/module/ModuleCapability.h"
#include "private/GFHostContext.h"
#include "private/GFSDKHandleSweep.h"
#include "private/GFSDKPrivate.h"

/**
 * @file GFHostApiMint.cpp
 * @brief One grant per module, and the check every primitive makes on it.
 *
 * ## Why the record outlives the module
 *
 * A context record is never freed. It is marked dead at unload and left
 * allocated for the life of the process.
 *
 * That is not laziness, it is the only safe answer. The module holds a
 * `GFHostApi*` that points INTO the record, and it may be running code on a
 * thread it failed to stop -- the very case this whole mechanism exists to
 * make safe. Freeing the record would turn a refusable call into a read of
 * freed memory, which is a worse failure than the one being prevented. The
 * cost is bounded by how many distinct modules a process ever loads, which is
 * a handful, and each record is a few dozen bytes.
 *
 * The same reasoning is already the house style: GFSDKHandleRegistry.h decides
 * a handle's validity by looking its POINTER up in a table rather than
 * dereferencing possibly-freed memory to read a magic word.
 */

namespace gf_sdk_internal {

namespace {

struct ContextRecord {
  /// Owned and stable for the life of the process, which is what lets
  /// ScopedContextAttribution point at it without copying.
  QByteArray module_id;
  uint32_t granted = 0;
  bool live = false;
  /// Gated calls currently running with this context. Guarded by the mutex.
  int in_flight = 0;
  GFHostApi table{};
};

struct Registry {
  QMutex mutex;
  /// Signalled whenever a gated call ends, for WaitHostApiIdle().
  QWaitCondition call_ended;
  /// The current record for each module id.
  QHash<QString, ContextRecord*> by_id;
  /// Every record ever minted, by pointer, so a context can be validated
  /// WITHOUT being dereferenced. Records are never removed.
  QHash<const void*, ContextRecord*> by_context;
};

auto Reg() -> Registry& {
  static Registry registry;
  return registry;
}

/// How many gated calls THIS thread is inside, per record. A thread that
/// waits for a module to go idle must not wait for its own calls.
auto HeldByThisThread() -> QHash<const void*, int>& {
  thread_local QHash<const void*, int> held;
  return held;
}

auto AsContext(ContextRecord* record) -> GFHostContextRef {
  return reinterpret_cast<GFHostContextRef>(record);
}

/// The status of @p ctx and, when it is known, its record. Caller holds the
/// mutex. Nothing is dereferenced to decide this: the pointer is looked up,
/// not followed, so an invented value is simply absent from the table.
auto StatusLocked(GFHostContextRef ctx, uint32_t capability,
                  ContextRecord** out) -> HostContextStatus {
  *out = nullptr;
  if (ctx == nullptr) return HostContextStatus::kUNKNOWN;

  auto* record = Reg().by_context.value(static_cast<const void*>(ctx), nullptr);
  if (record == nullptr) return HostContextStatus::kUNKNOWN;
  *out = record;
  if (!record->live) return HostContextStatus::kREVOKED;

  // capability 0 asks only "is this context live", which is the whole
  // question for the always-granted groups. Otherwise EVERY requested bit
  // must be granted: an entry point that needs two capabilities is refused
  // to a module holding only one of them.
  if (capability == 0 || (record->granted & capability) == capability) {
    return HostContextStatus::kOK;
  }
  return HostContextStatus::kDENIED;
}

void LogRefusal(HostContextStatus status, GFHostContextRef ctx,
                uint32_t capability, uint32_t granted,
                const char* entry_point) {
  switch (status) {
    case HostContextStatus::kREVOKED:
      LOG_W() << "refusing" << entry_point << "for module"
              << ContextModuleId(ctx)
              << ": the module has been unloaded, but a thread it started is "
                 "still calling the host";
      break;
    case HostContextStatus::kDENIED:
      LOG_W() << "refusing" << entry_point << "for module"
              << ContextModuleId(ctx)
              << ": its signed manifest does not declare"
              << GpgFrontend::Module::ModuleCapabilityMaskToString(capability &
                                                                   ~granted);
      break;
    default:
      LOG_W() << "refusing" << entry_point
              << ": the caller presented an unknown or forged host context";
      break;
  }
}

auto FillTable(ContextRecord* record) -> const GFHostApi* {
  auto& table = record->table;
  table = GFHostApi{};
  table.struct_size = sizeof(GFHostApi);
  table.abi_version = GF_SDK_ABI_VERSION;
  table.granted = record->granted;
  table.module_id = record->module_id.constData();
  table.context = AsContext(record);
  FillHostApiGroups(table, record->granted);
  return &table;
}

}  // namespace

auto MintHostApi(const char* module_id, uint32_t granted) -> const GFHostApi* {
  if (module_id == nullptr || *module_id == '\0') {
    LOG_W() << "refusing to mint a host api for an unnamed module";
    return nullptr;
  }

  const auto id = QString::fromUtf8(module_id);
  QMutexLocker locker(&Reg().mutex);

  // Minting a module that is still live with the same grant returns the table
  // it already holds: handing it a second one would leave the first live and
  // unreachable.
  auto* current = Reg().by_id.value(id, nullptr);
  if (current != nullptr && current->live && current->granted == granted) {
    return &current->table;
  }

  // Anything else gets a NEW record. A table is never rewritten once handed
  // out, because other threads read it without this lock; and a thread left
  // over from a previous load keeps presenting the old context, which stays
  // revoked instead of being revived by the reload.
  if (current != nullptr) current->live = false;

  auto* record = new ContextRecord();
  record->module_id = QByteArray(module_id);
  record->granted = granted;
  record->live = true;
  Reg().by_id.insert(id, record);
  Reg().by_context.insert(static_cast<const void*>(AsContext(record)), record);
  return FillTable(record);
}

void ReleaseHostApi(const char* module_id) {
  if (module_id == nullptr || *module_id == '\0') return;

  QMutexLocker locker(&Reg().mutex);
  auto* record = Reg().by_id.value(QString::fromUtf8(module_id), nullptr);
  if (record == nullptr) return;

  // Dead, not gone. See the note at the top of this file. The table is left
  // exactly as it was, so a stale thread still finds every group it was
  // given and is refused at the gate instead of reading a NULL.
  record->live = false;
}

auto WaitHostApiIdle(const char* module_id, int timeout_ms) -> bool {
  if (module_id == nullptr || *module_id == '\0') return true;
  const QByteArray id(module_id);
  QDeadlineTimer deadline(timeout_ms);

  QMutexLocker locker(&Reg().mutex);
  const auto& held = HeldByThisThread();
  for (;;) {
    int running = 0;
    for (auto* record : std::as_const(Reg().by_context)) {
      if (record->module_id != id) continue;
      running += record->in_flight - held.value(record, 0);
    }
    if (running <= 0) return true;
    if (!Reg().call_ended.wait(&Reg().mutex, deadline)) return false;
  }
}

auto BeginCall(GFHostContextRef ctx, uint32_t capability,
               const char* entry_point) -> CallTicket {
  HostContextStatus status;
  uint32_t granted = 0;
  {
    QMutexLocker locker(&Reg().mutex);
    ContextRecord* record = nullptr;
    status = StatusLocked(ctx, capability, &record);
    if (record != nullptr) granted = record->granted;
    if (status == HostContextStatus::kOK) {
      // Counted and attributed under the same lock that authorized it, so a
      // release cannot land between the check and the call being recorded.
      record->in_flight++;
      HeldByThisThread()[record]++;
      return CallTicket{record, record->module_id.constData()};
    }
  }
  LogRefusal(status, ctx, capability, granted, entry_point);
  return {};
}

void EndCall(const CallTicket& ticket) {
  if (ticket.record == nullptr) return;
  QMutexLocker locker(&Reg().mutex);
  auto* record = static_cast<ContextRecord*>(const_cast<void*>(ticket.record));
  record->in_flight--;
  auto& held = HeldByThisThread();
  if (--held[record] <= 0) held.remove(record);
  Reg().call_ended.wakeAll();
}

auto ContextModuleId(GFHostContextRef ctx) -> QString {
  QMutexLocker locker(&Reg().mutex);
  // Any record, live or dead: this names a module in a log line, and the
  // message that needs the name most is the one about an unloaded module
  // still calling. Reading a dead record is safe because none is ever freed.
  ContextRecord* record = nullptr;
  StatusLocked(ctx, 0, &record);
  return record == nullptr ? QString() : QString::fromUtf8(record->module_id);
}

auto ContextGranted(GFHostContextRef ctx) -> uint32_t {
  QMutexLocker locker(&Reg().mutex);
  ContextRecord* record = nullptr;
  if (StatusLocked(ctx, 0, &record) != HostContextStatus::kOK) return 0;
  return record->granted;
}

}  // namespace gf_sdk_internal
