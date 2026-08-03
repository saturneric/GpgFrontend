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

#include "core/function/ProfilePackage.h"
#include "ui/dialog/GeneralDialog.h"

class QLineEdit;
class QLabel;
class QCheckBox;
class QRadioButton;
class QDialogButtonBox;
class QPushButton;

namespace GpgFrontend::UI {

/**
 * @brief Choose where a profile is written, and what protects it.
 *
 * The contents are listed with their sizes because the one thing a user cannot
 * guess is how big their own workspace has become — and the workspace is
 * exactly where the cleartext of things they meant to encrypt tends to live, so
 * it is named rather than swept in.
 */
class ProfileExportDialog : public GeneralDialog {
  Q_OBJECT

 public:
  /**
   * @brief Construct the dialog.
   *
   * @param display_name what the profile is called
   * @param profile_root root being exported, for the size figures
   * @param parent parent widget
   */
  ProfileExportDialog(QString display_name, QString profile_root,
                      QWidget* parent);

  /**
   * @brief Where the package should be written.
   *
   * @return absolute path
   */
  [[nodiscard]] auto DestinationPath() const -> QString;

  /**
   * @brief Whether the user's own files travel too.
   *
   * @return true when the workspace is included
   */
  [[nodiscard]] auto IncludeWorkspace() const -> bool;

  /**
   * @brief What will protect the package.
   *
   * @return the chosen protection
   */
  [[nodiscard]] auto Protection() const -> ProfilePackageProtection;

  /**
   * @brief The passphrase, when one was chosen.
   *
   * @return the passphrase, empty when unprotected
   */
  [[nodiscard]] auto Passphrase() const -> GFBuffer;

 private slots:
  void slot_choose_destination();
  void slot_state_changed();
  void slot_accept();

 private:
  QString display_name_;
  QString profile_root_;
  QMap<QString, qint64> areas_;

  QLabel* destination_label_{};
  QPushButton* destination_button_{};
  QLabel* contents_label_{};
  QCheckBox* workspace_box_{};
  QRadioButton* protect_with_pin_{};
  QRadioButton* protect_with_nothing_{};
  QLineEdit* passphrase_edit_{};
  QLineEdit* confirm_edit_{};
  QLabel* protection_hint_{};
  QDialogButtonBox* buttons_{};

  QString destination_;

  void init_ui();

  /**
   * @brief The contents summary, sizes included.
   *
   * @return rich text for the contents label
   */
  [[nodiscard]] auto describe_contents() const -> QString;
};

}  // namespace GpgFrontend::UI
