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

#include "ProxyConnectionTestTask.h"

#include <QtNetwork>

GpgFrontend::UI::ProxyConnectionTestTask::ProxyConnectionTestTask(QString url,
                                                                  int timeout)
    : Task("proxy_connection_test_task"),
      url_(std::move(url)),
      timeout_(timeout),
      network_manager_(new QNetworkAccessManager(this)) {}

void GpgFrontend::UI::ProxyConnectionTestTask::run() {
  HoldOnLifeCycle(true);

  auto* network_reply = network_manager_->get(QNetworkRequest{url_});
  auto* timer = new QTimer(this);

  connect(network_reply, &QNetworkReply::finished, this,
          [this, network_reply]() {
            SPDLOG_DEBUG("key server domain reply: {} received",
                         url_.toStdString());
            this->slot_process_network_reply(network_reply);
          });

  connect(timer, &QTimer::timeout, this, [this, network_reply]() {
    SPDLOG_DEBUG("timeout for key server: {}", url_.toStdString());
    if (network_reply->isRunning()) {
      network_reply->abort();
      this->slot_process_network_reply(network_reply);
    }
  });

  timer->start(timeout_);
}

void GpgFrontend::UI::ProxyConnectionTestTask::slot_process_network_reply(
    QNetworkReply* reply) {
  auto buffer = reply->readAll();
  SPDLOG_DEBUG("key server domain reply: {}, buffer size: {}",
               url_.toStdString(), buffer.size());

  if (reply->error() == QNetworkReply::NoError && !buffer.isEmpty()) {
    result_ = "Reachable";
  } else {
    result_ = "Not Reachable";
  }

  emit SignalProxyConnectionTestResult(result_);
  emit SignalTaskRunnableEnd(0);
}
