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

#include <qnetworkaccessmanager.h>
#include <qnetworkreply.h>

#include "GpgFrontendUI.h"
#include "core/thread/ThreadingModel.h"

namespace GpgFrontend::UI {
class KeyServerSearchTask : public Thread::Task {
  Q_OBJECT
 public:
  /**
   * @brief Construct a new Key Server Search Task object
   *
   * @param keyserver_url
   * @param search_string
   */
  KeyServerSearchTask(QString keyserver_url, QString search_string);

  /**
   * @brief
   *
   */
  auto Run() -> int override;

 signals:

  /**
   * @brief
   *
   * @param result
   */
  void SignalKeyServerSearchResult(QNetworkReply::NetworkError reply,
                                   QString err_string, QByteArray buffer);

 private slots:

  void dealing_reply_from_server();

 private:
  QString keyserver_url_;  ///<
  QString search_string_;  ///<

  QNetworkAccessManager *manager_;  ///<
  QNetworkReply *reply_;            ///<
};

}  // namespace GpgFrontend::UI
