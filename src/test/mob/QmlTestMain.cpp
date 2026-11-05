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

/**
 * @file QmlTestMain.cpp
 * @brief gf_mob_qml_tests: the application's pages, tested in QML.
 *
 * The same plugin and singletons as the application, but no startup: the
 * pages are tested on their own, with an empty key list. They are laid out as
 * on the phone: in the Material style (qtquickcontrols2.conf names no style,
 * and the host's default has other measures), with the application's
 * resources (its fonts and icons) loaded.
 *
 * TestSupport (the context property of that name) does for a test what the
 * running application gets from elsewhere: a language installed, a PIN asked
 * for by the profile loader's thread, a passphrase asked for by the core.
 */

#include <QtQml/qqmlextensionplugin.h>
#include <QtQuickTest/quicktest.h>

#include <QtQml/QQmlContext>
#include <QtQml/QQmlEngine>
#include <QtQuickControls2/QQuickStyle>
#include <mutex>
#include <thread>

#include "AppServices.h"
#include "GpgFrontendResource.h"
#include "boot/BootContext.h"
#include "core/function/CoreSignalStation.h"
#include "core/model/GpgPassphraseContext.h"
#include "shell/present/TranslationUtils.h"
#include "startup/InteractionBridge.h"

Q_IMPORT_QML_PLUGIN(GpgFrontend_MobilePlugin)

class TestSupport : public QObject {
  Q_OBJECT

 public:
  explicit TestSupport(QQmlEngine* engine) : engine_(engine) {}

  ~TestSupport() override { JoinAsker(); }

  /// Install the catalog of @p locale ("de_DE", "en_US") and translate what
  /// is up again, as a language chosen in the settings does.
  Q_INVOKABLE void useLanguage(const QString& locale) {
    GpgFrontend::InstallTranslations(QLocale(locale));
    engine_->retranslate();
  }

  /// Ask for the profile's PIN as the profile loader does: from a thread of
  /// its own, waiting there for the answer (answer()).
  Q_INVOKABLE void askPin(bool creating, int failures) {
    Ask(GpgFrontend::Mobile::InteractionBridge::Kind::kAPP_KEY_PIN,
        {{QStringLiteral("creating"), creating},
         {QStringLiteral("failures"), failures}});
  }

  /// Ask the profile loader's confirmation: forcing open a profile another
  /// process holds (@p forceUnlock), else resetting the profile's key.
  Q_INVOKABLE void askConfirmation(bool forceUnlock) {
    using Kind = GpgFrontend::Mobile::InteractionBridge::Kind;
    Ask(forceUnlock ? Kind::kCONFIRM_FORCE_UNLOCK : Kind::kCONFIRM_KEY_RESET,
        {{QStringLiteral("pid"), 4242},
         {QStringLiteral("host"), QStringLiteral("phone")},
         {QStringLiteral("application"), QStringLiteral("GpgFrontend")},
         {QStringLiteral("reason"), QStringLiteral("unreadable")}});
  }

  /// The last question's answer ("cancelled" when declined); undefined while
  /// it waits.
  Q_INVOKABLE [[nodiscard]] QVariant answer() const {
    std::lock_guard lock(mutex_);
    return pin_answer_;
  }

  /// Ask for a passphrase as the core does when an operation needs one.
  Q_INVOKABLE void askPassphrase() {
    passphrase_ = QSharedPointer<GpgFrontend::GpgPassphraseContext>::create();
    passphrase_->SetPassphraseInfo(QStringLiteral("Test key"));
    emit GpgFrontend::CoreSignalStation::GetInstance()
        -> SignalNeedUserInputPassphrase(passphrase_);
  }

  /// What came of it: "waiting", "cancelled", or "answered:<passphrase>".
  Q_INVOKABLE [[nodiscard]] QString passphraseAnswer() const {
    if (passphrase_ == nullptr) return {};
    if (passphrase_->IsCancelled()) return QStringLiteral("cancelled");
    const auto typed = passphrase_->GetPassphrase();
    if (typed.Empty()) return QStringLiteral("waiting");
    return QStringLiteral("answered:") + typed.ConvertToQByteArray();
  }

 private:
  QQmlEngine* engine_;
  mutable std::mutex mutex_;
  std::thread asker_;
  QVariant pin_answer_;
  QSharedPointer<GpgFrontend::GpgPassphraseContext> passphrase_;

  // Ask @p kind from a thread of its own, as the profile loader does.
  void Ask(GpgFrontend::Mobile::InteractionBridge::Kind kind,
           const QVariantMap& details) {
    JoinAsker();
    {
      std::lock_guard lock(mutex_);
      pin_answer_ = QVariant();
    }
    asker_ = std::thread([this, kind, details]() {
      auto* bridge =
          GpgFrontend::Mobile::InteractionBridge::create(nullptr, nullptr);
      const auto answer = bridge->Ask(kind, details);
      std::lock_guard lock(mutex_);
      pin_answer_ = answer.isValid() ? answer : QStringLiteral("cancelled");
    });
  }

  // A question still waiting is declined first (not CancelAll(), which
  // would answer every later one too).
  void JoinAsker() {
    if (!asker_.joinable()) return;
    auto* bridge =
        GpgFrontend::Mobile::InteractionBridge::create(nullptr, nullptr);
    while (!answer().isValid()) {
      const auto current = bridge->Current();
      if (!current.isEmpty()) {
        bridge->decline(current.value(QStringLiteral("id")).toInt());
      }
      std::this_thread::yield();
    }
    asker_.join();
  }
};

class Setup : public QObject {
  Q_OBJECT

 public slots:
  void applicationAvailable() {
    GpgFrontend::InitResources();
    QQuickStyle::setStyle(QStringLiteral("Material"));
    services_ = std::make_unique<GpgFrontend::Mobile::AppServices>(
        QSharedPointer<GpgFrontend::BootContext>::create(0, nullptr));
  }

  void qmlEngineAvailable(QQmlEngine* engine) {
    support_ = std::make_unique<TestSupport>(engine);
    engine->rootContext()->setContextProperty(QStringLiteral("TestSupport"),
                                              support_.get());
  }

  void cleanupTestCase() { support_.reset(); }

 private:
  std::unique_ptr<GpgFrontend::Mobile::AppServices> services_;
  std::unique_ptr<TestSupport> support_;
};

QUICK_TEST_MAIN_WITH_SETUP(gf_mob_qml_tests, Setup)

#include "QmlTestMain.moc"
