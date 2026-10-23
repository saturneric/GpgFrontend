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

#include "core/function/openpgp/KeyDatabaseRuntime.h"
#include "core/model/KeyDatabaseInfo.h"
#include "core/typedef/CoreTypedef.h"
#include "ui/dialog/settings/KeyDatabaseSettingsModel.h"
#include "ui/widgets/ElidedPathLabel.h"

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

  /**
   * @brief The managed database folders a rename has to move, handed over to
   * be carried out when the change is applied.
   */
  auto TakePendingMoves() -> QContainer<KeyDatabaseMove>;

  /**
   * @brief The databases in use whose operations would be left running on a
   * database this page removes or points elsewhere.
   *
   * Such work is not stopped by applying: it finishes on the database it was
   * started on, which is no longer the one shown under that name, or no longer
   * shown at all. A database whose folder stays where it is shares that folder
   * with what replaces it, so its work is not counted.
   *
   * @return one line per such database, naming it and its operation count
   */
  [[nodiscard]] auto OperationsLeftRunning() const -> QStringList;

 signals:

  /**
   * @brief
   *
   * @param needed
   */
  void SignalRuntimeApplyNeeded();

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
  /// The built-in database is present exactly when the user has it turned on;
  /// it has a section of its own rather than a row. The model also holds the
  /// default choice and the built-in database's engine, until OK.
  KeyDatabaseSettingsModel model_;

  ElidedPathLabel* builtin_location_label_{};
  QAction* use_as_default_action_{};
  QAction* copy_folder_path_action_{};

  bool is_sandbox_ = false;

  /// Folders renamed in this page and not yet moved; see TakePendingMoves().
  QContainer<KeyDatabaseMove> pending_moves_;

  void record_move(const KeyDatabaseInfo& info, const QString& to);

  [[nodiscard]] auto pending_move_for(const QString& id) const
      -> const KeyDatabaseMove*;

  void drop_pending_move(const QString& id);

  /// True while SetSettings() is populating the widgets, so the checkbox's own
  /// toggled() handler does not mistake being restored for being clicked.
  bool loading_ = false;

  /**
   * @brief Fill one table from one of the lists. Shared, so the two tables
   * cannot drift apart in what they show.
   */
  void fill_table(QTableWidget* table,
                  const QContainer<KeyDatabaseInfo>& key_databases,
                  int selected_row);

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

  /// Whether @p path is the directory of a database already on this page, the
  /// DEFAULT one included, however either is spelled. @p self is the entry
  /// being edited, which may keep its own directory.
  [[nodiscard]] auto location_taken(const QString& path,
                                    const KeyDatabaseInfo* self = nullptr) const
      -> bool;

  /// Put the built-in section in step with what the engine on this computer
  /// offers and with the pending engine choice.
  void refresh_default_key_database_ui();

  /// The id of the selected row of the visible table, empty when none.
  [[nodiscard]] auto selected_id() const -> QString;

  /// Every "Default" marker and "Use as Default" control, from the model. Run
  /// after anything that can change which database is the default.
  void refresh_role_controls();

  /// Make @p id the default, pending until OK.
  void use_as_default(const QString& id);

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
