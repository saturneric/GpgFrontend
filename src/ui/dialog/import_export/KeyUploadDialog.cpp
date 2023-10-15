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
 * All the source code of GpgFrontend was modified and released by
 * Saturneric<eric@bktus.com> starting on May 12, 2021.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include "KeyUploadDialog.h"

#include <QtNetwork>
#include <algorithm>

#include "core/function/GlobalSettingStation.h"
#include "core/function/gpg/GpgKeyGetter.h"
#include "core/function/gpg/GpgKeyImportExporter.h"
#include "ui/struct/SettingsObject.h"

namespace GpgFrontend::UI {

KeyUploadDialog::KeyUploadDialog(const KeyIdArgsListPtr& keys_ids,
                                 QWidget* parent)
    : GeneralDialog(typeid(KeyUploadDialog).name(), parent),
      m_keys_(GpgKeyGetter::GetInstance().GetKeys(keys_ids)) {
  auto* pb = new QProgressBar();
  pb->setRange(0, 0);
  pb->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
  pb->setTextVisible(false);

  auto* layout = new QVBoxLayout();
  layout->addWidget(pb);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);
  this->setLayout(layout);

  this->setModal(true);
  this->setWindowTitle(_("Uploading Public Key"));
  this->setFixedSize(240, 42);
  this->setPosCenterOfScreen();
}

void KeyUploadDialog::SlotUpload() {
  auto out_data = std::make_unique<ByteArray>();
  GpgKeyImportExporter::GetInstance().ExportKeys(*m_keys_, out_data);
  slot_upload_key_to_server(*out_data);

  // Done
  this->hide();
  this->close();
}

void KeyUploadDialog::slot_upload_key_to_server(
    const GpgFrontend::ByteArray& keys_data) {
  std::string target_keyserver;

  try {
    SettingsObject key_server_json("key_server");

    const auto key_server_list =
        key_server_json.Check("server_list", nlohmann::json::array());

    int default_key_server_index = key_server_json.Check("default_server", 0);
    if (default_key_server_index >= key_server_list.size()) {
      throw std::runtime_error("default_server index out of range");
    }

    target_keyserver =
        key_server_list[default_key_server_index].get<std::string>();

    SPDLOG_DEBUG("set target key server to default key server: {}",
                 target_keyserver);

  } catch (...) {
    SPDLOG_ERROR(_("Cannot read default_keyserver From Settings"));
    QMessageBox::critical(nullptr, _("Default Keyserver Not Found"),
                          _("Cannot read default keyserver from your settings, "
                            "please set a default keyserver first"));
    return;
  }

  QUrl req_url(QString::fromStdString(target_keyserver + "/pks/add"));
  auto qnam = new QNetworkAccessManager(this);

  // Building Post Data
  QByteArray postData;

  auto data = std::string(keys_data);

  boost::algorithm::replace_all(data, "\n", "%0A");
  boost::algorithm::replace_all(data, "\r", "%0D");
  boost::algorithm::replace_all(data, "(", "%28");
  boost::algorithm::replace_all(data, ")", "%29");
  boost::algorithm::replace_all(data, "/", "%2F");
  boost::algorithm::replace_all(data, ":", "%3A");
  boost::algorithm::replace_all(data, "+", "%2B");
  boost::algorithm::replace_all(data, "=", "%3D");
  boost::algorithm::replace_all(data, " ", "+");

  QNetworkRequest request(req_url);
  request.setHeader(QNetworkRequest::ContentTypeHeader,
                    "application/x-www-form-urlencoded");

  postData.append("keytext").append("=").append(
      QString::fromStdString(data).toUtf8());

  // Send Post Data
  QNetworkReply* reply = qnam->post(request, postData);
  connect(reply, &QNetworkReply::finished, this,
          &KeyUploadDialog::slot_upload_finished);

  // Keep Waiting
  while (reply->isRunning()) {
    QApplication::processEvents();
  }
}

void KeyUploadDialog::slot_upload_finished() {
  auto* reply = qobject_cast<QNetworkReply*>(sender());

  QByteArray response = reply->readAll();
  SPDLOG_DEBUG("response: {}", response.toStdString());

  auto error = reply->error();
  if (error != QNetworkReply::NoError) {
    SPDLOG_DEBUG("error from reply: {}", reply->errorString().toStdString());
    QString message;
    switch (error) {
      case QNetworkReply::ContentNotFoundError:
        message = _("Key Not Found");
        break;
      case QNetworkReply::TimeoutError:
        message = _("Timeout");
        break;
      case QNetworkReply::HostNotFoundError:
        message = _("Key Server Not Found");
        break;
      default:
        message = _("Connection Error");
    }
    QMessageBox::critical(this, "Upload Failed", message);
    return;
  } else {
    QMessageBox::information(this, _("Upload Success"),
                             _("Upload Public Key Successfully"));
    SPDLOG_DEBUG("success while contacting keyserver!");
  }
  reply->deleteLater();
}

}  // namespace GpgFrontend::UI
