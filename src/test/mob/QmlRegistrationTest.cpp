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

#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QSignalSpy>

#include "MobileTest.h"
#include "core/function/openpgp/ChannelHandle.h"
#include "core/profile/ProfileSession.h"
#include "core/utils/GpgUtils.h"
#include "crypto/OperationQueue.h"
#include "crypto/PassphraseBroker.h"
#include "files/FileWorkspace.h"
#include "lifecycle/AppLifecycle.h"
#include "model/KeyListController.h"
#include "shell/rules/KeyGenProfile.h"
#include "startup/InteractionBridge.h"
#include "startup/StartupController.h"

namespace GpgFrontend::Test::Mobile {

namespace {

/// @p name under @p root, by the item tree too: a Repeater's items are not
/// its QObject children.
auto FindItem(QObject* root, const QString& name) -> QObject* {
  if (auto* found = root->findChild<QObject*>(name)) return found;
  auto* item = qobject_cast<QQuickItem*>(root);
  if (item == nullptr) return nullptr;
  for (auto* child : item->childItems()) {
    if (child->objectName() == name) return child;
    if (auto* found = FindItem(child, name)) return found;
  }
  return nullptr;
}

}  // namespace

// The runner imports the same plugin the application does
// (Q_IMPORT_QML_PLUGIN); each type it registers has to be usable from QML.
TEST(QmlRegistrationTest, EveryTypeOfTheModuleResolves) {
  RunOnMainThread([]() {
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(R"(
      import QtQml
      import GpgFrontend.Mobile
      QtObject {
        property var filter: KeyFilterModel { onlySecret: true }
        property var crypto: TextCryptoController {}
        property var importer: KeyImportController {}
        property var actions: OutputActions {}
        property var keyActions: KeyActionsController {}
        property var profiles: KeyGenProfilesController {}
        property var categories: KeyCategoriesController {}
        property var keyDatabases: KeyDatabasesController {}
        property var phoneProfiles: ProfilesController {}
        property var files: FileWorkspace
        property var selection: WorkSelection
        property string kind: ContentKind.textKind("x")
        property string word: Wording.operationLabel("encrypt")
        property var settings: SettingsModel {}
        property var asked: PassphraseBroker.current
        property var startup: StartupController.state
        property var keys: KeyListController.keys
        property var current: InteractionBridge.current
        property bool suspended: AppLifecycle.suspended
        property int ready: StartupController.State.kREADY
        property var startupObject: StartupController
        property var bridgeObject: InteractionBridge
        property var lifecycleObject: AppLifecycle
        property var keysObject: KeyListController
        property var passphrasesObject: PassphraseBroker
        property var operationsObject: OperationQueue
        property var log: OperationQueue.log
      }
    )",
                      QUrl());
    std::unique_ptr<QObject> object(component.create());
    ASSERT_NE(object, nullptr) << component.errorString().toStdString();
    EXPECT_NE(object->property("filter").value<QObject*>(), nullptr);
    EXPECT_NE(object->property("keys").value<QObject*>(), nullptr);
    for (const auto* name : {"crypto", "importer", "actions", "keyActions",
                             "profiles", "categories", "keyDatabases",
                             "phoneProfiles", "files", "settings", "log"}) {
      EXPECT_NE(object->property(name).value<QObject*>(), nullptr) << name;
    }
    EXPECT_EQ(object->property("suspended").toBool(), false);

    // The very objects the application made, not instances QML made itself:
    // a singleton with a default constructor is constructed by QML instead.
    using namespace GpgFrontend::Mobile;
    EXPECT_EQ(object->property("startupObject").value<QObject*>(),
              StartupController::create(nullptr, nullptr));
    EXPECT_EQ(object->property("bridgeObject").value<QObject*>(),
              InteractionBridge::create(nullptr, nullptr));
    EXPECT_EQ(object->property("lifecycleObject").value<QObject*>(),
              AppLifecycle::create(nullptr, nullptr));
    EXPECT_EQ(object->property("keysObject").value<QObject*>(),
              KeyListController::create(nullptr, nullptr));
    EXPECT_EQ(object->property("passphrasesObject").value<QObject*>(),
              PassphraseBroker::create(nullptr, nullptr));
    EXPECT_EQ(object->property("files").value<QObject*>(),
              FileWorkspace::create(nullptr, nullptr));
    EXPECT_EQ(object->property("operationsObject").value<QObject*>(),
              OperationQueue::create(nullptr, nullptr));
  });
}

