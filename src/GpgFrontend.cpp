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
#include "core/function/AESCryptoHelper.h"
#include "core/function/AppSecureKeyManager.h"
#include "core/function/GFBufferFactory.h"
#include "core/function/GlobalSettingStation.h"
#include "core/function/SystemSecretStore.h"
#include "platform/PlatformSecretStore.h"
#include "ui/dialog/AppKeyPinDialog.h"

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
      // A wrong PIN is caught and retried before the key loader ever runs, so
      // reaching here means the key file itself will not decrypt.
      QMessageBox::critical(
          nullptr, QObject::tr("App Secure Key Error"),
          QObject::tr("Failed to decrypt the application secure key. The key "
                      "file may be corrupted.") +
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
 * @brief The at-rest protection in effect, with the portable rule re-applied.
 *
 * GpgFrontendContext already resolved this, portable mode included. The rule is
 * applied a second time here only because a qApp property is untyped and
 * writable by anything in the process, and wrapping the key with a credential
 * store on a portable installation would strand it. One redundant comparison is
 * cheap insurance on the one file every stored secret depends on.
 *
 * @return the protection the key loader should act on
 */
auto RequestedAppKeyProtection() -> GpgFrontend::AppKeyProtection {
  return GpgFrontend::ApplyPortableModeRule(
      GpgFrontend::AppKeyProtectionFromApp(),
      GpgFrontend::GetGSS().IsProtableMode());
}

/**
 * @brief Discard the secure key and drop protection back to the default.
 *
 * The single, deliberately destructive escape hatch for a key that can no
 * longer be opened — a forgotten PIN or an unrecoverable keychain secret. It
 * deletes the on-disk key material, clears the keychain wrap secret, and resets
 * the protection preference to "none", so the next Initialize() generates a
 * fresh, unprotected key. Everything the old key encrypted becomes permanently
 * unreadable, so every caller confirms the choice with the user first.
 *
 * @return true when the key was reset and startup may proceed
 */
auto ResetAppSecureKeyToDefault() -> bool {
  auto& key_mgr = GpgFrontend::AppSecureKeyManager::GetInstance();
  if (!GpgFrontend::AppSecureKeyManager::ResetKeyStorage(key_mgr.GetKeyDir())) {
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
  GpgFrontend::GetSettings().setValue(
      "advanced/app_key_protection",
      AppKeyProtectionToString(GpgFrontend::AppKeyProtection::kNONE));

  qWarning() << "app secure key was reset at the user's request";
  return true;
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
      GpgFrontend::GetSettings().setValue(
          "advanced/app_key_protection",
          AppKeyProtectionToString(GpgFrontend::AppKeyProtection::kNONE));
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

      return ResetAppSecureKeyToDefault();
    }
  }

  return false;
}

/**
 * @brief Outcome of asking the user for the application key PIN.
 *
 * The empty-PIN case is deliberately not overloaded to mean "quit": a reset
 * key file legitimately continues with no PIN, and a cancel must not.
 */
struct AppKeyPinPrompt {
  bool proceed = false;       ///< false means quit
  GpgFrontend::GFBuffer pin;  ///< the PIN, empty when proceeding unprotected
};

/**
 * @brief Confirm and carry out a reset to default after a forgotten PIN.
 *
 * A two-step confirmation, matching the keychain lock-out reset: that
 * everything encrypted becomes permanently unreadable is stated twice, because
 * it cannot be undone. Any back-out returns false and leaves the key untouched.
 *
 * @return true only when the key was actually reset and startup may proceed
 */
auto ConfirmForgottenPinReset() -> bool {
  QMessageBox box(QMessageBox::Warning, QObject::tr("Reset to Default"),
                  QObject::tr("Resetting removes the PIN and lets the "
                              "application start, but everything it previously "
                              "encrypted becomes permanently unreadable."));
  box.setInformativeText(QObject::tr(
      "Only do this if you cannot recall the PIN. There is no other "
      "way to recover the key."));
  auto* back = box.addButton(QObject::tr("Go Back"), QMessageBox::RejectRole);
  auto* reset = box.addButton(QObject::tr("Reset to Default"),
                              QMessageBox::DestructiveRole);
  box.setDefaultButton(back);
  box.exec();

  if (box.clickedButton() != reset) return false;

  const auto confirm = QMessageBox::warning(
      nullptr, QObject::tr("Reset to Default"),
      QObject::tr("Everything the application has encrypted with the current "
                  "key will be permanently unreadable.") +
          "\n\n" + QObject::tr("Reset the secure key?"),
      QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);

  if (confirm != QMessageBox::Yes) return false;

  return ResetAppSecureKeyToDefault();
}

/**
 * @brief Prompt for the application key PIN until it opens the key file.
 *
 * The PIN is trial-unsealed here rather than left to Initialize(), because once
 * inside the loader a mistyped PIN and a corrupted key file are
 * indistinguishable: the old code turned the first into a refusal to launch.
 * Here it costs only a retry.
 *
 * Shapes of key file that are handled:
 *  - none yet: a fresh profile that opted into a PIN before a key existed, so
 *    the PIN is collected once and used to seal the key about to be generated;
 *  - present but not encrypted: the setting and the file disagree, which means
 *    a crashed transition or a hand-edited ini. The key is intact, so the
 *    setting is reset to "none" and startup proceeds unprotected rather than
 *    refusing to launch;
 *  - present but unreadable: an I/O error, distinct from a wrong PIN — a retry
 *    loop would only ever repeat, so it is reported and startup stops;
 *  - present and encrypted: the real case, retried until the PIN opens it.
 *
 * After a few misses the dialog reveals a reset option: a user who has truly
 * forgotten the PIN can discard the key for a fresh, unprotected one rather
 * than being trapped between an unopenable key and quitting.
 *
 * @param key_path path of the key file
 * @return whether to proceed, and the PIN to proceed with
 */
