/**
 * This file is part of GpgFrontend.
 *
 * GpgFrontend is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Foobar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Foobar.  If not, see <https://www.gnu.org/licenses/>.
 *
 * The initial version of the source code is inherited from gpg4usb-team.
 * Their source code version also complies with GNU General Public License.
 *
 * The source code version of this software was modified and released
 * by Saturneric<eric@bktus.com><eric@bktus.com> starting on May 12, 2021.
 *
 */

#include "ProxyConnectionTestThread.h"

void GpgFrontend::UI::ProxyConnectionTestThread::run() {
  QNetworkProxyQuery npq({QUrl(url_)});
  auto proxies_list = QNetworkProxyFactory::systemProxyForQuery(npq);

  if (proxies_list.isEmpty()) {
    LOG(INFO) << "no proxy applied";
  } else {
    LOG(INFO) << "proxies list hostname"
              << proxies_list.front().hostName().toStdString();
  }

  LOG(INFO) << "proxies list size" << proxies_list.size();

  auto manager = std::make_unique<QNetworkAccessManager>(nullptr);
  QNetworkRequest url_request;
  url_request.setUrl(QUrl(url_));
  auto _reply = manager->get(url_request);

  while (_reply->isRunning()) QApplication::processEvents();
  auto _buffer = _reply->readAll();
  if (_reply->error() == QNetworkReply::NoError && !_buffer.isEmpty()) {
    result_ = "Reachable";
  } else {
    result_ = "Not Reachable";
  }

  _reply->deleteLater();

  emit signalProxyConnectionTestResult(result_);
}