// Every page of the application loads.
TEST(QmlRegistrationTest, EveryPageLoads) {
  RunOnMainThread([]() {
    QQmlEngine engine;
    const QStringList pages = {"SplashPage",
                               "ErrorPage",
                               "MainPage",
                               "KeysPage",
                               "KeyDetailPage",
                               "TextPage",
                               "PinSheet",
                               "ConfirmSheet",
                               "PassphraseSheet",
                               "KeyToolbox",
                               "StatusSheet",
                               "SheetHost",
                               "ImportSheet",
                               "FilesPage",
                               "SettingsPage",
                               "GenerateSheet",
                               "AboutPage",
                               "FileDetailsSheet",
                               "DetachedVerifySheet",
                               "StatusLine",
                               "KeyFacts",
                               "SubkeySheet",
                               "AddSubkeySheet",
                               "RevokeSubkeySheet",
                               "ValiditySheet",
                               "KeyAlgoChooser",
                               "UidSheet",
                               "AddUidSheet",
                               "CategorySheet",
                               "KeyDatabaseSheet",
                               "ProfileSheet",
                               "OperationMenu",
                               "CategoryListSheet",
                               "SelectedKeysSheet",
                               "GeneratedKeySheet",
                               "ResultCard",
                               "KeyCategoriesSheet",
                               "KeyPickerSheet",
                               "CategoryKeysPage",
                               "KeyGroupPage",
                               "KeyGroupSheet",
                               "FactsPage",
                               "TranslatorsPage",
                               "ProfilesPage",
                               "KeyCategoriesPage",
                               "KeyGenProfilesPage",
                               "KeyGenProfileEditPage"};
    for (const auto& name : pages) {
      QQmlComponent component(&engine, "GpgFrontend.Mobile", name);
      std::unique_ptr<QObject> object(component.create());
      EXPECT_NE(object, nullptr) << name.toStdString() << ": "
                                 << component.errorString().toStdString();
    }
  });
}

// Expert Mode and the profile editor, on the engine's real algorithms.
TEST(QmlRegistrationTest, ExpertChoicesComeFromTheEngine) {
  const auto before = LoadKeyGenSettings();
  RunOnMainThread([]() {
    QQmlEngine engine;
    const auto find = [](QObject* root, const char* name) {
      return root->findChild<QObject*>(QString::fromLatin1(name));
    };

    // Expert Mode starts from the chosen profile, its subkey included.
    QQmlComponent sheet_type(&engine, "GpgFrontend.Mobile", "GenerateSheet");
    std::unique_ptr<QObject> sheet(sheet_type.create());
    ASSERT_NE(sheet, nullptr) << sheet_type.errorString().toStdString();
    auto* expert = find(sheet.get(), "generateExpert");
    ASSERT_NE(expert, nullptr);
    QMetaObject::invokeMethod(expert, "toggle");
    QMetaObject::invokeMethod(expert, "toggled");
    auto* primary = find(sheet.get(), "primaryChooser");
    ASSERT_NE(primary, nullptr);
    EXPECT_FALSE(primary->property("algos").toList().isEmpty());
    EXPECT_TRUE(primary->property("complete").toBool());
    EXPECT_EQ(primary->property("algo").toMap().value("id").toString(),
              "ed25519");
    auto* subkey = find(sheet.get(), "subkeyChooser");
    EXPECT_EQ(subkey->property("algo").toMap().value("id").toString(),
              "cv25519");
    // A subkey for nothing is not made: every usage taken away, it is not
    // complete, and says why.
    EXPECT_TRUE(subkey->property("complete").toBool());
    const auto usage = subkey->property("usage");
    subkey->setProperty("usage", QVariantMap{{"encrypt", false},
                                             {"sign", false},
                                             {"authenticate", false}});
    EXPECT_FALSE(subkey->property("complete").toBool());
    EXPECT_TRUE(subkey->property("usageMissing").toBool());
    subkey->setProperty("usage", usage);
    EXPECT_TRUE(subkey->property("complete").toBool());

    // Another profile, chosen in Expert Mode, fills the choices in again.
    auto* profile = find(sheet.get(), "generateProfile");
    ASSERT_NE(profile, nullptr);
    const auto offered = profile->property("model").toList();
    int rsa = -1;
    for (int i = 0; i < offered.size(); ++i) {
      if (offered.at(i).toMap().value("name").toString().startsWith(
              "RSA-3072")) {
        rsa = i;
      }
    }
    ASSERT_GE(rsa, 0);
    profile->setProperty("currentIndex", rsa);
    QMetaObject::invokeMethod(profile, "activated", Q_ARG(int, rsa));
    EXPECT_EQ(primary->property("algo").toMap().value("id").toString(),
              "rsa3072");

    // A new profile, saved from the editor as the desktop's would be.
    QQmlComponent editor_type(&engine, "GpgFrontend.Mobile",
                              "KeyGenProfileEditPage");
    std::unique_ptr<QObject> editor(
        editor_type.createWithInitialProperties({{"index", -1}}));
    ASSERT_NE(editor, nullptr) << editor_type.errorString().toStdString();
    find(editor.get(), "profileName")->setProperty("text", "QML Test Profile");
    QSignalSpy saved(editor.get(), SIGNAL(saved()));
    QMetaObject::invokeMethod(editor.get(), "save");
    EXPECT_EQ(saved.count(), 1);
    bool listed = false;
    for (const auto& p : LoadStoredKeyGenProfiles().profiles) {
      listed |= p.name == "QML Test Profile" && p.has_s_key;
    }
    EXPECT_TRUE(listed);
  });
  StoreKeyGenSettings(before);
}

