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
#include "core/utils/BuildInfoUtils.h"
#include "files/FileOperations.h"
#include "files/ShareRetentionStore.h"
#include "model/AboutInfo.h"
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

// The operations besides the four are offered once turned on, as on the
// desktop's toolbar; every page's model hears of it.
TEST(SettingsModelTest, ExtraOperationsAreOffOnlyUntilTurnedOn) {
  RunOnMainThread([]() {
    SettingsModel settings_page;
    SettingsModel text_page;
    int heard = 0;
    QObject::connect(&text_page, &SettingsModel::SignalOperationsChanged,
                     [&heard]() { ++heard; });
    EXPECT_TRUE(SettingsModel::ExtraOperations().isEmpty());

    const auto keys = Keys(settings_page);
    const auto row = [&keys](const SettingMeta& meta) {
      return static_cast<int>(keys.indexOf(QString::fromLatin1(meta.key)));
    };
    for (const auto* meta :
         {static_cast<const SettingMeta*>(&SettingItems::kShowEncryptSign),
          static_cast<const SettingMeta*>(&SettingItems::kShowDecryptVerify),
          static_cast<const SettingMeta*>(
              &SettingItems::kShowSymmetricEncrypt)}) {
      const auto at = row(*meta);
      ASSERT_GE(at, 0) << meta->key;
      const auto index = settings_page.index(at);
      EXPECT_EQ(settings_page.data(index, SettingsModel::kTypeRole), "bool");
      EXPECT_EQ(settings_page.data(index, SettingsModel::kSectionRole),
                "operations");
      EXPECT_EQ(settings_page.data(index, SettingsModel::kValueRole), false);
    }

    ASSERT_TRUE(
        settings_page.setValue(row(SettingItems::kShowEncryptSign), true));
    ASSERT_TRUE(
        settings_page.setValue(row(SettingItems::kShowSymmetricEncrypt), true));
    EXPECT_EQ(heard, 2);
    EXPECT_EQ(text_page.ExtraOperations(),
              (QStringList{"encrypt_sign", "symmetric"}));

    settings_page.setValue(row(SettingItems::kShowEncryptSign), false);
    settings_page.setValue(row(SettingItems::kShowSymmetricEncrypt), false);
    EXPECT_TRUE(SettingsModel::ExtraOperations().isEmpty());
  });
}

// What the Encrypt and Decrypt buttons do, as the desktop's toolbar
// defaults: chosen among what is turned on, and never one turned off.
TEST(SettingsModelTest, AButtonDoesWhatIsChosenAmongWhatIsOffered) {
  RunOnMainThread([]() {
    SettingsModel model;
    const auto keys = Keys(model);
    const auto row = [&keys](const SettingMeta& meta) {
      return static_cast<int>(keys.indexOf(QString::fromLatin1(meta.key)));
    };
    const auto encrypt_row = row(SettingItems::kDefaultEncryptOperation);
    const auto decrypt_row = row(SettingItems::kDefaultDecryptOperation);
    ASSERT_GE(encrypt_row, 0);
    ASSERT_GE(decrypt_row, 0);
    const auto choices = [&model](int at) {
      return model.data(model.index(at), SettingsModel::kChoicesRole)
          .toStringList();
    };

    // Nothing else turned on: nothing to choose, the plain ones.
    EXPECT_EQ(choices(encrypt_row), QStringList{"encrypt"});
    EXPECT_EQ(SettingsModel::DefaultEncrypt(), "encrypt");
    EXPECT_EQ(SettingsModel::DefaultDecrypt(), "decrypt");
    EXPECT_FALSE(model.setValue(encrypt_row, "encrypt_sign"));

    // Turned on, offered and chosen.
    ASSERT_TRUE(model.setValue(row(SettingItems::kShowEncryptSign), true));
    ASSERT_TRUE(model.setValue(row(SettingItems::kShowDecryptVerify), true));
    EXPECT_EQ(choices(encrypt_row), (QStringList{"encrypt", "encrypt_sign"}));
    EXPECT_EQ(choices(decrypt_row), (QStringList{"decrypt", "decrypt_verify"}));
    ASSERT_TRUE(model.setValue(encrypt_row, "encrypt_sign"));
    ASSERT_TRUE(model.setValue(decrypt_row, "decrypt_verify"));
    EXPECT_EQ(SettingsModel::DefaultEncrypt(), "encrypt_sign");
    EXPECT_EQ(SettingsModel::DefaultDecrypt(), "decrypt_verify");
    EXPECT_EQ(model.data(model.index(encrypt_row), SettingsModel::kValueRole),
              "encrypt_sign");
    // Not one of them: refused.
    EXPECT_FALSE(model.setValue(encrypt_row, "sign"));

    // Turned off again: the button does the plain one.
    model.setValue(row(SettingItems::kShowEncryptSign), false);
    model.setValue(row(SettingItems::kShowDecryptVerify), false);
    EXPECT_EQ(SettingsModel::DefaultEncrypt(), "encrypt");
    EXPECT_EQ(SettingsModel::DefaultDecrypt(), "decrypt");
    EXPECT_EQ(model.data(model.index(encrypt_row), SettingsModel::kValueRole),
              "encrypt");
    model.setValue(encrypt_row, "encrypt");
    model.setValue(decrypt_row, "decrypt");
  });
}

// The About pages: what the desktop's About dialog tells of the build, the
// engine and the translators.
TEST(SettingsModelTest, AboutTellsTheBuildTheEngineAndTheTranslators) {
  RunOnMainThread([]() {
    GpgFrontend::Mobile::AboutInfo about;
    QMap<QString, QString> build;
    for (const auto& row : about.build()) {
      build.insert(row.toMap().value("label").toString(),
                   row.toMap().value("value").toString());
    }
    EXPECT_EQ(build.value("GpgFrontend"), GetProjectVersion());
    EXPECT_FALSE(build.value("Qt").isEmpty());
    // The desktop's captions, without their colon.
    EXPECT_TRUE(build.contains("Git Commit"));
    // The commit shown short, as git abbreviates it, and kept whole.
    for (const auto& row : about.build()) {
      if (row.toMap().value("label") != "Git Commit") continue;
      EXPECT_EQ(row.toMap().value("full"), GetProjectBuildGitCommitHash());
      EXPECT_EQ(row.toMap().value("value").toString().size(),
                std::min<qsizetype>(12, GetProjectBuildGitCommitHash().size()));
    }
    // Nothing without a value: a phone links no GnuPG library.
    for (const auto& value : build) EXPECT_FALSE(value.isEmpty());
#if !GF_ENGINE_GNUPG
    EXPECT_FALSE(build.contains("GPGME"));
#endif

    const auto engine = about.engine();
    ASSERT_FALSE(engine.isEmpty());
    EXPECT_EQ(engine.first().toMap().value("label"), "Engine version");
    EXPECT_FALSE(engine.first().toMap().value("value").toString().isEmpty());
    EXPECT_FALSE(about.engineDependencies().isEmpty());

    EXPECT_FALSE(about.translators().isEmpty());

    // How the Android package was built, only there.
#ifdef Q_OS_ANDROID
    EXPECT_FALSE(about.androidBuild().isEmpty());
#else
    EXPECT_TRUE(about.androidBuild().isEmpty());
#endif
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