auto PromptForAppKeyPin(const QString& key_path) -> AppKeyPinPrompt {
  if (!QFileInfo(key_path).exists()) {
    GpgFrontend::UI::AppKeyPinDialog dialog(
        GpgFrontend::UI::AppKeyPinDialog::Mode::kSET);
    if (dialog.exec() != QDialog::Accepted) return {};
    return {true, dialog.Pin()};
  }

  auto on_disk = GpgFrontend::GFBufferFactory::FromFile(key_path);
  if (!on_disk) {
    // The file exists but will not read: an unlock loop can only repeat, so say
    // what is actually wrong and let ReportAppSecureKeyFailure handle the halt.
    QMessageBox::critical(
        nullptr, QObject::tr("App Secure Key Error"),
        QObject::tr("The application secure key at %1 could not be read.")
                .arg(key_path) +
            "\n" + QObject::tr("Please check your storage and permissions."),
        QMessageBox::Ok);
    return {};
  }

  if (!GpgFrontend::AESCryptoHelper::IsEncryptedBuffer(*on_disk)) {
    QMessageBox::warning(
        nullptr, QObject::tr("Application Key Not Protected"),
        QObject::tr("A PIN is configured, but the application key on disk is "
                    "not encrypted.") +
            "\n" +
            QObject::tr("This can happen if a previous change was interrupted. "
                        "The PIN setting has been turned off and the key is "
                        "left as it is."),
        QMessageBox::Ok);
    GpgFrontend::GetSettings().setValue(
        "advanced/app_key_protection",
        AppKeyProtectionToString(GpgFrontend::AppKeyProtection::kNONE));
    // The file really is plaintext, so proceeding with an empty PIN loads it
    // exactly as an unprotected profile would.
    return {true, {}};
  }

  // A locked keyring has a lockout; a PIN does not, so there is nothing to be
  // gained by capping attempts — only the user's own patience limits them.
  GpgFrontend::UI::AppKeyPinDialog dialog(
      GpgFrontend::UI::AppKeyPinDialog::Mode::kUNLOCK);
  int failures = 0;
  while (true) {
    const int code = dialog.exec();

    // The reset escape hatch, offered only after RevealResetOption() below. A
    // confirmed reset proceeds with an empty PIN onto a freshly generated key;
    // backing out drops straight back into the unlock loop.
    if (code == GpgFrontend::UI::AppKeyPinDialog::kResetRequested) {
      if (ConfirmForgottenPinReset()) return {true, {}};
      continue;
    }

    // Anything other than accept is a Quit: it costs nothing and leaves the key
    // intact, so it is never overloaded to mean reset.
    if (code != QDialog::Accepted) return {};

    auto pin = dialog.Pin();
    if (GpgFrontend::AppSecureKeyManager::UnsealKey(pin, {}, *on_disk)) {
      return {true, pin};
    }

    // Clear the field first and set the message last: clearing emits a change
    // that hides the error, so the message has to be the final mutation or it
    // would vanish before the dialog is shown again.
    ++failures;
    dialog.Clear();
    auto message = QObject::tr(
        "That PIN did not unlock the application key. Please try again.");
    // After a few misses, say the thing that actually matters — that a
    // forgotten PIN is not recoverable — and reveal the reset option so a stuck
    // user has a way out other than quitting.
    if (failures >= 3) {
      message += "\n" + QObject::tr(
                            "If you have forgotten your PIN, the application "
                            "key and everything encrypted with it cannot be "
                            "recovered.");
      dialog.RevealResetOption();
    }
    dialog.SetErrorText(message);
  }
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

  GpgFrontend::InstallPlatformSecretStore();

  auto& key_mgr = GpgFrontend::AppSecureKeyManager::GetInstance();
  const auto protection = RequestedAppKeyProtection();

  // The three backends are mutually exclusive and stay distinct because their
  // secrets are not alike: a PIN is low entropy and is stretched with Argon2id,
  // while the credential store's secret is 32 random bytes and uses a fast
  // derivation. Whichever is in use fills exactly one of the two slots below.
  GpgFrontend::GFBuffer pin;
  GpgFrontend::GFBuffer wrap;

  if (protection == GpgFrontend::AppKeyProtection::kPIN) {
    const auto prompt = PromptForAppKeyPin(key_mgr.GetLegacyKeyPath());
    // Cancelling quits: it costs the user nothing and leaves the key intact,
    // whereas resetting on their behalf would not. A reset key file proceeds
    // with an empty PIN, loaded exactly as an unprotected profile.
    if (!prompt.proceed) return 1;
    pin = prompt.pin;
  } else {
    const auto wrap_result =
        GpgFrontend::AppSecureKeyManager::ResolveWrapSecret(
            key_mgr.GetLegacyKeyPath(), GpgFrontend::GetSystemSecretStore(),
            protection == GpgFrontend::AppKeyProtection::kKEYCHAIN);

    if (!ReportAppKeyWrapOutcome(wrap_result)) return 1;
    wrap = wrap_result.secret;
  }

  const auto key_result = key_mgr.Initialize(pin, wrap);

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
