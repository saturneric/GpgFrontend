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

#include <QFile>
#include <QStandardPaths>
#include <algorithm>

#include "MobileTest.h"
#include "core/function/GlobalSettingStation.h"
#include "core/function/SettingCatalog.h"
#include "files/FileOperations.h"
#include "files/ShareRetentionStore.h"
#include "model/SettingsModel.h"

namespace GpgFrontend::Test::Mobile {

using GpgFrontend::Mobile::SettingsModel;
using GpgFrontend::Mobile::SharedFileStore;

namespace {

auto Keys(const SettingsModel& model) -> QStringList {
  QStringList keys;
  for (int row = 0; row < model.rowCount(); ++row) {
    keys << model.data(model.index(row), SettingsModel::kKeyRole).toString();
  }
  return keys;
}

}  // namespace

TEST(SettingsModelTest, AValueIsReadAndWrittenThroughTheCatalog) {
  RunOnMainThread([]() {
    SettingsModel model;
    const auto row = static_cast<int>(
        Keys(model).indexOf(SettingItems::kExpiringSoonDays.Key()));
    ASSERT_GE(row, 0);
    const auto index = model.index(row);
    EXPECT_EQ(model.data(index, SettingsModel::kTypeRole), "int");
    EXPECT_EQ(model.data(index, SettingsModel::kValueRole),
              SettingItems::kExpiringSoonDays.Read(GetSettings()));

    ASSERT_TRUE(model.setValue(row, 45));
    EXPECT_EQ(SettingItems::kExpiringSoonDays.Read(GetSettings()), 45);
    EXPECT_EQ(model.data(index, SettingsModel::kValueRole), 45);

    // Not a number: refused, nothing written.
    EXPECT_FALSE(model.setValue(row, QVariant::fromValue(QStringList{"x"})));
    EXPECT_EQ(SettingItems::kExpiringSoonDays.Read(GetSettings()), 45);

    ASSERT_TRUE(model.setValue(row, SettingItems::kExpiringSoonDays.Default()));
  });
}

TEST(SettingsModelTest, TheLongestCacheTimeIsNeverTheShorter) {
  RunOnMainThread([]() {
    SettingsModel model;
    const auto keys = Keys(model);
    const auto ttl =
        static_cast<int>(keys.indexOf(SettingItems::kPasswordCacheTtl.Key()));
    const auto max_ttl = static_cast<int>(
        keys.indexOf(SettingItems::kPasswordCacheMaxTtl.Key()));
    ASSERT_GE(ttl, 0);
    ASSERT_GE(max_ttl, 0);

    ASSERT_TRUE(model.setValue(max_ttl, 600));
    ASSERT_TRUE(model.setValue(ttl, 3600));
    // As the engine has it, and the page shows it.
    EXPECT_EQ(SettingItems::kPasswordCacheMaxTtl.Read(GetSettings()), 3600);
    EXPECT_EQ(model.data(model.index(max_ttl), SettingsModel::kValueRole),
              3600);

    model.setValue(ttl, SettingItems::kPasswordCacheTtl.Default());
    model.setValue(max_ttl, SettingItems::kPasswordCacheMaxTtl.Default());
  });
}

TEST(SettingsModelTest, ATimeSaysItsUnitAndOffersItsDefault) {
  RunOnMainThread([]() {
    SettingsModel model;
    const auto keys = Keys(model);

    const auto ttl = model.index(
        static_cast<int>(keys.indexOf(SettingItems::kPasswordCacheTtl.Key())));
    ASSERT_TRUE(ttl.isValid());
    EXPECT_EQ(model.data(ttl, SettingsModel::kUnitRole), "seconds");
    EXPECT_EQ(model.data(ttl, SettingsModel::kSectionRole), "passphrases");
    // A cache time is one of the offered spans, nothing else.
    EXPECT_EQ(model.data(ttl, SettingsModel::kCustomRole), false);
    const auto days = model.index(
        static_cast<int>(keys.indexOf(SettingItems::kExpiringSoonDays.Key())));
    ASSERT_TRUE(days.isValid());
    EXPECT_EQ(model.data(days, SettingsModel::kUnitRole), "days");
    EXPECT_EQ(model.data(days, SettingsModel::kSectionRole), "keys");
    EXPECT_EQ(model.data(days, SettingsModel::kCustomRole), true);

    // A fresh profile's value is one of the choices, never a custom one.
    for (const auto& offer : SettingsModel::Offered()) {
      if (offer.choices.isEmpty()) continue;
      const auto fallback = std::visit(
          [](const auto* setting) {
            return QVariant::fromValue(setting->Default()).toLongLong();
          },
          offer.item);
      EXPECT_TRUE(std::any_of(offer.choices.cbegin(), offer.choices.cend(),
                              [fallback](const QVariant& c) {
                                return c.toLongLong() == fallback;
                              }));
    }
  });
}

TEST(SettingsModelTest, ClearingSharedFilesEmptiesTheStoreAndItsSize) {
  RunOnMainThread([]() {
    auto* store = SharedFileStore();
    ASSERT_NE(store, nullptr);
    const auto staged =
        QStandardPaths::writableLocation(QStandardPaths::CacheLocation) +
        "/settings-test-staged.txt";
    QFile file(staged);
    ASSERT_TRUE(file.open(QIODevice::WriteOnly));
    file.write("shared");
    file.close();
    ASSERT_TRUE(store
                    ->Retain(staged, "shared.txt", true,
                             QDateTime::currentDateTimeUtc())
                    .has_value());

    SettingsModel model;
    EXPECT_GE(model.SharedFileCount(), 1);
    // What the page says the files take is what the store holds.
    qint64 held = 0;
    for (const auto& entry : store->Entries()) held += entry.size;
    EXPECT_GE(held, 6);
    EXPECT_EQ(model.SharedFileSize(), held);
    model.clearSharedFiles();
    EXPECT_EQ(model.SharedFileCount(), 0);
    EXPECT_EQ(model.SharedFileSize(), 0);
    EXPECT_TRUE(store->Entries().isEmpty());
  });
}

}  // namespace GpgFrontend::Test::Mobile
