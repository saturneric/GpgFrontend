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

#include <QtNetwork>

class Ui_NetworkSettings;

namespace GpgFrontend::UI {
class NetworkTab : public QWidget {
  Q_OBJECT

 public:
  /**
   * @brief Construct a new Network Tab object
   *
   * @param parent
   */
  explicit NetworkTab(QWidget* parent = nullptr);

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

 private slots:

  /**
   * @brief
   *
   */
  void slot_test_proxy_connection_result();

 private:
  QSharedPointer<Ui_NetworkSettings> ui_;                           ///<
  QNetworkProxy::ProxyType proxy_type_ = QNetworkProxy::HttpProxy;  ///<

  /**
   * @brief
   *
   */
  void apply_proxy_settings();

  /**
   * @brief
   *
   * @param enabled
   */
  void switch_ui_enabled(bool enabled);

  /**
   * @brief
   *
   * @param type_text
   */
  void switch_ui_proxy_type(const QString& type_text);
};
}  // namespace GpgFrontend::UI
