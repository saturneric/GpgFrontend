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

#include <array>

#include "core/profile/ProfilePackage.h"
#include "ui/dialog/profile/ProfileExportSummary.h"

class QCheckBox;
class QLabel;
class QLineEdit;
class QPushButton;

namespace GpgFrontend::UI {

class MetaListPanel;
class ProfileExportPage;
class SecretEntryPanel;

/**
 * @brief Export a profile in three steps: what goes in, what protects it, and
 * where it is written.
 *
 * One screen used to hold all three at equal weight, and the decision that
 * matters -- the passphrase -- sat third in a column of four cards. Each step
 * now answers one question, with the detail that backs it listed beneath.
 *
 * A QWizard, so the navigation, the button order and the look are the
 * platform's own and match the first-run wizard. The pages live for the
 * wizard's lifetime, so Back and Continue never lose what was typed or ticked.
 *
 * The passphrase is typed into the same SecretEntryPanel the application key's
 * PIN uses. It is not optional: a profile file carries the profile's own key,
 * so an unsealed one hands over everything the profile ever encrypted, and
 * this wizard does not offer that.
 */
class ProfileExportDialog : public QWizard {
  Q_OBJECT

 public:
  /**
   * @brief Construct the wizard.
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
  void slot_page_changed(int id);

 private:
  QString display_name_;
  QMap<QString, qint64> areas_;

  std::array<ProfileExportPage*, kProfileExportStepCount> pages_{};

  // Contents
  QLabel* contents_summary_{};
  QCheckBox* workspace_box_{};
  MetaListPanel* contents_{};
  bool include_workspace_ = false;

  // Protection
  SecretEntryPanel* entry_{};

  // Save
  MetaListPanel* review_{};
  QLineEdit* destination_edit_{};
  QPushButton* destination_button_{};
  QLabel* free_space_label_{};
  QLabel* warning_label_{};

  QString destination_;
  qint64 free_bytes_ = -1;

  /// @brief Step 1: the profile, a one-line summary, the workspace choice, and
  /// the itemised list.
  void build_contents_page();

  /// @brief Step 2: the passphrase, and the mechanism that uses it.
  void build_protection_page();

  /// @brief Step 3: what is about to happen, and where.
  void build_save_page();

  /// @brief A page for a step, titled and registered under the step's id.
  auto add_page(ProfileExportStep step) -> ProfileExportPage*;

  /// @brief The contents rows for the current choice.
  [[nodiscard]] auto contents_rows() const -> QList<MetaListRow>;

  /// @brief What has been chosen so far, for the pure rules.
  [[nodiscard]] auto choice() const -> ProfileExportChoice;
};

}  // namespace GpgFrontend::UI
