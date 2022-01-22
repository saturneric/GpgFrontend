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

#include "ui/KeyUploadDialog.h"

#include <algorithm>

#include "gpg/function/GpgKeyGetter.h"
#include "gpg/function/GpgKeyImportExporter.h"
#include "ui/settings/GlobalSettingStation.h"

namespace GpgFrontend::UI {

KeyUploadDialog::KeyUploadDialog(const KeyIdArgsListPtr& keys_ids,
                                 QWidget* parent)
    : QDialog(parent), mKeys(GpgKeyGetter::GetInstance().GetKeys(keys_ids)) {
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
}

void KeyUploadDialog::slotUpload() {
  auto out_data = std::make_unique<ByteArray>();
  GpgKeyImportExporter::GetInstance().ExportKeys(*mKeys, out_data);
  uploadKeyToServer(*out_data);
}

void KeyUploadDialog::uploadKeyToServer(
    const GpgFrontend::ByteArray& keys_data) {
  std::string target_keyserver;
  if (target_keyserver.empty()) {
    try {
      auto& settings = GlobalSettingStation::GetInstance().GetUISettings();

      target_keyserver = settings.lookup("keyserver.default_server").c_str();

      LOG(INFO) << _("Set target Key Server to default Key Server")
                << target_keyserver;
    } catch (...) {
      LOG(ERROR) << _("Cannot read default_keyserver From Settings");
      QMessageBox::critical(
          nullptr, _("Default Keyserver Not Found"),
          _("Cannot read default keyserver from your settings, "
            "please set a default keyserver first"));
      return;
    }
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
  connect(reply, SIGNAL(finished()), this, SLOT(slotUploadFinished()));

  // Keep Waiting
  while (reply->isRunning()) {
    QApplication::processEvents();
  }

  // Done
  this->hide();
  this->close();
}

void KeyUploadDialog::slotUploadFinished() {
  auto* reply = qobject_cast<QNetworkReply*>(sender());

  QByteArray response = reply->readAll();
  LOG(INFO) << "Response: " << response.toStdString();

  auto error = reply->error();
  if (error != QNetworkReply::NoError) {
    LOG(INFO) << "Error From Reply" << reply->errorString().toStdString();
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
    LOG(INFO) << "Success while contacting keyserver!";
  }
  reply->deleteLater();
}

}  // namespace GpgFrontend::UI
