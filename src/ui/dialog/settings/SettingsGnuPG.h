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

class Ui_GnuPGSettings;

namespace GpgFrontend::UI {
class GnuPGTab : public QWidget {
  Q_OBJECT

 public:
  /**
   * @brief Construct a new GnuPG Tab object
   *
   * @param parent
   */
  explicit GnuPGTab(QWidget* parent = nullptr);

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

 private slots:

  /**
   * @brief
   *
   */
  void slot_update_custom_gnupg_install_path_label(int state);

 private:
  QSharedPointer<Ui_GnuPGSettings> ui_;  ///<
  QString custom_gnupg_path_;

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  auto check_custom_gnupg_path(const QString& path) -> bool;
};
}  // namespace GpgFrontend::UI
