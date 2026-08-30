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

#pragma once

#include <optional>

#include "core/model/GFBuffer.h"
#include "ui/dialog/SecretPrompt.h"

class QLabel;
class QPushButton;

namespace GpgFrontend::UI {

class SecretEntryPanel;

/**
 * @brief Ask for a secret that protects something on disk.
 *
 * Deliberately not PassphraseDialog: that one is bound to a
 * GpgPassphraseContext and carries a 120 second abort timer sized for gpgme's
 * pinentry callback, which would cut short a settings dialog that is waiting on
 * the answer. Only the strength scoring is shared, via PassphraseStrength.h.
 *
 * The dialog never touches the secret's subject. The caller decides what an
 * answer means — unlocking the application key at startup, re-sealing it from
 * the Advanced tab, or opening a profile file somebody sent.
 *
 * The name is narrower than what it now does, and that is on purpose. Its
 * twenty strings are translated into eight languages under the context
 * GpgFrontend::UI::AppKeyPinDialog, and a class rename would silently orphan
 * every one of them; what the dialog is *for* is carried by
 * SecretPromptSubject instead.
 */
class GF_UI_EXPORT AppKeyPinDialog : public QDialog {
  Q_OBJECT

 public:
  /// What the dialog is being opened for. An alias rather than its own enum, so
  /// that AppKeyPinDialog::Mode::kSET still resolves at every existing caller
  /// while the wording table can speak about modes without depending on a
  /// dialog.
  using Mode = SecretPromptMode;

  /// exec() result, alongside QDialog::Accepted (1) / Rejected (0), meaning the
  /// user asked to reset the key to its default unprotected state instead of
  /// unlocking it. Only reachable in kUNLOCK, and only after
  /// RevealResetOption(). The dialog performs no reset itself — the caller
  /// confirms and carries it out.
  static constexpr int kResetRequested = 2;

  /**
   * @brief Ask for the application key's PIN, with the wording that has always
   * gone with it.
   *
   * @param mode what to ask for
   * @param parent parent widget
   */
  explicit AppKeyPinDialog(Mode mode, QWidget* parent = nullptr);

  /**
   * @brief Ask for some other secret, in the same shape.
   *
   * @param mode what to ask for
   * @param texts what to call it; see DefaultSecretPromptTexts()
   * @param parent parent widget
   */
  AppKeyPinDialog(Mode mode, SecretPromptTexts texts,
                  QWidget* parent = nullptr);

  /**
   * @brief Run one prompt for a profile file's passphrase.
   *
   * The retry message goes inside the dialog rather than into a message box
   * ahead of it: the correction is being typed here, so this is where the
   * reason for it belongs.
   *
   * @param parent parent widget, may be null at startup
   * @param mode kUNLOCK to open a file, kSET to seal one
   * @param retry true when a previous answer did not open it
   * @param context_rows the file, as BuildProfilePackageRows() describes it
   * @return the passphrase, or nothing when the user gave up
   */
  static auto AskPackagePassphrase(QWidget* parent, Mode mode, bool retry,
                                   QVector<MetaListRow> context_rows)
      -> std::optional<GFBuffer>;

  /**
   * @brief The PIN the user chose or entered.
   *
   * In kCHANGE this is the new PIN; see CurrentPin() for the old one.
   *
   * @return the PIN, or an empty buffer when the dialog was rejected
   */
  [[nodiscard]] auto Pin() const -> GFBuffer;

  /**
   * @brief The existing PIN, in kCHANGE only.
   *
   * @return the current PIN, or an empty buffer in the other modes
   */
  [[nodiscard]] auto CurrentPin() const -> GFBuffer;

  /**
   * @brief Show why the previous attempt failed, for a retry loop.
   *
   * @param text message to display, or empty to clear it
   */
  void SetErrorText(const QString& text);

  /// @brief Wipe every field.
  void Clear();

  /**
   * @brief Reveal the "reset to default" escape hatch (kUNLOCK only).
   *
   * Hidden until the caller decides the user is stuck — typically after a few
   * failed attempts — so the destructive option is not put in front of someone
   * who simply mistyped once. Clicking it ends the dialog with kResetRequested;
   * a no-op in the other modes, and never offered for a file, where there is
   * nothing this application could reset.
   */
  void RevealResetOption();

 protected:
  /// @brief Centre the dialog on its screen once its final size is known.
  ///
  /// The unlock prompt is shown parentless at startup, so without this the
  /// window manager decides where it lands. Positioning here — not in the
  /// constructor — waits until the layout has settled its size.
  void showEvent(QShowEvent* event) override;

 private:
  /// @brief Everything both constructors do, once.
  void init_ui();

  Mode mode_;
  SecretPromptTexts texts_;
  SecretEntryPanel* entry_{};
  QPushButton* accept_button_{};
  QPushButton*
      reset_button_{};  ///< "reset to default" escape hatch; kUNLOCK only
};

}  // namespace GpgFrontend::UI
