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

#include "ui/dialog/SecretPrompt.h"

#include "ui/dialog/AppKeyPinDialog.h"

namespace GpgFrontend::UI {

namespace {

/// Whether the mode has the user choose a secret rather than recall one. The
/// distinction decides three things at once — whether a confirmation field
/// exists, whether the length floor applies, and whether the irreversibility
/// warning is shown — so it is worth naming rather than repeating.
auto ChoosesASecret(SecretPromptMode mode) -> bool {
  return mode != SecretPromptMode::kUNLOCK;
}

/**
 * @brief Wording for the application key kept on this computer.
 *
 * Every string here goes through AppKeyPinDialog::tr() rather than
 * QObject::tr(), and that is not incidental. These twenty strings are already
 * translated into eight languages under the context
 * GpgFrontend::UI::AppKeyPinDialog; naming any other context would orphan all
 * of them at once, silently, with the prompts falling back to English. The
 * strings themselves are reproduced verbatim from the dialog they came from and
 * pinned by GFUiSecretPromptTextsTest, so generalising the dialog cannot
 * quietly reword a prompt that was already in front of users.
 */
auto AppKeyTexts(SecretPromptMode mode) -> SecretPromptTexts {
  SecretPromptTexts texts;

  switch (mode) {
    case SecretPromptMode::kUNLOCK:
      texts.window_title = AppKeyPinDialog::tr("Unlock Application Key");
      break;
    case SecretPromptMode::kCHANGE:
      texts.window_title = AppKeyPinDialog::tr("Change Application PIN");
      break;
    case SecretPromptMode::kSET:
      texts.window_title = AppKeyPinDialog::tr("Set an Application PIN");
      break;
  }

  texts.subtitle =
      ChoosesASecret(mode)
          ? AppKeyPinDialog::tr(
                "This PIN encrypts the application key on disk. You will be "
                "asked for it every time the application starts.")
          : AppKeyPinDialog::tr(
                "This application's key is protected by a PIN. Enter it to "
                "continue.");

  texts.current_label = mode == SecretPromptMode::kCHANGE
                            ? AppKeyPinDialog::tr("Current PIN")
                            : AppKeyPinDialog::tr("PIN");
  texts.new_label = mode == SecretPromptMode::kCHANGE
                        ? AppKeyPinDialog::tr("New PIN")
                        : AppKeyPinDialog::tr("PIN");
  texts.confirm_label = AppKeyPinDialog::tr("Confirm");
  texts.reveal_label = AppKeyPinDialog::tr("Show PIN");
  texts.strength_caption = AppKeyPinDialog::tr("Strength");

  texts.accept_button = mode == SecretPromptMode::kUNLOCK
                            ? AppKeyPinDialog::tr("Unlock")
                            : AppKeyPinDialog::tr("OK");

  // Unlocking only ever happens at startup, where cancelling closes the
  // application rather than merely dismissing a dialog. Name the button after
  // what it does so the consequence is not a surprise.
  if (mode == SecretPromptMode::kUNLOCK) {
    texts.cancel_button = AppKeyPinDialog::tr("Quit");
  }

  texts.hint =
      ChoosesASecret(mode)
          ? AppKeyPinDialog::tr("Use at least %1 characters.")
                .arg(kMinSecretLength)
          : AppKeyPinDialog::tr("This PIN cannot be recovered if it is lost.");

  // The one thing a user must understand before choosing a PIN: there is no
  // recovery path, because the key is encrypted with it and nothing else. Never
  // shown when unlocking, where that choice was made long ago.
  if (ChoosesASecret(mode)) {
    texts.warning = AppKeyPinDialog::tr(
        "If you forget this PIN, everything the application has encrypted "
        "becomes permanently unreadable. There is no recovery.");
  }

  texts.too_short_message =
      AppKeyPinDialog::tr("The PIN must be at least %1 characters.");
  texts.mismatch_message = AppKeyPinDialog::tr("The two PINs do not match.");
  // Deliberately left empty. A kCHANGE form with nothing in the current-PIN
  // field has always simply kept its accept button disabled and its hint up:
  // the user is mid-way through filling in a form, which is not yet a mistake
  // worth naming.
  texts.current_missing_message = {};

  return texts;
}

/**
 * @brief Wording for a profile file's passphrase.
 *
 * Deliberately never says "PIN". These are new strings with no translations to
 * preserve, so they go through QObject::tr() and take their own context.
 */
auto ProfilePackageTexts(SecretPromptMode mode, const QString& context)
    -> SecretPromptTexts {
  SecretPromptTexts texts;

  switch (mode) {
    case SecretPromptMode::kUNLOCK:
      texts.window_title = QObject::tr("Open a Profile File");
      texts.subtitle = QObject::tr(
          "This file is protected by a passphrase. Enter it to "
          "read what is inside.");
      texts.accept_button = QObject::tr("Open");
      texts.hint = QObject::tr(
          "Nothing in this file can be read until the passphrase opens it.");
      break;

    case SecretPromptMode::kSET:
    case SecretPromptMode::kCHANGE:
      texts.window_title = QObject::tr("Protect This Profile File");
      // Said because it is otherwise unsaid: this prompt does not verify the
      // old passphrase, it replaces it. Someone re-typing what they believe the
      // file already uses can hand it a new one without ever being told.
      texts.subtitle = QObject::tr(
          "This is the passphrase the file will be sealed with from now on. If "
          "you enter a different one, the old one will no longer open it.");
      texts.accept_button = QObject::tr("Save");
      texts.hint = QObject::tr(
          "Keep this passphrase. The file cannot be opened without it.");
      texts.warning = QObject::tr(
          "If you lose this passphrase, this file can never be opened again. "
          "There is no recovery.");
      break;
  }

  texts.context_caption = QObject::tr("File");
  texts.context = context;

  texts.current_label = QObject::tr("Passphrase");
  texts.new_label = QObject::tr("Passphrase");
  texts.confirm_label = QObject::tr("Repeat");
  texts.reveal_label = QObject::tr("Show passphrase");
  texts.strength_caption = QObject::tr("Strength");

  texts.too_short_message =
      QObject::tr("The passphrase must be at least %1 characters.");
  texts.mismatch_message = QObject::tr("The two entries do not match.");
  texts.current_missing_message = {};

  return texts;
}

}  // namespace

auto DefaultSecretPromptTexts(SecretPromptSubject subject,
                              SecretPromptMode mode, const QString& context)
    -> SecretPromptTexts {
  return subject == SecretPromptSubject::kAppKey
             ? AppKeyTexts(mode)
             : ProfilePackageTexts(mode, context);
}

}  // namespace GpgFrontend::UI
