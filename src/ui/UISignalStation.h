/**
 * Copyright (C) 2021 Saturneric <eric@bktus.com>
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

#include "ui/widgets/InfoBoardWidget.h"

namespace GpgFrontend::UI {

/**
 * @brief
 *
 */
class UISignalStation : public QObject {
  Q_OBJECT
  static std::unique_ptr<UISignalStation> instance;

 public:
  /**
   * @brief Get the Instance object
   *
   * @return SignalStation*
   */
  static auto GetInstance() -> UISignalStation*;

 signals:
  /**
   * @brief
   *
   */
  void SignalKeyDatabaseRefresh();

  /**
   * @brief
   *
   */
  void SignalKeyDatabaseRefreshDone();

  /**
   * @brief
   *
   */
  void SignalUIRefresh();

  /**
   * @brief
   *
   * @param text
   * @param verify_label_status
   */
  void SignalRefreshInfoBoard(const QString& text,
                              InfoBoardStatus verify_label_status);

  /**
   * @brief
   *
   * @param message
   * @param timeout
   */
  void SignalRefreshStatusBar(const QString& message, int timeout);

  /**
   * @brief
   *
   */
  void SignalUserInputPassphraseDone(QString passparase);

  /**
   * @brief
   *
   */
  void SignalNeedUserInputPassphrase();

  /**
   * @brief
   *
   */
  void SignalRestartApplication(int);
};

}  // namespace GpgFrontend::UI
