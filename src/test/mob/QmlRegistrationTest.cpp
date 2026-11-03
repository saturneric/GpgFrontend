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

#include "MobileTest.h"
#include "crypto/PassphraseBroker.h"
#include "lifecycle/AppLifecycle.h"
#include "model/KeyListController.h"
#include "startup/InteractionBridge.h"
#include "startup/StartupController.h"

namespace GpgFrontend::Test::Mobile {

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
        property var files: FileWorkspace {}
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
      }
    )",
                      QUrl());
    std::unique_ptr<QObject> object(component.create());
    ASSERT_NE(object, nullptr) << component.errorString().toStdString();
    EXPECT_NE(object->property("filter").value<QObject*>(), nullptr);
    EXPECT_NE(object->property("keys").value<QObject*>(), nullptr);
    for (const auto* name :
         {"crypto", "importer", "actions", "keyActions", "files", "settings"}) {
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
  });
}

// Every page of the application loads.
TEST(QmlRegistrationTest, EveryPageLoads) {
  RunOnMainThread([]() {
    QQmlEngine engine;
    for (const auto* name :
         {"SplashPage", "ErrorPage", "MainPage", "KeysPage", "KeyDetailPage",
          "TextPage", "PinSheet", "ConfirmSheet", "PassphraseSheet",
          "RecipientSheet", "SignerSheet", "ResultSheet", "ImportSheet",
          "FilesPage", "SettingsPage", "GenerateSheet", "AboutPage",
          "FileDetailsSheet", "VerifySheet"}) {
      QQmlComponent component(&engine, "GpgFrontend.Mobile", name);
      std::unique_ptr<QObject> object(component.create());
      EXPECT_NE(object, nullptr)
          << name << ": " << component.errorString().toStdString();
    }
  });
}

}  // namespace GpgFrontend::Test::Mobile
