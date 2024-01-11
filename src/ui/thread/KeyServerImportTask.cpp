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

#include "ui/thread/KeyServerImportTask.h"

#include "core/function/gpg/GpgKeyImportExporter.h"
#include "ui/struct/SettingsObject.h"

GpgFrontend::UI::KeyServerImportTask::KeyServerImportTask(
    std::string keyserver_url, std::vector<std::string> keyids)
    : Task("key_server_import_task"),
      keyserver_url_(std::move(keyserver_url)),
      keyids_(std::move(keyids)),
      manager_(new QNetworkAccessManager(this)) {
  HoldOnLifeCycle(true);

  if (keyserver_url_.empty()) {
    try {
      SettingsObject key_server_json("key_server");
      const auto key_server_list =
          key_server_json.Check("server_list", nlohmann::json::array());

      size_t const default_key_server_index =
          key_server_json.Check("default_server", 0);
      if (default_key_server_index >= key_server_list.size()) {
        throw std::runtime_error("default_server index out of range");
      }
      auto default_key_server =
          key_server_list[default_key_server_index].get<std::string>();

      keyserver_url_ = default_key_server;
    } catch (...) {
      GF_UI_LOG_ERROR("setting operation error", "server_list",
                      "default_server");
      keyserver_url_ = "https://keys.openpgp.org";
      return;
    }

    GF_UI_LOG_DEBUG("key server import task sets key server url: {}",
                    keyserver_url_);
  }
}

void GpgFrontend::UI::KeyServerImportTask::Run() {
  QUrl const keyserver_url = QUrl(keyserver_url_.c_str());
  for (const auto& key_id : keyids_) {
    QUrl const req_url(keyserver_url.scheme() + "://" + keyserver_url.host() +
                       "/pks/lookup?op=get&search=0x" + key_id.c_str() +
                       "&options=mr");

    reply_ = manager_->get(QNetworkRequest(req_url));
    connect(reply_, &QNetworkReply::finished, this,
            &KeyServerImportTask::dealing_reply_from_server);
  }
}

void GpgFrontend::UI::KeyServerImportTask::dealing_reply_from_server() {
  auto const network_reply = reply_->error();
  auto buffer = reply_->readAll();

  if (network_reply != QNetworkReply::NoError) {
    GF_UI_LOG_ERROR("key import error, message from key server reply: ",
                    buffer);
    QString err_msg;
    switch (network_reply) {
      case QNetworkReply::ContentNotFoundError:
        err_msg = QString(_("Key not found in the Keyserver."));
        break;
      case QNetworkReply::TimeoutError:
        err_msg = QString(_("Network connection timeout."));
        break;
      case QNetworkReply::HostNotFoundError:
        err_msg =
            QString(_("Cannot resolve the address of target key server."));
        break;
      default:
        err_msg = QString(_("General connection error occurred."));
    }
    emit SignalKeyServerImportResult(false, err_msg, buffer, nullptr);
  }

  auto info = GpgKeyImportExporter::GetInstance().ImportKey(GFBuffer(buffer));
  emit SignalKeyServerImportResult(true, _("Success"), buffer, info);

  if (static_cast<size_t>(result_count_++) == keyids_.size() - 1) {
    emit SignalTaskShouldEnd(0);
  }
}