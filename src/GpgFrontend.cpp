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

#include <qcommandlineparser.h>
#include <qloggingcategory.h>

//
#include "Application.h"
#include "BinaryValidate.h"
#include "Command.h"
#include "GpgFrontendContext.h"
#include "Initialize.h"
#include "core/function/AppSecureKeyManager.h"
#include "core/function/GlobalSettingStation.h"
#include "core/function/SystemSecretStore.h"
#include "platform/PlatformSecretStore.h"

namespace {

/**
 * @brief Report a secure key failure to the user.
 *
 * AppSecureKeyManager lives in gf_core, which does not link QtWidgets, so it
 * reports failures as a status and the presentation happens here.
 *
 * @param result outcome of AppSecureKeyManager::Initialize()
 * @return true when startup may continue
 */
auto ReportAppSecureKeyFailure(
    const GpgFrontend::AppSecureKeyInitResult& result) -> bool {
  switch (result.status) {
    case GpgFrontend::AppSecureKeyStatus::kOK:
      return true;

    case GpgFrontend::AppSecureKeyStatus::kWRITE_FAILED:
      // Continuing would be silent data loss: everything encrypted during this
      // session gets a key ID that no longer exists on the next start, so it
      // would come back as unreadable rather than merely unsaved.
      QMessageBox::critical(
          nullptr, QObject::tr("Save Key Failed"),
          QObject::tr("The application secure key could not be saved: %1")
                  .arg(result.detail) +
              "\n" +
              QObject::tr("Anything saved now would be unreadable after a "
                          "restart, so the application will not continue. "
                          "Please check your storage and permissions."),
          QMessageBox::Ok);
      return false;

    case GpgFrontend::AppSecureKeyStatus::kDECRYPT_FAILED:
      QMessageBox::critical(
          nullptr, QObject::tr("App Secure Key Error"),
          QObject::tr("Failed to decrypt the application secure key. Your PIN "
                      "may be incorrect, or the key file may be corrupted.") +
              "\n" + QObject::tr("Please clear the secure key and try again."),
          QMessageBox::Ok);
      return false;

    case GpgFrontend::AppSecureKeyStatus::kREAD_FAILED:
      QMessageBox::critical(
          nullptr, QObject::tr("App Secure Key Error"),
          QObject::tr(
              "Failed to read the application secure key from disk at: %1")
                  .arg(result.detail) +
              "\n" +
              QObject::tr("Please ensure the key file exists and is "
                          "accessible, or try re-initializing the secure key."),
          QMessageBox::Ok);
      return false;

    case GpgFrontend::AppSecureKeyStatus::kGENERATE_FAILED:
      QMessageBox::critical(
          nullptr, QObject::tr("Secure Key Generation Failed"),
          QObject::tr("Failed to generate an application secure key.") + "\n" +
              QObject::tr("Please check your system's cryptography support."),
          QMessageBox::Ok);
      return false;
  }

  return false;
}

/**
 * @brief Whether OS-backed protection of the key file is in effect.
 *
 * GFOSSecretStore already accounts for portable mode, and for the setting
 * being opt-in, where it is resolved in GpgFrontendContext. The portable check
 * is repeated here only because a qApp property is untyped and writable by
 * anything in the process, and wrapping the key on a portable installation
 * would strand it. One redundant comparison is cheap insurance on the one file
 * every stored secret depends on.
 *
 * @return true when the key file should be protected by the credential store
 */
auto IsKeyWrapRequested() -> bool {
  if (GpgFrontend::GetGSS().IsProtableMode()) return false;
  return qApp->property("GFOSSecretStore").toBool();
}

/**
 * @brief Report the outcome of reconciling the key file's at-rest protection.
 *
 * @param result outcome of AppSecureKeyManager::ResolveWrapSecret()
 * @return true when startup may continue
 */
auto ReportAppKeyWrapOutcome(const GpgFrontend::AppKeyWrapResult& result)
    -> bool {
  switch (result.status) {
    case GpgFrontend::AppKeyWrapStatus::kNOT_WRAPPED:
    case GpgFrontend::AppKeyWrapStatus::kWRAPPED:
    case GpgFrontend::AppKeyWrapStatus::kJUST_ENABLED:
    case GpgFrontend::AppKeyWrapStatus::kJUST_DISABLED:
      return true;

    case GpgFrontend::AppKeyWrapStatus::kSTORE_UNAVAILABLE:
      // Turn the preference back off rather than retrying, and failing, at
      // every launch. The key stays exactly as it was, just unprotected.
      GpgFrontend::GetSettings().setValue("advanced/os_secret_store", false);
      QMessageBox::warning(
          nullptr, QObject::tr("System Keychain Unavailable"),
          QObject::tr("The application key could not be protected using the "
                      "system keychain, so it remains stored unprotected.") +
              "\n" +
              QObject::tr("This setting has been turned off. You can turn it "
                          "on again once a keychain is available."),
          QMessageBox::Ok);
      return true;

    case GpgFrontend::AppKeyWrapStatus::kIO_FAILED:
      QMessageBox::critical(
          nullptr, QObject::tr("App Secure Key Error"),
          QObject::tr("The application secure key at %1 could not be read or "
                      "rewritten.")
                  .arg(result.detail) +
              "\n" + QObject::tr("Please check your storage and permissions."),
          QMessageBox::Ok);
      return false;

    case GpgFrontend::AppKeyWrapStatus::kLOCKED_OUT: {
      // Never reset on the user's behalf: resetting is unrecoverable, whereas
      // quitting costs nothing and lets them unlock the keychain and retry.
      QMessageBox box(QMessageBox::Critical,
                      QObject::tr("Secure Key Unavailable"),
                      QObject::tr("The application key is protected by a "
                                  "secret kept in the system keychain, and "
                                  "that secret could not be read."));
      box.setInformativeText(
          QObject::tr("This usually means the keychain is locked, was reset, "
                      "or this profile was copied from another computer or "
                      "user account.") +
          "\n\n" +
          QObject::tr("You can unlock the keychain and start the application "
                      "again. Resetting the key instead lets the application "
                      "start, but everything it previously encrypted becomes "
                      "permanently unreadable."));
      box.setDetailedText(
          QObject::tr("Keychain backend: %1").arg(result.detail));

      auto* quit = box.addButton(QObject::tr("Quit"), QMessageBox::RejectRole);
      auto* reset = box.addButton(QObject::tr("Reset Secure Key"),
                                  QMessageBox::DestructiveRole);
      box.setDefaultButton(quit);
      box.exec();

      if (box.clickedButton() != reset) return false;

      const auto confirm = QMessageBox::warning(
          nullptr, QObject::tr("Reset Secure Key"),
          QObject::tr("Everything the application has encrypted with the old "
                      "key will be permanently unreadable.") +
              "\n\n" + QObject::tr("Reset the secure key?"),
          QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);

      if (confirm != QMessageBox::Yes) return false;

      auto& key_mgr = GpgFrontend::AppSecureKeyManager::GetInstance();
      if (!QFile::remove(key_mgr.GetLegacyKeyPath())) {
        QMessageBox::critical(
            nullptr, QObject::tr("Reset Secure Key"),
            QObject::tr("The key file at %1 could not be removed.")
                .arg(key_mgr.GetLegacyKeyPath()),
            QMessageBox::Ok);
        return false;
      }

      if (auto* store = GpgFrontend::GetSystemSecretStore(); store != nullptr) {
        store->Remove(GpgFrontend::kAppKeyWrapAccount);
      }
      GpgFrontend::GetSettings().setValue("advanced/os_secret_store", false);

      qWarning() << "app secure key was reset at the user's request";
      return true;
    }
  }

  return false;
}

}  // namespace

