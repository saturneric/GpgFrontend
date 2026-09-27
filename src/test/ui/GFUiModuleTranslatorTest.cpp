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

#include <gtest/gtest.h>

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <array>

#include "GpgFrontendTest.h"
#include "core/module/ModuleManager.h"
#include "ui/UIModuleManager.h"
#include "ui/command/CommandRegistry.h"
#include "ui/lua/NativeWidgetRegistry.h"

namespace GpgFrontend::Test {

// Module translators are reinstalled on every interface language switch and
// again on every restart-loop pass. QTranslator::load(const uchar*, int) does
// not copy its input, so a round that forgets its predecessors leaves them
// installed on qApp reading QM bytes it has just released -- which is what took
// the process down mid-restart after a language change in the wizard.
//
// These run the whole exercise on the main thread: installTranslator() and
// parenting a QTranslator to qApp are not worker-thread business.

namespace {

// A string the key server sync module translates in its own context, so the
// answer can only come from a module translator, never from the application's
// own.
constexpr auto kProbeContext = "ModuleKeyServerSync";
constexpr auto kProbeSource = "Public Key Upload Successful";

struct Round {
  QContainer<QPointer<QTranslator>> installed;
  QString probe;
};

// Modules are loaded and registered on a worker task that races the test
// suite, and a module only hands over its translator data reader while
// registering. Without this the whole exercise runs against an empty set.
auto WaitForModules() -> bool {
  return WaitFor(
      []() -> bool {
        return Module::ModuleManager::GetInstance().IsAllModulesRegistered();
      },
      10000);
}

auto Reinstall() -> Round {
  Round round;
  RunOnMainThread([&round]() {
    auto& manager = UI::UIModuleManager::GetInstance();
    manager.RegisterAllModuleTranslators();
    round.installed = manager.InstalledTranslators();
    round.probe = QCoreApplication::translate(kProbeContext, kProbeSource);
  });
  return round;
}

void SetLocale(const QLocale& locale) {
  RunOnMainThread([&locale]() { QLocale::setDefault(locale); });
}

/**
 * @brief Switch the process to a locale the modules actually ship QM data for,
 * and put the previous one back afterwards.
 *
 * The default test locale has no module translations, so a reinstall under it
 * installs nothing and there is no lifetime to test.
 */
class ScopedTranslatedLocale {
 public:
  ScopedTranslatedLocale() { SetLocale(QLocale("zh_CN")); }

  ~ScopedTranslatedLocale() {
    SetLocale(original_);
    Reinstall();
  }

  ScopedTranslatedLocale(const ScopedTranslatedLocale&) = delete;
  auto operator=(const ScopedTranslatedLocale&)
      -> ScopedTranslatedLocale& = delete;

