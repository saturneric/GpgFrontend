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

#include "ListedKeyServerTestTask.h"

#include <QtNetwork>
#include <vector>

GpgFrontend::UI::ListedKeyServerTestTask::ListedKeyServerTestTask(
    const QStringList& urls, int timeout, QWidget* parent)
    : Task("listed_key_server_test_task"),
      urls_(urls),
      timeout_(timeout),
      network_manager_(new QNetworkAccessManager(this)),
      result_(urls_.size(), kTestResultType_Error) {
  qRegisterMetaType<std::vector<KeyServerTestResultType>>(
      "std::vector<KeyServerTestResultType>");
}

void GpgFrontend::UI::ListedKeyServerTestTask::run() {
  HoldOnLifeCycle(true);

  size_t index = 0;
  for (const auto& url : urls_) {
    auto key_url = QUrl{url};
    SPDLOG_DEBUG("key server request: {}", key_url.host().toStdString());

    auto* network_reply = network_manager_->get(QNetworkRequest{key_url});
    auto* timer = new QTimer(this);

    connect(network_reply, &QNetworkReply::finished, this,
            [this, index, network_reply]() {
              SPDLOG_DEBUG("key server domain reply: {}",
                           urls_[index].toStdString());
              this->slot_process_network_reply(index, network_reply);
            });

    connect(timer, &QTimer::timeout, this, [this, index, network_reply]() {
      SPDLOG_DEBUG("timeout for key server: {}", urls_[index].toStdString());
      if (network_reply->isRunning()) {
        network_reply->abort();
        this->slot_process_network_reply(index, network_reply);
      }
    });

    timer->start(timeout_);

    index++;
  }
}

void GpgFrontend::UI::ListedKeyServerTestTask::slot_process_network_reply(
    int index, QNetworkReply* reply) {
  if (!reply->isRunning() && reply->error() == QNetworkReply::NoError) {
    result_[index] = kTestResultType_Success;
  } else {
    if (!reply->isFinished())
      result_[index] = kTestResultType_Timeout;
    else
      result_[index] = kTestResultType_Error;
  }

  if (++result_count_ == urls_.size()) {
    emit SignalKeyServerListTestResult(result_);
    emit SignalTaskRunnableEnd(0);
  }
}
