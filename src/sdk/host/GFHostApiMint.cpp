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
#include <QHash>
#include <QMutex>
#include <QMutexLocker>
#include <QString>

#include "GFSDKBuildInfo.h"
#include "private/GFHostContext.h"
#include "private/GFSDKHandleSweep.h"
#include "private/GFSDKPrivat.h"

/**
 * @file GFSDKHostApiMint.cpp
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
  GFHostApi table{};
};

struct Registry {
  QMutex mutex;
  /// By id, for minting and for the legacy-export check.
  QHash<QString, ContextRecord*> by_id;
  /// By pointer, so a context can be validated WITHOUT being dereferenced.
  QHash<const void*, ContextRecord*> by_context;
};

auto Reg() -> Registry& {
  static Registry registry;
  return registry;
}

auto AsContext(ContextRecord* record) -> GFHostContextRef {
  return reinterpret_cast<GFHostContextRef>(record);
}

/// The live record for @p ctx, or nullptr. Caller holds the mutex.
auto LiveRecordLocked(GFHostContextRef ctx) -> ContextRecord* {
  if (ctx == nullptr) return nullptr;
  auto* record = Reg().by_context.value(static_cast<const void*>(ctx), nullptr);
  if (record == nullptr || !record->live) return nullptr;
  return record;
}

}  // namespace

auto MintHostApi(const char* module_id, uint32_t granted) -> const GFHostApi* {
  if (module_id == nullptr || *module_id == '\0') {
    LOG_W() << "refusing to mint a host api for an unnamed module";
    return nullptr;
  }

  const auto id = QString::fromUtf8(module_id);
  QMutexLocker locker(&Reg().mutex);

  auto* record = Reg().by_id.value(id, nullptr);
  if (record == nullptr) {
    record = new ContextRecord();
    record->module_id = QByteArray(module_id);
    Reg().by_id.insert(id, record);
    Reg().by_context.insert(static_cast<const void*>(AsContext(record)),
                            record);
  }

  // Re-minting an existing module updates the grant in place rather than
  // handing out a second table. A module that is activated twice is holding
  // the first pointer; giving it a different one would leave the first live
  // and unreachable.
  record->granted = granted;
  record->live = true;

  auto& table = record->table;
  table = GFHostApi{};
  table.struct_size = sizeof(GFHostApi);
  table.abi_version = GF_SDK_ABI_VERSION;
  table.granted = granted;
  table.module_id = record->module_id.constData();
  table.context = AsContext(record);
  FillHostApiGroups(table, granted);

  return &table;
}

void ReleaseHostApi(const char* module_id) {
  if (module_id == nullptr || *module_id == '\0') return;

  QMutexLocker locker(&Reg().mutex);
  auto* record = Reg().by_id.value(QString::fromUtf8(module_id), nullptr);
  if (record == nullptr) return;

  // Dead, not gone. See the note at the top of this file.
  record->live = false;
  record->granted = 0;
}

auto ContextStatusOf(GFHostContextRef ctx, uint32_t capability)
    -> HostContextStatus {
  QMutexLocker locker(&Reg().mutex);
  if (ctx == nullptr) return HostContextStatus::kUNKNOWN;

  auto* record = Reg().by_context.value(static_cast<const void*>(ctx), nullptr);
  // Nothing was dereferenced to decide this: the pointer was looked up, not
  // followed. An invented value is simply absent from the table.
  if (record == nullptr) return HostContextStatus::kUNKNOWN;
  if (!record->live) return HostContextStatus::kREVOKED;

  // capability 0 asks only "is this context live", which is the whole
  // question for the always-granted groups.
  if (capability == 0 || (record->granted & capability) != 0) {
    return HostContextStatus::kOK;
  }
  return HostContextStatus::kDENIED;
}

auto ContextHolds(GFHostContextRef ctx, uint32_t capability,
                  const char* entry_point) -> bool {
  const auto status = ContextStatusOf(ctx, capability);
  if (status == HostContextStatus::kOK) return true;

  switch (status) {
    case HostContextStatus::kREVOKED:
      LOG_W() << "refusing" << entry_point << "for module"
              << ContextModuleId(ctx)
              << ": it has been torn down and its grant released. Something "
                 "it left running is still calling.";
      break;
    case HostContextStatus::kDENIED:
      LOG_W() << "refusing" << entry_point << "for module"
              << ContextModuleId(ctx)
              << ": its signed manifest does not declare the capability this "
                 "call needs";
      break;
    default:
      LOG_W() << "refusing" << entry_point
              << ": the caller presented a host context this process does not "
                 "recognise";
      break;
  }
  return false;
}

auto ContextModuleId(GFHostContextRef ctx) -> QString {
  QMutexLocker locker(&Reg().mutex);
  auto* record = LiveRecordLocked(ctx);
  return record == nullptr ? QString() : QString::fromUtf8(record->module_id);
}

auto ContextAttributionId(GFHostContextRef ctx) -> const char* {
  QMutexLocker locker(&Reg().mutex);
  auto* record = LiveRecordLocked(ctx);
  // Safe to hand out beyond the lock: a record is never freed, and its
  // module_id is never reassigned after minting.
  return record == nullptr ? nullptr : record->module_id.constData();
}

}  // namespace gf_sdk_internal