 private:
  QLocale original_;
};

}  // namespace

TEST(ModuleTranslatorTest, ReinstallReleasesPreviousTranslators) {
  ASSERT_TRUE(WaitForModules());
  const ScopedTranslatedLocale locale;

  const auto first = Reinstall();
  if (first.installed.isEmpty()) {
    GTEST_SKIP() << "no module registered a translator data reader here";
  }
  for (const auto& translator : first.installed) {
    ASSERT_FALSE(translator.isNull());
  }

  const auto second = Reinstall();

  // Gone, not merely forgotten: a translator that is only dropped from the
  // manager's list stays installed on qApp and keeps reading the QM bytes this
  // very call released.
  for (const auto& translator : first.installed) {
    EXPECT_TRUE(translator.isNull());
  }

  EXPECT_EQ(second.installed.size(), first.installed.size());
  for (const auto& translator : second.installed) {
    EXPECT_FALSE(translator.isNull());
  }
}

TEST(ModuleTranslatorTest, RepeatedReinstallKeepsTranslationsResolving) {
  ASSERT_TRUE(WaitForModules());
  const ScopedTranslatedLocale locale;

  const auto first = Reinstall();
  if (first.installed.isEmpty()) {
    GTEST_SKIP() << "no module registered a translator data reader here";
  }

  // Otherwise the probe below proves nothing: an untranslated string comes back
  // unchanged whether a translator is consulted or not.
  ASSERT_NE(first.probe, QString(kProbeSource));

  // Three more switches, the way a user clicking through the wizard's language
  // box produces them. The answer must not drift or come back garbled.
  for (int i = 0; i < 3; i++) {
    EXPECT_EQ(Reinstall().probe, first.probe);
  }
}

namespace {

struct ModuleText {
  const char* module_id;
  const char* context;
  const char* source;
};

// What the Settings sidebar and the menus show. Each was once marked where
// lupdate could not see it (a GC_TR macro, a local Tr() wrapper), so none of
// it reached a .ts file and all of it shipped in English.
constexpr std::array<ModuleText, 6> kModuleUiText = {{
    {"com.bktus.gpgfrontend.module.email", "ModuleEMail", "Mail Accounts"},
    {"com.bktus.gpgfrontend.module.email", "ModuleEMail", "Mail Editor"},
    {"com.bktus.gpgfrontend.module.im", "ModuleIM", "Instant Messaging"},
    {"com.bktus.gpgfrontend.module.im", "ModuleIM", "IM Encrypt"},
    {"com.bktus.gpgfrontend.module.key_server_sync", "ModuleKeyServerSync",
     "Key Servers"},
    {"com.bktus.gpgfrontend.module.version_checking", "ModuleVersionChecking",
     "Updates"},
}};

constexpr auto kStranger = "com.example.translator.stranger";

}  // namespace

TEST(ModuleTranslatorTest, ModuleUiTextTranslatesInItsOwnContext) {
  ASSERT_TRUE(WaitForModules());
  const ScopedTranslatedLocale locale;

  if (Reinstall().installed.isEmpty()) {
    GTEST_SKIP() << "no module registered a translator data reader here";
  }

  auto& modules = Module::ModuleManager::GetInstance();
  int checked = 0;
  for (const auto& t : kModuleUiText) {
    const auto context = modules.GetModuleTranslationContext(t.module_id);
    if (context.isEmpty()) continue;  // not loaded in this build
    EXPECT_EQ(context, t.context) << t.module_id;
    EXPECT_NE(QCoreApplication::translate(t.context, t.source),
              QString(t.source))
        << t.context << ": " << t.source;
    checked++;
  }
  EXPECT_GT(checked, 0) << "a translator is installed, so some module is";
}

// Every module's strings sit under its own translation_context. A bare "GTrC"
// context is what lupdate writes when it cannot resolve the generated
// GFModuleTr.h, and nothing translates in it any more.
TEST(ModuleTranslatorTest, EveryModuleFilesItsStringsUnderItsOwnContext) {
  const QDir root(QString(GF_TEST_SOURCE_DIR) + "/modules/src");
  ASSERT_TRUE(root.exists());

  int checked = 0;
  for (const auto& dir : root.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
    QFile manifest(root.filePath(dir + "/module.json"));
    const QDir ts(root.filePath(dir + "/ts"));
    if (!ts.exists() || !manifest.open(QIODevice::ReadOnly)) continue;
    const auto context = QJsonDocument::fromJson(manifest.readAll())
                             .object()
                             .value("translation_context")
                             .toString();
    ASSERT_FALSE(context.isEmpty()) << dir.toStdString();

    for (const auto& name : ts.entryList({"*.ts"}, QDir::Files)) {
      QFile f(ts.filePath(name));
      ASSERT_TRUE(f.open(QIODevice::ReadOnly)) << name.toStdString();
      const auto text = QString::fromUtf8(f.readAll());
      EXPECT_FALSE(text.contains("<name>GTrC</name>")) << name.toStdString();
      EXPECT_TRUE(text.contains("<name>" + context + "</name>"))
          << name.toStdString() << " has nothing in " << context.toStdString();
      checked++;
    }
  }
  EXPECT_GT(checked, 0);
}

// The context is the Host's to decide, from the owner's signed manifest: a
// module that names another module's context gets none, not that one.
TEST(ModuleTranslatorTest, HostDecidesWhichContextModuleTextIsIn) {
  UI::NativeWidgetEntry entry;
  entry.owner = kStranger;
  entry.id = QString(kStranger) + ".borrower";
  entry.kind = UI::NativeWidgetKind::kDIALOG;
  entry.create = [](quint64, const QCborMap&) -> QWidget* { return nullptr; };
  entry.tr_context = "ModuleEMail";
  entry.title = "Mail Accounts";
  ASSERT_TRUE(UI::NativeWidgetRegistry::Instance().Register(entry));
  const auto found = UI::NativeWidgetRegistry::Instance().Find(entry.id);
  ASSERT_TRUE(found.has_value());
  EXPECT_TRUE(found->tr_context.isEmpty());
  EXPECT_EQ(found->Translate(found->title), QString("Mail Accounts"));
  EXPECT_TRUE(
      UI::NativeWidgetRegistry::Instance().Unregister(kStranger, entry.id));

  UI::CommandProvider command;
  command.id = QString(kStranger) + ".borrower";
  command.owner = kStranger;
  command.descriptor = QCborMap{{QStringLiteral("title"), "Mail Accounts"},
                                {QStringLiteral("tr_context"), "ModuleEMail"}};
  command.run = [](const gf::cmd::CommandContext&, QCborMap,
                   std::vector<gf::cmd::Blob>, gf::cmd::Completer done) {
    done({GF_CMD_OK, 0, {}, {}, {}});
  };
  auto& commands = UI::CommandRegistry::Instance();
  ASSERT_EQ(commands.Register(std::move(command)), GF_CMD_OK);
  const auto described = commands.Describe(QString(kStranger) + ".borrower");
  ASSERT_TRUE(described.has_value());
  EXPECT_TRUE(
      described->value(QStringLiteral("tr_context")).toString().isEmpty());
  EXPECT_EQ(UI::CommandTitle(*described), QString("Mail Accounts"));
  EXPECT_EQ(commands.Unregister(QString(kStranger) + ".borrower", kStranger),
            GF_CMD_OK);
}

}  // namespace GpgFrontend::Test