// Settings names the language in use, the system's until one is chosen.
TEST(QmlRegistrationTest, SettingsNamesTheLanguage) {
  RunOnMainThread([]() {
    QQmlEngine engine;
    QQmlComponent page_type(&engine, "GpgFrontend.Mobile", "SettingsPage");
    std::unique_ptr<QObject> page(page_type.create());
    ASSERT_NE(page, nullptr) << page_type.errorString().toStdString();
    auto* row = FindItem(page.get(), "language");
    ASSERT_NE(row, nullptr);
    EXPECT_EQ(row->property("valueText").toString(),
              page->property("languageName").toString());
    EXPECT_FALSE(row->property("valueText").toString().isEmpty());
  });
}

// Settings names the key database in use, and its sheet offers the others
// and a new one by name.
TEST(QmlRegistrationTest, KeyDatabaseSheetOffersTheDatabases) {
  const auto current =
      LookupChannelHandle(UsableDefaultChannelHandle()).database_id;
  RunOnMainThread([&current]() {
    QQmlEngine engine;
    const auto find = FindItem;

    QQmlComponent page_type(&engine, "GpgFrontend.Mobile", "SettingsPage");
    std::unique_ptr<QObject> page(page_type.create());
    ASSERT_NE(page, nullptr) << page_type.errorString().toStdString();
    auto* row = find(page.get(), "keyDatabase");
    ASSERT_NE(row, nullptr);
    QString name;
    for (const auto& info : GetGpgKeyDatabaseInfos()) {
      if (info.id == current) name = KeyDatabaseDisplayName(info.id, info.name);
    }
    ASSERT_FALSE(name.isEmpty());
    EXPECT_EQ(row->property("valueText").toString(), name);

    // And on the Keys tab, where it is switched most.
    QQmlComponent keys_type(&engine, "GpgFrontend.Mobile", "KeysPage");
    std::unique_ptr<QObject> keys(keys_type.create());
    ASSERT_NE(keys, nullptr) << keys_type.errorString().toStdString();
    auto* header = keys->property("header").value<QObject*>();
    ASSERT_NE(header, nullptr);
    auto* button = find(header, "keyDatabaseButton");
    ASSERT_NE(button, nullptr);
    EXPECT_EQ(button->property("text").toString(), name);

    // A pop-up, so not among the page's children: made on its own here.
    QQmlComponent sheet_type(&engine);
    sheet_type.setData(R"(
      import QtQuick
      import GpgFrontend.Mobile
      KeyDatabaseSheet {
        databases: KeyDatabasesController {}
        switcher: KeyDatabaseSwitchSheet { databases: KeyDatabasesController {} }
      }
    )",
                       QUrl());
    std::unique_ptr<QObject> sheet(sheet_type.create());
    ASSERT_NE(sheet, nullptr) << sheet_type.errorString().toStdString();
    QMetaObject::invokeMethod(sheet.get(), "begin");
    // Its items are in its body, not among its own children.
    auto* body = sheet->property("body").value<QObject*>();
    ASSERT_NE(body, nullptr);
    auto* choice = find(body, "keyDatabase_" + name);
    ASSERT_NE(choice, nullptr);
    EXPECT_TRUE(choice->property("checked").toBool());

    // Naming one starts from a free name, and a bad one is refused there.
    QMetaObject::invokeMethod(sheet.get(), "beginNew");
    EXPECT_TRUE(sheet->property("naming").toBool());
    auto* field = find(body, "keyDatabaseName");
    ASSERT_NE(field, nullptr);
    EXPECT_TRUE(field->property("text").toString().startsWith("Key DB "));
    field->setProperty("text", name);
    QMetaObject::invokeMethod(sheet.get(), "create");
    EXPECT_FALSE(find(body, "keyDatabaseProblem")
                     ->property("text")
                     .toString()
                     .isEmpty());
    EXPECT_FALSE(sheet->property("busy").toBool());
    // A refused name starts no switch: the choice stays, nothing runs.
    auto* switcher = sheet->property("switcher").value<QObject*>();
    ASSERT_NE(switcher, nullptr);
    EXPECT_FALSE(switcher->property("switching").toBool());
    EXPECT_TRUE(sheet->property("naming").toBool());
  });
  EXPECT_EQ(LookupChannelHandle(UsableDefaultChannelHandle()).database_id,
            current);
}

