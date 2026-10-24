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

#include "screenshot/ScreenshotSeed.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QUuid>

#include "core/function/openpgp/AbstractKeyRepository.h"
#include "core/function/openpgp/KeyCategoryRepository.h"
#include "core/function/openpgp/KeyGroupRepository.h"
#include "core/function/openpgp/KeyImportExportOperation.h"
#include "core/model/GFBuffer.h"
#include "core/model/GpgAbstractKey.h"
#include "core/model/GpgImportInformation.h"
#include "core/model/GpgKeyGroup.h"
#include "core/model/KeyDatabaseInfo.h"
#include "core/model/SettingsObject.h"
#include "core/struct/settings_object/KeyDatabaseListSO.h"
#include "core/utils/GpgUtils.h"

namespace GpgFrontend::Screenshot {

namespace {

auto FindKey(int channel, const QList<DemoKey>& keys, const QString& name)
    -> GpgAbstractKeyPtr {
  for (const auto& key : keys) {
    if (key.name == name) {
      return AbstractKeyRepository::GetInstance(channel).GetKey(
          key.fingerprint);
    }
  }
  return nullptr;
}

}  // namespace

auto ScreenshotDatabaseId(const QString& demo_id) -> QString {
  // The settings page lists database ids, which are UUIDs everywhere else; a
  // name-based one is shaped the same and comes out the same every run.
  static const QUuid kNamespace(u"6ba7b811-9dad-11d1-80b4-00c04fd430c8"_s);
  return QUuid::createUuidV5(kNamespace, u"gpgfrontend-screenshot:"_s + demo_id)
      .toString(QUuid::WithoutBraces);
}

auto ScreenshotDatabaseDir(const DemoDatabase& db) -> QString {
  return u"dbs/"_s + db.name;
}

auto BuildKeyDatabaseList(const QList<DemoDatabase>& requested) -> QJsonObject {
  KeyDatabaseListSO list;

  // Reconciliation points this entry at the profile's own keyring whatever
  // path is stored; the path is written the way a self-contained profile
  // stores it anyway.
  KeyDatabaseItemSO builtin;
  builtin.id = QLatin1StringView(kDefaultKeyDatabaseId);
  builtin.name = QLatin1StringView(kDefaultKeyDatabaseName);
  builtin.path = QString::fromLatin1(kProfilePathToken) + u"/db"_s;
  builtin.channel = 0;
  builtin.backend_type = u"gnupg"_s;
  builtin.kind = KeyDatabaseKind::kDEFAULT;
  list.key_databases.append(builtin);

  int channel = 1;
  for (const auto& db : requested) {
    KeyDatabaseItemSO item;
    item.id = ScreenshotDatabaseId(db.id);
    item.name = db.name;
    item.path = QString::fromLatin1(kProfilePathToken) + QLatin1Char('/') +
                ScreenshotDatabaseDir(db);
    item.channel = channel++;
    item.backend_type = db.backend;
    item.kind = KeyDatabaseKind::kMANAGED;
    list.key_databases.append(item);
  }

  list.default_database_id = QLatin1StringView(kDefaultKeyDatabaseId);
  return list.ToJson();
}

auto StoreKeyDatabaseList(const QList<DemoDatabase>& requested) -> bool {
  auto so = SettingsObject(kKeyDatabaseListObject);
  return so.Store(BuildKeyDatabaseList(requested));
}

auto ImportDemoKeys(int channel, const QString& directory,
                    const DemoDatabase& db) -> QString {
  for (const auto& file_name : db.imports) {
    QFile file(QDir(directory).filePath(file_name));
    if (!file.open(QIODevice::ReadOnly)) return u"unreadable:"_s + file_name;

    const auto info = KeyImportExportOperation::GetInstance(channel).ImportKey(
        GFBuffer(file.readAll()));
    if (info == nullptr || info->imported == 0) {
      return u"not-imported:"_s + file_name;
    }
  }

  auto& repo = AbstractKeyRepository::GetInstance(channel);
  repo.FlushCache();
  for (const auto& key : db.keys) {
    if (repo.GetKey(key.fingerprint) == nullptr) {
      return u"missing-after-import:"_s + key.name;
    }
  }
  return {};
}

auto SeedGroupsAndCategories(int channel, const DemoData& demo,
                             const QDateTime& created) -> QString {
  const auto alice = FindKey(channel, demo.keys, u"Alice"_s);
  const auto bob = FindKey(channel, demo.keys, u"Bob"_s);
  const auto erin = FindKey(channel, demo.keys, u"Erin"_s);
  if (alice == nullptr || bob == nullptr || erin == nullptr) {
    return u"demo-key-missing"_s;
  }

  KeyGroupCO group;
  group.id = QLatin1StringView(kDemoGroupId);
  group.name = QLatin1StringView(kDemoGroupName);
  group.email = u"team@example.com"_s;
  group.comment = u"Alice and Bob"_s;
  group.creation_time = created;

  auto& groups = KeyGroupRepository::GetInstance(channel);
  groups.AddKeyGroup(GpgKeyGroup(group));
  if (!groups.AddKey2KeyGroup(group.id, alice) ||
      !groups.AddKey2KeyGroup(group.id, bob)) {
    return u"group-members-refused"_s;
  }

  auto& categories = KeyCategoryRepository::GetInstance(channel);
  const auto category =
      categories.AddCategory(QLatin1StringView(kDemoCategoryName),
                             QLatin1StringView(kDemoCategoryColor));
  if (category.isEmpty() || !categories.AddKey2Category(category, bob->ID()) ||
      !categories.AddKey2Category(category, erin->ID())) {
    return u"category-refused"_s;
  }

  AbstractKeyRepository::GetInstance(channel).FlushCache();
  return {};
}

}  // namespace GpgFrontend::Screenshot