/**
 *
 * @param argc
 * @param argv
 * @return
 */
auto main(int argc, char* argv[]) -> int {
  // initialize qt resources
  Q_INIT_RESOURCE(gpgfrontend);

  auto const ctx =
      QSharedPointer<GpgFrontend::GpgFrontendContext>::create(argc, argv);

  // create qt core application
  ctx->InitApplication();

  const auto* app = ctx->GetApp();
  Q_ASSERT(app != nullptr);

  // do some early init
  GpgFrontend::PreInit(ctx);

  auto rtn = 0;

  QCommandLineParser parser;
  parser.addHelpOption();
  parser.addOptions({
      {{"v", "version"}, "show version information"},
      {{"t", "test"}, "run all unit test cases"},
      {{"e", "environment"}, "show environment information"},
      {{"l", "log-level"}, "set log level (debug, info, warn, error)", "none"},
      {{{}, "self-check"}, "check libraries and executables validity"},
  });

  // Hold back GoogleTest flags (`--gtest_*`) from the app's parser, which would
  // otherwise reject them as unknown options. They are consumed later by
  // testing::InitGoogleTest, which reads the unmodified argv during RunTest, so
  // `gpgfrontend -t --gtest_filter=...` works without the GTEST_* env vars.
  QStringList parser_args;
  for (const auto& arg : QCoreApplication::arguments()) {
    if (arg.startsWith("--gtest_")) continue;
    parser_args << arg;
  }
  parser.process(parser_args);

  const auto self_check = app->property("GFSelfCheck").toBool();
  if ((self_check || parser.isSet("self-check")) && !ValidateLibraries()) {
    QMessageBox::critical(
        nullptr, QObject::tr("Program Self-Test Failed"),
        QObject::tr(
            "The application has detected an issue while verifying essential "
            "libraries and binaries that were digitally signed during the "
            "build. "
            "This means one or more files may have been altered or are being "
            "loaded from the wrong location. For security reasons, the program "
            "must now exit."),
        QMessageBox::Ok);
    return -1;
  }

  if (parser.isSet("v")) {
    return GpgFrontend::PrintVersion();
  }

  if (parser.isSet("l")) {
    GpgFrontend::ParseLogLevel(parser.value("l"));
  }

  if (parser.isSet("e")) {
    return GpgFrontend::PrintEnvInfo();
  }

  const auto secure_level = qApp->property("GFSecureLevel").toInt();
  GpgFrontend::GFBuffer buf;

  if (secure_level > 2) {
    bool ok = false;
    auto pin = QInputDialog::getText(
        nullptr, QObject::tr("PIN Required"),
        QObject::tr("High security mode is enabled.") + "\n\n" +
            QObject::tr("To unlock the application please enter your PIN."),
        QLineEdit::Password, {}, &ok);

    if (!ok || pin.isEmpty()) return 1;

    buf = GpgFrontend::GFBuffer(pin);
    pin.fill('X');
    pin.clear();
  }

  GpgFrontend::InstallPlatformSecretStore();

  // The two protections are mutually exclusive and must stay distinct: a PIN
  // is low entropy and is stretched with Argon2id, while the credential
  // store's secret is 32 random bytes and uses a fast derivation. In high
  // security mode the typed PIN protects the key, so the credential store
  // stays out of it entirely and `wrap` is left empty.
  GpgFrontend::GFBuffer wrap;

  if (secure_level <= 2) {
    auto& key_mgr = GpgFrontend::AppSecureKeyManager::GetInstance();
    const auto wrap_result =
        GpgFrontend::AppSecureKeyManager::ResolveWrapSecret(
            key_mgr.GetLegacyKeyPath(), GpgFrontend::GetSystemSecretStore(),
            IsKeyWrapRequested());

    if (!ReportAppKeyWrapOutcome(wrap_result)) return 1;
    wrap = wrap_result.secret;
  }

  const auto key_result =
      GpgFrontend::AppSecureKeyManager::GetInstance().Initialize(buf, wrap);

  if (!ReportAppSecureKeyFailure(key_result)) return 1;

  if (parser.isSet("t")) {
    ctx->gather_external_gnupg_info = false;
    ctx->unit_test_mode = true;

    InitGlobalBasicEnvSync(ctx);
    rtn = RunTest(ctx);
    ShutdownGlobalBasicEnv(ctx);
    return rtn;
  }

  ctx->gather_external_gnupg_info = true;
  ctx->unit_test_mode = false;

  InitGlobalBasicEnv(ctx, true);

  rtn = StartApplication(ctx);
  ShutdownGlobalBasicEnv(ctx);
  return rtn;
}
