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

#include "ListedKeyServerTestTask.h"

#include <QtNetwork>

#include "core/utils/BuildInfoUtils.h"

GpgFrontend::UI::ListedKeyServerTestTask::ListedKeyServerTestTask(
    QStringList urls, int timeout, QWidget* /*parent*/)
    : Task("listed_key_server_test_task"),
      urls_(std::move(urls)),
      result_(urls_.size(), kTEST_RESULT_TYPE_ERROR),
      network_manager_(new QNetworkAccessManager(this)),
      timeout_(timeout) {
  HoldOnLifeCycle(true);
  qRegisterMetaType<std::vector<KeyServerTestResultType>>(
      "std::vector<KeyServerTestResultType>");
}

auto GpgFrontend::UI::ListedKeyServerTestTask::Run() -> int {
  size_t index = 0;
  for (const auto& url : urls_) {
    auto key_url = QUrl{url};
    GF_UI_LOG_DEBUG("key server request: {}", key_url.host());

    auto request = QNetworkRequest(key_url);
    request.setHeader(QNetworkRequest::UserAgentHeader,
                      GetHttpRequestUserAgent());

    auto* network_reply = network_manager_->get(request);
    auto* timer = new QTimer(this);

    connect(network_reply, &QNetworkReply::finished, this,
            [this, index, network_reply]() {
              GF_UI_LOG_DEBUG("key server domain reply: {}", urls_[index]);
              this->slot_process_network_reply(index, network_reply);
            });

    connect(timer, &QTimer::timeout, this, [this, index, network_reply]() {
      GF_UI_LOG_DEBUG("timeout for key server: {}", urls_[index]);
      if (network_reply->isRunning()) {
        network_reply->abort();
        this->slot_process_network_reply(index, network_reply);
      }
    });

    timer->start(timeout_);
    index++;
  }

  return 0;
}

void GpgFrontend::UI::ListedKeyServerTestTask::slot_process_network_reply(
    int index, QNetworkReply* reply) {
  if (!reply->isRunning() && reply->error() == QNetworkReply::NoError) {
    result_[index] = kTEST_RESULT_TYPE_SUCCESS;
  } else {
    if (!reply->isFinished()) {
      result_[index] = kTEST_RESULT_TYPE_TIMEOUT;
    } else {
      result_[index] = kTEST_RESULT_TYPE_ERROR;
    }
  }

  if (++result_count_ == urls_.size()) {
    emit SignalKeyServerListTestResult(result_);
    emit SignalTaskShouldEnd(0);
  }
}
