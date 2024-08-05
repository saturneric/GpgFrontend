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

#include "ui/thread/KeyServerSearchTask.h"

#include "core/utils/BuildInfoUtils.h"

GpgFrontend::UI::KeyServerSearchTask::KeyServerSearchTask(QString keyserver_url,
                                                          QString search_string)
    : Task("key_server_search_task"),
      keyserver_url_(std::move(keyserver_url)),
      search_string_(std::move(search_string)),
      manager_(new QNetworkAccessManager(this)) {
  HoldOnLifeCycle(true);
}

auto GpgFrontend::UI::KeyServerSearchTask::Run() -> int {
  QUrl url_from_remote = keyserver_url_ +
                         "/pks/lookup?search=" + search_string_ +
                         "&op=index&options=mr";

  auto request = QNetworkRequest(url_from_remote);
  request.setHeader(QNetworkRequest::UserAgentHeader,
                    GetHttpRequestUserAgent());

  reply_ = manager_->get(request);
  connect(reply_, &QNetworkReply::finished, this,
          &KeyServerSearchTask::dealing_reply_from_server);

  return 0;
}

void GpgFrontend::UI::KeyServerSearchTask::dealing_reply_from_server() {
  QByteArray buffer;
  QNetworkReply::NetworkError network_reply = reply_->error();
  if (network_reply == QNetworkReply::NoError) {
    buffer = reply_->readAll();
  }

  LOG_D() << "reply from key server:" << network_reply
          << "err string:" << reply_->errorString()
          << "reply:" << QString::fromLatin1(buffer);

  emit SignalKeyServerSearchResult(network_reply, reply_->errorString(),
                                   buffer);
  emit SignalTaskShouldEnd(0);
}
