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

#include "ui/dialog/GeneralDialog.h"

class QLineEdit;
class QLabel;
class QPushButton;

namespace GpgFrontend::UI {

/**
 * @brief Edit the display name, email and comment of an existing key group.
 *
 * Membership is not touched here; KeyGroupManageDialog owns that. Kept as its
 * own modal so the manage dialog's header stays readable and so a rename can
 * be offered from elsewhere later.
 */
class KeyGroupEditDialog : public GeneralDialog {
  Q_OBJECT
 public:
  /**
   * @brief Construct the editor pre-filled with a group's current metadata.
   *
   * @param name current display name
   * @param email current email address
   * @param comment current comment
   * @param parent parent widget
   */
  explicit KeyGroupEditDialog(const QString& name, const QString& email,
                              const QString& comment,
                              QWidget* parent = nullptr);

  /**
   * @brief The edited display name.
   */
  [[nodiscard]] auto Name() const -> QString;

  /**
   * @brief The edited email address.
   */
  [[nodiscard]] auto Email() const -> QString;

  /**
   * @brief The edited comment.
   */
  [[nodiscard]] auto Comment() const -> QString;

 protected:
  void showEvent(QShowEvent* event) override;

 private:
  QLineEdit* name_;
  QLineEdit* email_;
  QLineEdit* comment_;
  QLabel* error_label_;
  QPushButton* save_button_;

  // Re-run validation and gate the save button on the result.
  void update_validation_state();
};

}  // namespace GpgFrontend::UI
