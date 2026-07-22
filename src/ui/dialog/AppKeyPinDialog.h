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

#include "core/model/GFBuffer.h"

class QLabel;
class QLineEdit;
class QProgressBar;
class QPushButton;

namespace GpgFrontend::UI {

/**
 * @brief Ask for the PIN that protects the application secure key file.
 *
 * Deliberately not PassphraseDialog: that one is bound to a
 * GpgPassphraseContext and carries a 120 second abort timer sized for gpgme's
 * pinentry callback, which would cut short a settings dialog that is waiting on
 * the answer. Only the strength scoring is shared, via PassphraseStrength.h.
 *
 * The dialog never touches the key itself. The caller decides what a PIN means
 * — unlocking the file at startup, or re-sealing it from the Advanced tab.
 */
class GF_UI_EXPORT AppKeyPinDialog : public QDialog {
  Q_OBJECT

 public:
  /// What the dialog is being opened for.
  enum class Mode {
    kSET,     ///< choose a new PIN: new field, confirmation, strength meter
    kUNLOCK,  ///< enter the existing PIN: one field, retryable
    kCHANGE,  ///< current PIN, then a new one with confirmation
  };

  /// exec() result, alongside QDialog::Accepted (1) / Rejected (0), meaning the
  /// user asked to reset the key to its default unprotected state instead of
  /// unlocking it. Only reachable in kUNLOCK, and only after
  /// RevealResetOption(). The dialog performs no reset itself — the caller
  /// confirms and carries it out.
  static constexpr int kResetRequested = 2;

  /**
   * @brief Construct the dialog.
   *
   * @param mode what to ask for
   * @param parent parent widget
   */
  explicit AppKeyPinDialog(Mode mode, QWidget* parent = nullptr);

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
   * a no-op in the other modes.
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
  /// @brief Enable the accept button only when the input satisfies the mode.
  void refresh_validity();

  /// @brief Update the strength bar from the new-PIN field.
  void refresh_strength();

  /// @brief Restore the idle guidance in the message row: the mode's hint, in a
  /// dimmed neutral colour, so the reserved space reads as advice rather than
  /// an empty gap and never as an alarm.
  void show_default_hint();

  Mode mode_;
  QString hint_text_;          ///< dimmed guidance shown when there is no error
  QLineEdit* current_edit_{};  ///< existing PIN; kCHANGE and kUNLOCK
  QLineEdit* new_edit_{};      ///< new PIN; kSET and kCHANGE
  QLineEdit* confirm_edit_{};  ///< repeat of new_edit_; kSET and kCHANGE
  QProgressBar* strength_bar_{};
  QLabel* strength_label_{};
  QLabel* error_label_{};
  QPushButton* accept_button_{};
  QPushButton*
      reset_button_{};  ///< "reset to default" escape hatch; kUNLOCK only
};

}  // namespace GpgFrontend::UI
