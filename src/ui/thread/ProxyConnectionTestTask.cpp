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

#include "ProxyConnectionTestTask.h"

#include <QtNetwork>

#include "core/utils/BuildInfoUtils.h"

GpgFrontend::UI::ProxyConnectionTestTask::ProxyConnectionTestTask(QString url,
                                                                  int timeout)
    : Task("proxy_connection_test_task"),
      url_(std::move(url)),
      timeout_(timeout),
      network_manager_(new QNetworkAccessManager(this)) {
  HoldOnLifeCycle(true);
}

auto GpgFrontend::UI::ProxyConnectionTestTask::Run() -> int {
  auto request = QNetworkRequest(url_);
  request.setHeader(QNetworkRequest::UserAgentHeader,
                    GetHttpRequestUserAgent());

  auto* network_reply = network_manager_->get(request);
  auto* timer = new QTimer(this);

  connect(network_reply, &QNetworkReply::finished, this,
          [this, network_reply]() {
            GF_UI_LOG_DEBUG("key server domain reply: {} received", url_);
            this->slot_process_network_reply(network_reply);
          });

  connect(timer, &QTimer::timeout, this, [this, network_reply]() {
    GF_UI_LOG_DEBUG("timeout for key server: {}", url_);
    if (network_reply->isRunning()) {
      network_reply->abort();
      this->slot_process_network_reply(network_reply);
    }
  });

  timer->start(timeout_);
  return 0;
}

void GpgFrontend::UI::ProxyConnectionTestTask::slot_process_network_reply(
    QNetworkReply* reply) {
  auto buffer = reply->readAll();
  GF_UI_LOG_DEBUG("key server domain reply: {}, buffer size: {}", url_,
                  buffer.size());

  if (reply->error() == QNetworkReply::NoError && !buffer.isEmpty()) {
    result_ = "Reachable";
  } else {
    result_ = "Not Reachable";
  }

  emit SignalProxyConnectionTestResult(result_);
  emit SignalTaskShouldEnd(0);
}
