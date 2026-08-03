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
class QRadioButton;
class QDialogButtonBox;

namespace GpgFrontend::UI {

/**
 * @brief Ask for the name and the keyring choice of a new profile.
 *
 * The keyring question is asked rather than defaulted because both answers are
 * wrong for somebody. An empty keyring is isolated and can be packaged, but the
 * profile opens with no keys at all, which reads as "my keys are gone". The
 * system keyring is familiar, but the profile is then not really separate and
 * its keys cannot travel inside a package.
 */
class ProfileCreateDialog : public GeneralDialog {
  Q_OBJECT

 public:
  /**
   * @brief Construct the dialog.
   *
   * @param taken_ids ids already in use, so a collision is caught while typing
   * rather than on accept
   * @param parent parent widget
   */
  explicit ProfileCreateDialog(QStringList taken_ids, QWidget* parent);

  /// Sanitised id, valid only after the dialog was accepted.
  [[nodiscard]] auto Id() const -> QString { return id_; }

  /// Free-form display name.
  [[nodiscard]] auto DisplayName() const -> QString;

  /// Whether the profile keeps its own keyring rather than the system one.
  [[nodiscard]] auto SelfContained() const -> bool;

 private slots:
  void slot_name_changed();
  void slot_accept();

 private:
  QStringList taken_ids_;
  QString id_;

  QLineEdit* name_edit_{};
  QLabel* id_label_{};
  QRadioButton* own_keyring_{};
  QRadioButton* system_keyring_{};
  QDialogButtonBox* buttons_{};

  void init_ui();
};

}  // namespace GpgFrontend::UI
