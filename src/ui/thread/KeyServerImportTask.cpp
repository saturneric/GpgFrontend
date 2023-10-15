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
 * The source code version of this software was modified and released
 * by Saturneric<eric@bktus.com><eric@bktus.com> starting on May 12, 2021.
 *
 */

#include "ui/thread/KeyServerImportTask.h"

#include <vector>

GpgFrontend::UI::KeyServerImportTask::KeyServerImportTask(
    std::string keyserver_url, std::vector<std::string> keyids)
    : Task("key_server_import_task"),
      keyserver_url_(std::move(keyserver_url)),
      keyids_(std::move(keyids)),
      manager_(new QNetworkAccessManager(this)) {}

void GpgFrontend::UI::KeyServerImportTask::run() {
  HoldOnLifeCycle(true);

  QUrl keyserver_url = QUrl(keyserver_url_.c_str());
  for (const auto& key_id : keyids_) {
    QUrl req_url(keyserver_url.scheme() + "://" + keyserver_url.host() +
                 "/pks/lookup?op=get&search=0x" + key_id.c_str() +
                 "&options=mr");

    reply_ = manager_->get(QNetworkRequest(req_url));

    connect(reply_, &QNetworkReply::finished, this,
            &KeyServerImportTask::dealing_reply_from_server);
  }
}

void GpgFrontend::UI::KeyServerImportTask::dealing_reply_from_server() {
  QByteArray buffer;
  QNetworkReply::NetworkError network_reply = reply_->error();
  if (network_reply == QNetworkReply::NoError) {
    buffer = reply_->readAll();
  }
  emit SignalKeyServerImportResult(network_reply, buffer);

  if (result_count_++ == keyids_.size() - 1) {
    emit SignalTaskRunnableEnd(0);
  }
}