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
 * by Saturneric<eric@bktus.com> starting on May 12, 2021.
 *
 */

#include "server/BaseAPI.h"

#include "rapidjson/writer.h"

BaseAPI::BaseAPI(ComUtils::ServiceType serviceType)
    : utils(new ComUtils(nullptr)),
      reqUrl(utils->getUrl(serviceType)),
      request(reqUrl) {
  request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
}

BaseAPI::~BaseAPI() { utils->deleteLater(); }

QNetworkReply *BaseAPI::send_json_data() {
  rapidjson::StringBuffer sb;
  rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
  document.Accept(writer);

  QByteArray postData(sb.GetString());
  qDebug() << "postData" << QString::fromUtf8(postData);

  auto reply = utils->getNetworkManager().post(request, postData);

  while (reply->isRunning()) QApplication::processEvents();

  QByteArray replyData = reply->readAll().constData();
  if (utils->checkServerReply(replyData)) {
    good = true, deal_reply();
  }

  return reply;
}

void BaseAPI::start() {
  construct_json();
  send_json_data()->deleteLater();
}

void BaseAPI::refresh() {
  document.Clear();
  utils->clear();
  good = false;
}

bool BaseAPI::result() const { return good; }
