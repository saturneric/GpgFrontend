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

#include "core/thread/Task.h"
#include "core/typedef/GpgTypedef.h"

namespace GpgFrontend {
class GpgImportInformation;
}

namespace GpgFrontend::UI {

class KeyServerImportTask : public Thread::Task {
  Q_OBJECT
 public:
  /**
   * @brief Construct a new Key Server Search Task object
   *
   * @param keyserver_url
   * @param search_string
   */
  KeyServerImportTask(QString keyserver_url, int channel, KeyIdArgsList keyids);

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
  void SignalKeyServerImportResult(int, bool, QString, QByteArray,
                                   std::shared_ptr<GpgImportInformation>);

 private slots:

  /**
   * @brief
   *
   */
  void dealing_reply_from_server();

 private:
  QString keyserver_url_;            ///<
  int current_gpg_context_channel_;  ///<
  KeyIdArgsList keyids_;             ///<
  int result_count_ = 0;

  QNetworkAccessManager *manager_;  ///<
  QNetworkReply *reply_;            ///<
};
}  // namespace GpgFrontend::UI
