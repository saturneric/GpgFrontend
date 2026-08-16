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

#include "ui/function/GuiProfileLoaderDelegate.h"

#include <qinputdialog.h>
#include <qlineedit.h>

#include <optional>

#include "core/function/SystemSecretStore.h"
#include "ui/dialog/AppKeyPinDialog.h"

namespace GpgFrontend::UI {

namespace {

/// After a few misses, say the thing that actually matters — that a forgotten
/// PIN is not recoverable — and offer the way out.
constexpr int kRevealResetAfter = 3;

/**
 * @brief Confirm a reset twice, because it cannot be undone.
 *
 * That everything encrypted becomes permanently unreadable is stated in both
 * steps. Any back-out leaves the key untouched.
 *
 * @param reason why the key cannot be opened
 * @return true only when the user has said yes twice
 */
auto ConfirmDestructiveReset(ProfileKeyResetReason reason) -> bool {
  QMessageBox box(
      QMessageBox::Warning, QObject::tr("Reset to Default"),
      reason == ProfileKeyResetReason::kPIN_FORGOTTEN
          ? QObject::tr("Resetting removes the PIN and lets the application "
                        "start, but everything it previously encrypted becomes "
                        "permanently unreadable.")
          : QObject::tr("Resetting lets the application start, but everything "
                        "it previously encrypted becomes permanently "
                        "unreadable."));
  box.setInformativeText(
      reason == ProfileKeyResetReason::kPIN_FORGOTTEN
          ? QObject::tr("Only do this if you cannot recall the PIN. There is "
                        "no other way to recover the key.")
          : QObject::tr("You can unlock the keychain and start the application "
                        "again instead. That costs nothing and leaves the key "
                        "intact."));
  auto* back = box.addButton(QObject::tr("Go Back"), QMessageBox::RejectRole);
  auto* reset = box.addButton(QObject::tr("Reset to Default"),
                              QMessageBox::DestructiveRole);
  box.setDefaultButton(back);
  box.exec();

  if (box.clickedButton() != reset) return false;

  return QMessageBox::warning(
             nullptr, QObject::tr("Reset to Default"),
             QObject::tr("Everything the application has encrypted with the "
                         "current key will be permanently unreadable.") +
                 "\n\n" + QObject::tr("Reset the secure key?"),
             QMessageBox::Yes | QMessageBox::Cancel,
             QMessageBox::Cancel) == QMessageBox::Yes;
}

}  // namespace

auto GuiProfileLoaderDelegate::AskPackagePassphrase(const QString& package,
                                                    bool retry)
    -> std::optional<GFBuffer> {
  if (retry) {
    QMessageBox::warning(nullptr, QObject::tr("Open Profile"),
                         QObject::tr("That passphrase did not open this file."),
                         QMessageBox::Ok);
  }

  bool accepted = false;
  auto entered = QInputDialog::getText(
      nullptr, QObject::tr("Open Profile"),
      QObject::tr("Enter the passphrase that protects this file:") + "\n" +
          QDir::toNativeSeparators(package),
      QLineEdit::Password, {}, &accepted);
  if (!accepted || entered.isEmpty()) return {};

  GFBuffer passphrase(entered);
  entered.fill('X');
  entered.clear();
  return passphrase;
}

auto GuiProfileLoaderDelegate::AskAppKeyPin(const AppKeyPinRequest& request)
    -> AppKeyPinAnswer {
  if (request.creating) {
    AppKeyPinDialog dialog(AppKeyPinDialog::Mode::kSET);
    if (dialog.exec() != QDialog::Accepted) return {};
    return {AppKeyPinAnswer::Action::kUsePin, dialog.Pin()};
  }

  // One dialog per attempt, so the loader owns the retry loop and this only has
  // to render one question. The error text is set last, because clearing the
  // field emits a change that hides it.
  AppKeyPinDialog dialog(AppKeyPinDialog::Mode::kUNLOCK);
  if (request.failures > 0) {
    auto message = QObject::tr(
        "That PIN did not unlock the application key. Please try again.");
    if (request.failures >= kRevealResetAfter) {
      message += "\n" + QObject::tr(
                            "If you have forgotten your PIN, the application "
                            "key and everything encrypted with it cannot be "
                            "recovered.");
      dialog.RevealResetOption();
    }
    dialog.SetErrorText(message);
  }

  const int code = dialog.exec();

  if (code == AppKeyPinDialog::kResetRequested) {
    if (!ConfirmDestructiveReset(ProfileKeyResetReason::kPIN_FORGOTTEN)) {
      // Backing out of the reset is not backing out of the unlock: report a
      // failed attempt so the loader asks again.
      return {AppKeyPinAnswer::Action::kUsePin, {}};
    }
    return {AppKeyPinAnswer::Action::kResetKey};
  }

  // Anything other than accept is a Quit: it costs nothing and leaves the key
  // intact, so it is never overloaded to mean reset.
  if (code != QDialog::Accepted) return {};

  return {AppKeyPinAnswer::Action::kUsePin, dialog.Pin()};
}

auto GuiProfileLoaderDelegate::ConfirmForceUnlock(const ProfileLockResult& held)
    -> bool {
  const auto held_by =
      held.pid != 0
          ? QObject::tr("It is open in process %1 on %2.")
                .arg(held.pid)
                .arg(held.host.isEmpty() ? QObject::tr("this computer")
                                         : held.host)
          : QObject::tr("Another process has it open.");

  QMessageBox box(
      QMessageBox::Warning, QObject::tr("Profile Is Already Open"),
      QObject::tr("This profile is already open in another window.") + "\n\n" +
          held_by + "\n\n" +
          QObject::tr("Opening it twice would corrupt its stored data.") +
          "\n\n" + QObject::tr("Profile: %1").arg(held.path));
  auto* quit = box.addButton(QObject::tr("Quit"), QMessageBox::AcceptRole);
  auto* force =
      box.addButton(QObject::tr("Force Unlock"), QMessageBox::DestructiveRole);
  box.setDefaultButton(quit);
  box.exec();

  if (box.clickedButton() != force) return false;

  // Deliberately a second, separate confirmation: if the holder is in fact
  // alive, this reintroduces exactly the concurrent-write window the lock
  // exists to prevent.
  return QMessageBox::warning(
             nullptr, QObject::tr("Force Unlock"),
             QObject::tr("Only do this if you are certain no other GpgFrontend "
                         "window has this profile open.") +
                 "\n\n" +
                 QObject::tr("If one does, both copies will corrupt the "
                             "profile's stored data."),
             QMessageBox::Cancel | QMessageBox::Yes,
             QMessageBox::Cancel) == QMessageBox::Yes;
}

auto GuiProfileLoaderDelegate::ConfirmKeyReset(ProfileKeyResetReason reason)
    -> bool {
  if (reason == ProfileKeyResetReason::kKEYCHAIN_SECRET_LOST) {
    QMessageBox box(QMessageBox::Critical,
                    QObject::tr("Secure Key Unavailable"),
                    QObject::tr("The application key is protected by a secret "
                                "kept in the system keychain, and that secret "
                                "could not be read."));
    box.setInformativeText(
        QObject::tr("This usually means the keychain is locked, was reset, or "
                    "this profile was copied from another computer or user "
                    "account.") +
        "\n\n" +
        QObject::tr("You can unlock the keychain and start the application "
                    "again. Resetting the key instead lets the application "
                    "start, but everything it previously encrypted becomes "
                    "permanently unreadable."));

    // This is the most destructive choice the application ever puts in front
    // of anyone, and until now it was the only credential store dialog that
    // gave no way to find out what actually broke. A keyring that refused to
    // unlock and a profile carried over from another machine read identically
    // in the prose above; only the store's own message separates them.
    if (const auto detail = SystemSecretStoreReason(); !detail.isEmpty()) {
      box.setDetailedText(detail);
    }

    auto* quit = box.addButton(QObject::tr("Quit"), QMessageBox::RejectRole);
    auto* reset = box.addButton(QObject::tr("Reset Secure Key"),
                                QMessageBox::DestructiveRole);
    box.setDefaultButton(quit);
    box.exec();

    if (box.clickedButton() != reset) return false;
  }

  return ConfirmDestructiveReset(reason);
}

void GuiProfileLoaderDelegate::Report(const ProfileLoadError& error) {
  const auto subject = QDir::toNativeSeparators(error.subject);

  switch (error.failure) {
    case ProfileLoadFailure::kCANCELLED:
      // The user backed out on purpose; saying so again would be nagging.
      return;

    case ProfileLoadFailure::kSELECTION_INVALID:
      QMessageBox::critical(nullptr, QObject::tr("Cannot Open Profile"),
                            error.detail, QMessageBox::Ok);
      return;

    case ProfileLoadFailure::kALREADY_OPEN:
      QMessageBox::critical(
          nullptr, QObject::tr("Profile Is Already Open"),
          QObject::tr("This profile is already open in another window.") +
              "\n\n" + subject,
          QMessageBox::Ok);
      return;

    case ProfileLoadFailure::kLOCK_UNAVAILABLE:
      QMessageBox::critical(
          nullptr, QObject::tr("Cannot Lock Profile"),
          QObject::tr("The lock file at %1 could not be created.")
                  .arg(subject) +
              "\n" + QObject::tr("Please check your storage and permissions."),
          QMessageBox::Ok);
      return;

    case ProfileLoadFailure::kNOT_A_PACKAGE:
    case ProfileLoadFailure::kPACKAGE_MALFORMED:
    case ProfileLoadFailure::kMOUNT_FAILED:
      QMessageBox::critical(nullptr, QObject::tr("Cannot Open Profile"),
                            error.detail + "\n\n" + subject, QMessageBox::Ok);
      return;

    case ProfileLoadFailure::kPACKAGE_TAMPERED:
      QMessageBox::critical(nullptr, QObject::tr("This File Has Been Altered"),
                            error.detail + "\n\n" + subject, QMessageBox::Ok);
      return;

    case ProfileLoadFailure::kTOO_NEW:
    case ProfileLoadFailure::kUPGRADE_REFUSED: {
      const auto writer =
          error.actor.isEmpty() ? QObject::tr("a newer version") : error.actor;
      QMessageBox::critical(
          nullptr,
          error.failure == ProfileLoadFailure::kTOO_NEW
              ? QObject::tr("Profile Is Too New")
              : QObject::tr("Profile Cannot Be Opened"),
          QObject::tr("This application data was last used by %1, which stores "
                      "it in a format this version does not understand.")
                  .arg(writer) +
              "\n\n" + error.detail + "\n\n" +
              QObject::tr("Continuing would damage it. Please use %1 or later, "
                          "or start this version with a different profile.")
                  .arg(writer) +
              "\n\n" + QObject::tr("Data folder: %1").arg(subject),
          QMessageBox::Ok);
      return;
    }

    case ProfileLoadFailure::kUPGRADE_FAILED:
      QMessageBox::critical(
          nullptr, QObject::tr("Profile Upgrade Failed"),
          QObject::tr("Upgrading this profile stopped at step '%1'.")
                  .arg(error.actor) +
              "\n\n" + error.detail + "\n\n" +
              QObject::tr("The profile is intact. Please report this."),
          QMessageBox::Ok);
      return;

    case ProfileLoadFailure::kKEY_WRITE_FAILED:
      // Continuing would be silent data loss: everything encrypted during this
      // session gets a key ID that no longer exists on the next start, so it
      // would come back as unreadable rather than merely unsaved.
      QMessageBox::critical(
          nullptr, QObject::tr("Save Key Failed"),
          QObject::tr("The application secure key could not be saved: %1")
                  .arg(error.detail) +
              "\n" +
              QObject::tr("Anything saved now would be unreadable after a "
                          "restart, so the application will not continue. "
                          "Please check your storage and permissions."),
          QMessageBox::Ok);
      return;

    case ProfileLoadFailure::kKEY_DECRYPT_FAILED:
      // A wrong PIN is caught and retried before the key loader ever runs, so
      // reaching here means the key file itself will not decrypt.
      QMessageBox::critical(
          nullptr, QObject::tr("App Secure Key Error"),
          QObject::tr("Failed to decrypt the application secure key. The key "
                      "file may be corrupted.") +
              "\n" + QObject::tr("Please clear the secure key and try again."),
          QMessageBox::Ok);
      return;

    case ProfileLoadFailure::kKEY_READ_FAILED:
      QMessageBox::critical(
          nullptr, QObject::tr("App Secure Key Error"),
          QObject::tr(
              "Failed to read the application secure key from disk at: %1")
                  .arg(error.detail) +
              "\n" +
              QObject::tr("Please ensure the key file exists and is "
                          "accessible, or try re-initializing the secure key."),
          QMessageBox::Ok);
      return;

    case ProfileLoadFailure::kKEY_GENERATE_FAILED:
      QMessageBox::critical(
          nullptr, QObject::tr("Secure Key Generation Failed"),
          QObject::tr("Failed to generate an application secure key.") + "\n" +
              QObject::tr("Please check your system's cryptography support."),
          QMessageBox::Ok);
      return;

    case ProfileLoadFailure::kKEY_IO_FAILED:
      QMessageBox::critical(
          nullptr, QObject::tr("App Secure Key Error"),
          QObject::tr("The application secure key at %1 could not be read or "
                      "rewritten.")
                  .arg(error.detail) +
              "\n" + QObject::tr("Please check your storage and permissions."),
          QMessageBox::Ok);
      return;

    case ProfileLoadFailure::kKEY_LOCKED_OUT:
      // The offer to reset was already made and declined; quitting leaves the
      // key intact so the keychain can be unlocked and the start retried.
      return;
  }
}

void GuiProfileLoaderDelegate::Note(ProfileLoadNotice notice,
                                    const QString& detail) {
  switch (notice) {
    case ProfileLoadNotice::kKEYCHAIN_UNAVAILABLE: {
      QMessageBox box(
          QMessageBox::Warning, QObject::tr("System Keychain Unavailable"),
          QObject::tr("The application key could not be protected using the "
                      "system keychain, so it remains stored unprotected.") +
              "\n" +
              QObject::tr("This setting has been turned off. You can turn it "
                          "on again once a keychain is available."));
      box.addButton(QMessageBox::Ok);

      // The user asked for keychain protection and is being silently dropped
      // back to none, so this is exactly the moment they need to be able to
      // find out what broke rather than guess.
      auto reason = SystemSecretStoreReason();
      if (reason.isEmpty()) reason = detail;
      if (!reason.isEmpty()) box.setDetailedText(reason);

      box.exec();
      return;
    }

    case ProfileLoadNotice::kPIN_SET_BUT_KEY_PLAINTEXT:
      QMessageBox::warning(
          nullptr, QObject::tr("Application Key Not Protected"),
          QObject::tr("A PIN is configured, but the application key on disk is "
                      "not encrypted.") +
              "\n" +
              QObject::tr("This can happen if a previous change was "
                          "interrupted. The PIN setting has been turned off "
                          "and the key is left as it is."),
          QMessageBox::Ok);
      return;

    case ProfileLoadNotice::kKEY_WAS_RESET:
      // Both paths that reach here already confirmed the reset twice; saying it
      // a third time after the fact helps nobody.
      qWarning() << "the profile secure key was reset:" << detail;
      return;
  }
}

}  // namespace GpgFrontend::UI
