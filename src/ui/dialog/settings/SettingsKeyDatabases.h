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

#include "core/model/KeyDatabaseInfo.h"
#include "core/typedef/CoreTypedef.h"

class Ui_KeyDatabasesSettings;

namespace GpgFrontend::UI {
class KeyList;

/**
 * @brief
 *
 */
class KeyDatabasesTab : public QWidget {
  Q_OBJECT

 public:
  /**
   * @brief Construct a new General Tab object
   *
   * @param parent
   */
  explicit KeyDatabasesTab(QWidget* parent = nullptr);

  /**
   * @brief Set the Settings object
   *
   */
  void SetSettings();

  /**
   * @brief
   *
   */
  void ApplySettings();

 signals:

  /**
   * @brief
   *
   * @param needed
   */
  void SignalDeepRestartNeeded();

 protected:
  /**
   * @brief
   *
   * @param event
   */
  void contextMenuEvent(QContextMenuEvent* event) override;

 private:
  QSharedPointer<Ui_KeyDatabasesSettings> ui_;  ///<
  QMenu* popup_menu_{};
  const QString app_path_;
  QContainer<KeyDatabaseInfo> active_key_db_infos_;

  /// The three kinds, kept apart rather than in one list, because what may be
  /// done to a key database follows entirely from which one it is. A flat list
  /// is what let a folder be picked for a database whose folder is derived, and
  /// let the derived one be dragged out of channel 0.
  ///
  /// The DEFAULT database is present exactly when the user has it turned on; it
  /// appears in neither table, because there is nothing about it to show in a
  /// row that the checkbox does not say better.
  std::optional<KeyDatabaseInfo> default_db_info_;
  QContainer<KeyDatabaseInfo> managed_db_infos_;
  QContainer<KeyDatabaseInfo> external_db_infos_;

  bool is_sandbox_ = false;

  /// True while SetSettings() is populating the widgets, so the checkbox's own
  /// toggled() handler does not mistake being restored for being clicked.
  bool loading_ = false;

  /**
   * @brief Fill one table from one of the lists. Shared, so the two tables
   * cannot drift apart in what they show.
   */
  void fill_table(QTableWidget* table,
                  const QContainer<KeyDatabaseInfo>& key_databases,
                  int first_channel, int selected_row);

  /**
   * @brief
   *
   */
  void slot_refresh_key_database_table(int selected_row = -1);

  /// The table the user is looking at, and the list behind it. Every row action
  /// works on the visible tab, so nothing can move between the two lists.
  [[nodiscard]] auto current_table() const -> QTableWidget*;
  auto current_list() -> QContainer<KeyDatabaseInfo>*;

  /// The selected row of the visible table, or -1.
  [[nodiscard]] auto current_row() const -> int;

  /// How many key databases the profile holds in total, the DEFAULT one
  /// included. The cap and the never-empty rule are both about this number.
  [[nodiscard]] auto total_count() const -> int;

  /// Put the DEFAULT checkbox and its hint in step with what the engine on this
  /// computer actually offers.
  void refresh_default_key_database_ui();

  /**
   * @brief
   *
   */
  void slot_open_key_database();

  /**
   * @brief
   *
   */
  void slot_move_up_key_database();

  /**
   * @brief
   *
   */
  void slot_move_to_top_key_database();

  /**
   * @brief
   *
   */
  void slot_move_down_key_database();

  /**
   * @brief
   *
   */
  void slot_edit_key_database();

  /**
   * @brief
   *
   */
  void slot_add_new_key_database();

  /**
   * @brief
   *
   */
  void slot_add_external_key_database();

  /**
   * @brief
   *
   */
  void slot_remove_existing_key_database();

  /**
   * @brief React to the DEFAULT key database being turned on or off.
   */
  void slot_toggle_default_key_database(bool checked);
};

}  // namespace GpgFrontend::UI
