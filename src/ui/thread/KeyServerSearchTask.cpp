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

#include "ui/thread/KeyServerSearchTask.h"

#include <utility>

GpgFrontend::UI::KeyServerSearchTask::KeyServerSearchTask(
    std::string keyserver_url, std::string search_string)
    : Task("key_server_search_task"),
      keyserver_url_(std::move(keyserver_url)),
      search_string_(std::move(search_string)),
      manager_(new QNetworkAccessManager(this)) {
  HoldOnLifeCycle(true);
}

void GpgFrontend::UI::KeyServerSearchTask::run() {
  QUrl url_from_remote =
      QString::fromStdString(keyserver_url_) +
      "/pks/lookup?search=" + QString::fromStdString(search_string_) +
      "&op=index&options=mr";

  reply_ = manager_->get(QNetworkRequest(url_from_remote));

  connect(reply_, &QNetworkReply::finished, this,
          &KeyServerSearchTask::dealing_reply_from_server);
}

void GpgFrontend::UI::KeyServerSearchTask::dealing_reply_from_server() {
  QByteArray buffer;
  QNetworkReply::NetworkError network_reply = reply_->error();
  if (network_reply == QNetworkReply::NoError) {
    buffer = reply_->readAll();
  }
  emit SignalKeyServerSearchResult(network_reply, buffer);
  emit SignalTaskShouldEnd(0);
}
