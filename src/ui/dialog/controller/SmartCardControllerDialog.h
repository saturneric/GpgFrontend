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

#include "core/model/GpgOpenPGPCard.h"
#include "ui/dialog/GeneralDialog.h"

class Ui_SmartCardControllerDialog;

namespace GpgFrontend::UI {
class SmartCardControllerDialog : public GeneralDialog {
  Q_OBJECT
 public:
  /**
   * @brief Construct a new Smart Card Controller Dialog object
   *
   * @param parent
   */
  explicit SmartCardControllerDialog(QWidget* parent = nullptr);

 private slots:

  /**
   * @brief
   *
   */
  void slot_refresh();

  /**
   * @brief
   *
   */
  void slot_listen_smart_card_changes();

  /**
   * @brief
   *
   * @param disable
   */
  void slot_disable_controllers(bool disable);

  /**
   * @brief
   *
   */
  void slot_fetch_smart_card_keys();

 private:
  QSharedPointer<Ui_SmartCardControllerDialog> ui_;  ///<
  int channel_;
  bool has_card_ = false;
  GpgOpenPGPCard card_info_;
  QString cached_status_hash_;
  QTimer* timer_ = nullptr;
  bool scd_version_supported_ = false;

  /**
   * @brief Apply every translatable string.
   *
   * The strings in the .ui file are Designer-time placeholders only.
   */
  void init_texts();

  /**
   * @brief Build the drop-down menus of the action bar.
   */
  void init_actions();

  /**
   * @brief Wire up every signal of the dialog.
   */
  void init_connections();

  /**
   * @brief Get the smart card serial number object
   *
   */
  void select_smart_card_by_serial_number(const QString& serial_number);

  /**
   * @brief
   *
   */
  void fetch_smart_card_info(const QString& serial_number);

  /**
   * @brief Fill the detail pane from card_info_.
   */
  void render_card_info();

  /**
   * @brief Fill the identity form and the header chips.
   */
  void render_identity();

  /**
   * @brief Fill the retry counters, UIF and status form.
   */
  void render_status();

  /**
   * @brief Fill the on-card key table.
   */
  void render_card_keys();

  /**
   * @brief Fill the extended capability and additional info forms.
   */
  void render_capabilities();

  /**
   * @brief
   *
   */
  void refresh_key_tree_view(int channel);

  /**
   * @brief
   *
   */
  void reset_status();

  /**
   * @brief
   *
   * @param attr
   */
  void modify_key_attribute(const QString& attr);

  /**
   * @brief
   *
   * @param attr
   */
  void modify_key_pin(const QString& pinref);
};
}  // namespace GpgFrontend::UI
