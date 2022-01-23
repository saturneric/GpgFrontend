/**
 * Copyright (C) 2021 Saturneric
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
 * Saturneric<eric@bktus.com> starting on May 12, 2021.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#ifndef GPGFRONTEND_SETTINGSKEYSERVER_H
#define GPGFRONTEND_SETTINGSKEYSERVER_H

#include "ui/GpgFrontendUI.h"

class Ui_KeyServerSettings;

namespace GpgFrontend::UI {
/**
 * @brief
 *
 */
class KeyserverTab : public QWidget {
  Q_OBJECT

 public:
  /**
   * @brief Construct a new Keyserver Tab object
   *
   * @param parent
   */
  explicit KeyserverTab(QWidget* parent = nullptr);

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

 private:
  std::shared_ptr<Ui_KeyServerSettings> ui_;
  QString default_key_server_;
  QStringList key_server_str_list_;
  QMenu* popup_menu_{};

  QRegularExpression url_reg_{
      R"(^https?:\/\/(www\.)?[-a-zA-Z0-9@:%._\+~#=]{1,256}\.[a-zA-Z0-9()]{1,6}\b([-a-zA-Z0-9()@:%_\+.~#?&//=]*)$)"};

 private slots:

  /**
   * @brief
   *
   */
  void slot_add_key_server();

  /**
   * @brief
   *
   */
  void slot_refresh_table();

  /**
   * @brief
   *
   */
  void slot_test_listed_key_server();

 signals:
  /**
   * @brief
   *
   * @param needed
   */
  void SignalRestartNeeded(bool needed);

 protected:
  /**
   * @brief
   *
   * @param event
   */
  void contextMenuEvent(QContextMenuEvent* event) override;
};
}  // namespace GpgFrontend::UI

#endif  // GPGFRONTEND_SETTINGSKEYSERVER_H
