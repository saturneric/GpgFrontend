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

#include "ui/dialog/key_generate/KeyGenerateSummary.h"
#include "ui/struct/settings_object/KeyGenSO.h"

namespace GpgFrontend::UI {

/**
 * @brief Settings, Key Generation: the profiles, and what Quick Generate does.
 *
 * Two tabs. Quick Generate holds the default profile and the user ID patterns;
 * Profiles holds the list the key generation wizard offers, in its order.
 *
 * The default profile is one setting with two uses: the wizard preselects it
 * and Quick Generate makes it. Both tabs show and change that one value, and
 * each follows the other at once.
 */
class KeyGenerationTab : public QWidget {
  Q_OBJECT

 public:
  explicit KeyGenerationTab(QWidget* parent = nullptr);

  /**
   * @brief Show what is stored, dropping any edits.
   */
  void SetSettings();

  /**
   * @brief Store what is shown.
   */
  void ApplySettings();

 protected:
  /// @brief A dropped row: moves the profile it stands for.
  auto eventFilter(QObject* watched, QEvent* event) -> bool override;

 private:
  int channel_;  ///< the key database whose engine decides what is offered
  KeyGenSO settings_;
  QString default_name_;  ///< the profile the default resolves to here
  quint64 preview_seed_;  ///< the example's draw, kept until asked for another

  // Quick Generate
  QComboBox* default_combo_;
  QLabel* default_note_;
  QLineEdit* name_pattern_;
  QLineEdit* email_pattern_;
  QLineEdit* comment_pattern_;
  QLabel* name_problem_;
  QLabel* email_problem_;
  QLabel* comment_problem_;
  QLabel* preview_;
  QLabel* preview_note_;

  // Profiles
  QTableWidget* profile_table_;
  QLabel* detail_title_;
  QLabel* detail_signing_;
  QLabel* detail_encryption_;
  QLabel* detail_validity_;
  QLabel* detail_availability_;
  // One action each, shared by the toolbar, the menus and the keys.
  QAction* new_action_;
  QAction* edit_action_;
  QAction* duplicate_action_;
  QAction* default_action_;
  QAction* hide_action_;
  QAction* delete_action_;
  QAction* up_action_;
  QAction* down_action_;

  auto build_quick_tab() -> QWidget*;
  auto build_profiles_tab() -> QWidget*;

  [[nodiscard]] auto current_pattern() const -> UidPattern;

  /// @brief The row selected, or -1.
  [[nodiscard]] auto current_row() const -> int;

  /// @brief Whether the profile in @p row is the default as it resolves here.
  [[nodiscard]] auto is_default_profile(int row) const -> bool;

  [[nodiscard]] auto resolve(const KeyGenProfile& profile) const
      -> std::optional<ResolvedKeyGenProfile>;

  [[nodiscard]] auto facts(const KeyGenProfile& profile) const
      -> KeyGenProfileFacts;

  void refresh_profiles(int select);
  void refresh_default_combo();
  void refresh_profile_detail();
  void refresh_pattern();

  /// @brief The edited profile, or nothing when the dialog was cancelled.
  auto edit(const ResolvedKeyGenProfile& start, int ignore_index)
      -> std::optional<KeyGenProfile>;

  void slot_new();
  void slot_edit();
  void slot_duplicate();
  void slot_delete();
  void slot_set_default();
  void slot_toggle_hidden();
  void slot_move(int delta);
  void slot_restore_defaults();
};

}  // namespace GpgFrontend::UI
