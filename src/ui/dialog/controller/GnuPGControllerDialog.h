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

#include "core/struct/settings_object/KeyDatabaseItemSO.h"
#include "ui/dialog/GeneralDialog.h"

class Ui_GnuPGControllerDialog;

namespace GpgFrontend::UI {
class GnuPGControllerDialog : public GeneralDialog {
  Q_OBJECT
 public:
  /**
   * @brief Construct a new General Tab object
   *
   * @param parent
   */
  explicit GnuPGControllerDialog(QWidget* parent = nullptr);

 public slots:

  /**
   * @brief
   *
   */
  void SlotAccept();

 signals:

  /**
   * @brief
   *
   * @param needed
   */
  void SignalRestartNeeded(int);

 private slots:

  /**
   * @brief
   *
   * @param needed
   */
  void slot_set_restart_needed(int);

  /**
   * @brief
   *
   */
  void slot_update_custom_gnupg_install_path_label(int state);

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
  void slot_move_down_key_database();

  /**
   * @brief
   *
   */
  void slot_move_to_top_key_database();

  /**
   * @brief
   *
   */
  void slot_edit_key_database();

 private:
  std::shared_ptr<Ui_GnuPGControllerDialog> ui_;  ///<
  int restart_mode_{0};                           ///<
  QString custom_key_database_path_;
  QString custom_gnupg_path_;
  QMenu* popup_menu_{};
  QList<KeyDatabaseItemSO> buffered_key_db_so_;

  /**
   * @brief Get the Restart Needed object
   *
   * @return true
   * @return false
   */
  [[nodiscard]] auto get_restart_needed() const -> int;

  /**
   * @brief Set the settings object
   *
   */
  void set_settings();

  /**
   * @brief
   *
   */
  void apply_settings();

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  auto check_custom_gnupg_path(QString) -> bool;

 protected:
  /**
   * @brief
   *
   * @param event
   */
  void contextMenuEvent(QContextMenuEvent* event) override;
};
}  // namespace GpgFrontend::UI
