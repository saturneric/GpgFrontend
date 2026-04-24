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
  QContainer<KeyDatabaseInfo> key_db_infos_;
  bool is_sandbox_ = false;

  /**
   * @brief
   *
   */
  void slot_refresh_key_database_table();

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
  void slot_remove_existing_key_database();
};
}  // namespace GpgFrontend::UI
