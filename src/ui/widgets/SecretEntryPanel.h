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
#include "ui/dialog/SecretPrompt.h"
#include "ui/widgets/SecretEntryState.h"

class QCheckBox;
class QGridLayout;
class QLabel;
class QLineEdit;
class QProgressBar;
class QPushButton;

namespace GpgFrontend::UI {

/**
 * @brief The fields a secret is typed into, and everything that belongs beside
 * them.
 *
 * Extracted from AppKeyPinDialog so the export dialog can hold exactly the same
 * cluster inside a group box. Composition rather than a shared base class,
 * because the two dialogs that need this do not share one: AppKeyPinDialog is a
 * QDialog and ProfileExportDialog is a GeneralDialog. A widget is the only seam
 * that fits both, which also makes that divergence stop mattering.
 *
 * What it carries: the fields, a reveal toggle that flips all of them at once,
 * a strength meter, an optional generator, and a message row whose height is
 * reserved so that showing an error never moves anything else on screen.
 */
class GF_UI_EXPORT SecretEntryPanel : public QWidget {
  Q_OBJECT

 public:
  /**
   * @brief Which fields exist, and what everything is called.
   *
   * Structure and wording arrive together because the panel needs both and
   * neither is derivable from the other: whether a confirmation field exists is
   * a decision, and what to call the secret in it is a different one.
   */
  struct Config {
    bool ask_current = false;  ///< a field for a secret that already exists
    bool ask_new = true;       ///< a new secret, its confirmation, and strength

    /// Offer to invent a secret. Only worth showing where the secret is going
    /// to be written down rather than remembered — which is a profile file's
    /// passphrase, and not the PIN typed at every startup.
    bool offer_generation = false;

    SecretPromptTexts texts;
  };

  /**
   * @brief Build the panel.
   *
   * @param config which fields to show and what to call them
   * @param parent parent widget
   */
  explicit SecretEntryPanel(Config config, QWidget* parent = nullptr);

  /// @brief Wipe the fields on the way out; see Clear().
  ~SecretEntryPanel() override;

  /**
   * @brief The secret the user chose or entered.
   *
   * @return the new secret, or the existing one when no new secret is asked for
   */
  [[nodiscard]] auto Secret() const -> GFBuffer;

  /**
   * @brief The existing secret, where one was asked for.
   *
   * @return the current secret, or an empty buffer
   */
  [[nodiscard]] auto CurrentSecret() const -> GFBuffer;

  /**
   * @brief Whether what has been typed satisfies the configuration.
   *
   * @return true when it can be accepted
   */
  [[nodiscard]] auto Acceptable() const -> bool;

  /**
   * @brief Show why the previous attempt failed, for a retry loop.
   *
   * @param text message to display, or empty to fall back to the idle hint
   */
  void SetErrorText(const QString& text);

  /**
   * @brief Wipe every field.
   *
   * Each field is overwritten with filler of the same length before it is
   * cleared, as the prompts this replaces already did by hand. Honest about
   * what that achieves: QString is implicitly shared and Qt's own input
   * machinery may already hold copies, so this narrows the window in which a
   * secret sits in ordinary heap memory rather than closing it. That is also
   * why Secret() hands the value straight into a GFBuffer, which is secure
   * storage and can actually be zeroed.
   */
  void Clear();

  /// @brief Put the caret in the first field the user has to fill.
  void FocusFirstField();

 signals:
  /// @brief Something was typed. Carries no secret, by design: a signal
  /// argument is copied to every connection, and none of them needs the value.
  void SignalStateChanged();

 private:
  /// @brief Wipe every field without announcing it.
  ///
  /// Split out of Clear() because the destructor needs the wipe and must not
  /// have the announcement; see the comment there.
  void scrub();

  /// @brief Build one password field, sized and cleared like all the others.
  auto make_field() -> QLineEdit*;

  /// @brief Update the strength bar from the new-secret field.
  void refresh_strength();

  /// @brief Restore the idle guidance: the configured hint, dimmed, so the
  /// reserved space reads as advice rather than an empty gap and never as an
  /// alarm.
  void show_default_hint();

  /// @brief Re-read the fields, re-render the message row, and announce it.
  void refresh();

  /// @brief Fill both fields with a secret nobody has to invent.
  void generate();

  Config config_;

  QLineEdit* current_edit_{};
  QLineEdit* new_edit_{};
  QLineEdit* confirm_edit_{};
  QCheckBox* reveal_box_{};
  QPushButton* generate_button_{};
  QProgressBar* strength_bar_{};
  QLabel* strength_label_{};
  QLabel* message_label_{};

  SecretEntryState state_;
};

}  // namespace GpgFrontend::UI
