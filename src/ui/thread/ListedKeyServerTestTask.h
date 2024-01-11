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

#include "GpgFrontendUI.h"
#include "core/thread/ThreadingModel.h"

class QNetworkAccessManager;
class QNetworkReply;

namespace GpgFrontend::UI {

/**
 * @brief
 *
 */
class ListedKeyServerTestTask : public Thread::Task {
  Q_OBJECT
 public:
  enum KeyServerTestResultType {
    kTestResultType_Success,
    kTestResultType_Timeout,
    kTestResultType_Error,
  };

  explicit ListedKeyServerTestTask(const QStringList& urls, int timeout,
                                   QWidget* parent = nullptr);

  /**
   * @brief
   *
   */
  void Run() override;

 signals:
  /**
   * @brief
   *
   * @param result
   */
  void SignalKeyServerListTestResult(
      std::vector<KeyServerTestResultType> result);

 private:
  QStringList urls_;                             ///<
  std::vector<KeyServerTestResultType> result_;  ///<
  QNetworkAccessManager* network_manager_;       ///<
  int timeout_ = 500;                            ///<
  int result_count_ = 0;                         ///<

  /**
   * @brief
   *
   * @param index
   * @param reply
   */
  void slot_process_network_reply(int index, QNetworkReply* reply);
};

}  // namespace GpgFrontend::UI

class TestListedKeyServerThread {};