// The profiles page marks the open profile, and its sheet checks what is
// typed before anything is asked of the core.
TEST(QmlRegistrationTest, ProfilesPageChecksWhatIsTyped) {
  const auto open_id = ProfileSession::Instance().Profile().Id();
  RunOnMainThread([&open_id]() {
    QQmlEngine engine;
    QQmlComponent page_type(&engine, "GpgFrontend.Mobile", "ProfilesPage");
    std::unique_ptr<QObject> page(page_type.create());
    ASSERT_NE(page, nullptr) << page_type.errorString().toStdString();
    auto* row = FindItem(page.get(), "profile_" + open_id);
    ASSERT_NE(row, nullptr);
    EXPECT_TRUE(row->property("selected").toBool());

    auto* sheet = page->property("sheet").value<QObject*>();
    ASSERT_NE(sheet, nullptr);
    auto* body = sheet->property("body").value<QObject*>();
    ASSERT_NE(body, nullptr);
    QSignalSpy accepted(sheet, SIGNAL(accepted(QString)));
    const auto problem = [&]() {
      return FindItem(body, "profileProblem")->property("text").toString();
    };

    // An export's passphrase: long enough, and the same twice.
    QMetaObject::invokeMethod(sheet, "begin", Q_ARG(QVariant, "export"),
                              Q_ARG(QVariant, QVariant()));
    FindItem(body, "profileSecret")->setProperty("text", "short");
    FindItem(body, "profileConfirm")->setProperty("text", "short");
    QMetaObject::invokeMethod(sheet, "submit");
    EXPECT_FALSE(problem().isEmpty());
    FindItem(body, "profileSecret")->setProperty("text", "long enough one");
    FindItem(body, "profileConfirm")->setProperty("text", "long enough two");
    QMetaObject::invokeMethod(sheet, "submit");
    EXPECT_FALSE(problem().isEmpty());
    EXPECT_EQ(accepted.count(), 0);
    FindItem(body, "profileConfirm")->setProperty("text", "long enough one");
    QMetaObject::invokeMethod(sheet, "submit");
    ASSERT_EQ(accepted.count(), 1);
    EXPECT_EQ(accepted.front().front().toString(), "long enough one");
    // Not kept in the sheet once taken.
    EXPECT_TRUE(
        FindItem(body, "profileSecret")->property("text").toString().isEmpty());

    // A new profile's name may not be one there is.
    QMetaObject::invokeMethod(sheet, "begin", Q_ARG(QVariant, "create"),
                              Q_ARG(QVariant, QVariant()));
    FindItem(body, "profileName")
        ->setProperty("text", row->property("text").toString());
    QMetaObject::invokeMethod(sheet, "submit");
    EXPECT_FALSE(problem().isEmpty());
    EXPECT_EQ(accepted.count(), 1);
  });
}

}  // namespace GpgFrontend::Test::Mobile