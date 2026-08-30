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

#include "core/profile/ProfilePackage.h"
#include "ui/dialog/GeneralDialog.h"
#include "ui/dialog/profile/ProfileExportSummary.h"

class QDialogButtonBox;
class QLabel;
class QPushButton;

namespace GpgFrontend::UI {

class MetaListPanel;
class SecretEntryPanel;

/**
 * @brief Choose where a profile is written, and what protects it.
 *
 * The contents are listed with their sizes because the one thing a user cannot
 * guess is how big their own workspace has become — and the workspace is
 * exactly where the cleartext of things they meant to encrypt tends to live, so
 * it is named rather than swept in.
 *
 * The passphrase is typed into the same SecretEntryPanel the application key's
 * PIN uses, so that a profile file's protection is offered with the same
 * seriousness, the same reveal toggle and the same floor rather than as two
 * bare fields at the bottom of a form. It is not optional: a profile file
 * carries the profile's own key, so an unsealed one hands over everything the
 * profile ever encrypted, and this dialog no longer offers that.
 */
class ProfileExportDialog : public GeneralDialog {
  Q_OBJECT

 public:
  /**
   * @brief Construct the dialog.
   *
   * @param display_name what the profile is called
   * @param storage the session's storage, asked for the size figures
   * @param parent parent widget
   */
  ProfileExportDialog(QString display_name, const ProfileAccessor& storage,
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
   * @brief What will protect the package: always a passphrase.
   *
   * Kept as a call rather than dropped, because the packing request carries the
   * field and the reader still understands unsealed files written by older
   * builds.
   *
   * @return kPIN
   */
  [[nodiscard]] auto Protection() const -> ProfilePackageProtection;

  /**
   * @brief The passphrase the file will be sealed with.
   *
   * @return the passphrase
   */
  [[nodiscard]] auto Passphrase() const -> GFBuffer;

 private slots:
  void slot_choose_destination();
  void slot_state_changed();

 private:
  QString display_name_;
  QMap<QString, qint64> areas_;

  QLabel* destination_hint_{};         ///< shown until a destination is chosen
  MetaListPanel* destination_list_{};  ///< shown once one is
  QPushButton* destination_button_{};

  MetaListPanel* contents_{};
  bool include_workspace_ = false;

  SecretEntryPanel* entry_{};

  QLabel* warning_icon_{};
  QLabel* warning_label_{};
  QLabel* summary_label_{};
  QDialogButtonBox* buttons_{};

  QString destination_;
  qint64 free_bytes_ = -1;

  void init_ui();

  /// @brief Build the "save to" group: the rows, and the button that fills
  /// them.
  void build_destination(QVBoxLayout* layout);

  /// @brief Build the "what goes in" group from BuildProfileExportContents().
  void build_contents(QVBoxLayout* layout);

  /// @brief Build the protection group: the fields, and how they are used.
  void build_protection(QVBoxLayout* layout);

  /// @brief The contents rows for the current choice.
  [[nodiscard]] auto contents_rows() const -> QVector<MetaListRow>;

  /// @brief What has been chosen so far, for the pure rules.
  [[nodiscard]] auto choice() const -> ProfileExportChoice;
};

}  // namespace GpgFrontend::UI
