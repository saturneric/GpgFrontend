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

#include <optional>

#include "core/function/SystemSecretStore.h"
#include "core/profile/ProfilePackage.h"
#include "core/profile/ProfileSession.h"
#include "ui/dialog/AppKeyPinDialog.h"
#include "ui/dialog/MetaListDialog.h"
#include "ui/dialog/profile/ProfilePackageMeta.h"
#include "ui/function/ProfileController.h"

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
  // The header is read here rather than passed in: the core delegate interface
  // hands this only a path and a retry flag, and widening it to carry a parsed
  // header would be a core API change in service of a label. Reading it costs
  // one cheap open, and gf_ui links gf_core anyway.
  //
  // Whatever it says is shown as a claim and never as a fact. The header sits
  // outside the sealed payload, so anyone holding the file can write anything
  // into it; the point of showing it at all is that a file whose claims look
  // wrong is worth a second thought before a passphrase is typed at it.
  const auto inspection = InspectProfilePackage(package);

  // The retry message goes inside the prompt rather than into a box in front of
  // it: the correction is typed here, so this is where the reason for it
  // belongs, and one dialog to dismiss beats two.
  return AppKeyPinDialog::AskPackagePassphrase(
      nullptr, AppKeyPinDialog::Mode::kUNLOCK, retry,
      BuildProfilePackageRows(QFileInfo(package), inspection.header,
                              inspection.Ok()));
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

auto BuildProfileLockConflictTexts(const ProfileLockResult& held)
    -> ProfileLockConflictTexts {
  const auto held_by =
      held.pid != 0
          ? QObject::tr("Process %1 on %2")
                .arg(held.pid)
                .arg(held.host.isEmpty() ? QObject::tr("this computer")
                                         : held.host)
          : QObject::tr("Another process");

  return {
      QObject::tr("Profile Is Already Open"),
      QObject::tr("Opening it twice would corrupt its stored data."),
      {
          {.caption = QObject::tr("Profile"), .value = held.path, .path = true},
          {.caption = QObject::tr("Held by"), .value = held_by},
      },
      // What forcing the lock actually risks. This used to be a second dialog
      // behind the Force Unlock button; it says the same thing here, where it
      // is read before the choice rather than after it, and where neither
      // answer costs a second box to dismiss.
      QObject::tr("Only do this if you are certain no other GpgFrontend "
                  "window has this profile open. If one does, both copies "
                  "will corrupt the profile's stored data."),
  };
}

auto GuiProfileLoaderDelegate::ConfirmForceUnlock(const ProfileLockResult& held)
    -> bool {
  const auto texts = BuildProfileLockConflictTexts(held);

  MetaListDialog dialog(texts.title, texts.subtitle);
  dialog.AddSection({}, texts.rows);
  dialog.AddNote(texts.note, /*danger=*/true);
  // Index 0, so MetaListDialog::AddButton makes it the default -- the same
  // safe default the old QMessageBox::setDefaultButton(quit) gave it.
  dialog.AddButton(QObject::tr("Quit"), QDialogButtonBox::AcceptRole);
  const auto force = dialog.AddButton(QObject::tr("Force Unlock"),
                                      QDialogButtonBox::DestructiveRole);
  dialog.exec();

  return dialog.Choice() == force;
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

  if (reason == ProfileKeyResetReason::kKEY_FILE_MISSING) {
    const auto key_path = ProfileSession::Instance().Accessor().PathOf(
        ProfileArea::kSecure, "app.key");

    QMessageBox box(QMessageBox::Critical, QObject::tr("Secure Key Missing"),
                    QObject::tr("The application key file is missing, but this "
                                "profile still holds settings and data "
                                "encrypted with it."));
    box.setInformativeText(
        QObject::tr("This usually means the data folder was restored or "
                    "copied without the key file.") +
        "\n\n" +
        QObject::tr("You can quit, put the key file back from your backup, "
                    "and start the application again. Creating a new key "
                    "instead lets the application start, but everything it "
                    "previously encrypted becomes permanently unreadable."));
    box.setDetailedText(QObject::tr("Expected key file: %1").arg(key_path));

    auto* quit = box.addButton(QObject::tr("Quit"), QMessageBox::RejectRole);
    auto* reset = box.addButton(QObject::tr("Create New Key"),
                                QMessageBox::DestructiveRole);
    box.setDefaultButton(quit);
    box.exec();

    if (box.clickedButton() != reset) return false;
  }

  return ConfirmDestructiveReset(reason);
}

void GuiProfileLoaderDelegate::Report(const ProfileLoadError& error) {
  const auto text = DescribeProfileLoadError(error);
  if (!text) return;
  QMessageBox::critical(nullptr, text->title, text->text, QMessageBox::Ok);
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
