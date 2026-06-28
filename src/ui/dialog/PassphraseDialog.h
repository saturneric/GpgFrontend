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

#include "core/model/GpgPassphraseContext.h"

namespace GpgFrontend::UI {
class PassphraseDialog : public QDialog {
  Q_OBJECT
 public:
  /**
   * @brief Construct a new Passphrase Dialog object
   *
   * @param ctx
   * @param parent
   */
  explicit PassphraseDialog(const QSharedPointer<GpgPassphraseContext>& ctx,
                            QWidget* parent = nullptr);

  /**
   * @brief Get the entered passphrase
   *
   * @return QString
   */
  [[nodiscard]] auto Passphrase() const -> GFBuffer;

  /**
   * @brief Clear the passphrase input fields and context.
   */
  void Clear();

 private:
  /// Abort the operation if the user does not respond within this window.
  static constexpr int kDefaultTimeoutSeconds = 120;

  QSharedPointer<GpgPassphraseContext> ctx_;
  QLineEdit* password_edit_ = nullptr;
  QLineEdit* confirm_password_edit_ = nullptr;
  QCheckBox* show_password_checkbox_ = nullptr;
  QProgressBar* passphrase_strength_bar_ = nullptr;
  QLabel* passphrase_strength_label_ = nullptr;
  QLabel* timeout_label_ = nullptr;
  QTimer* timeout_timer_ = nullptr;
  int remaining_seconds_ = kDefaultTimeoutSeconds;

  [[nodiscard]] auto validate_passphrase_input() -> bool;
  void update_passphrase_strength(const QString& text);
  void update_timeout_label();
};
}  // namespace GpgFrontend::UI