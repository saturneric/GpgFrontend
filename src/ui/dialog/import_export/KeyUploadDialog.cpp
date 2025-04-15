/**
 * Copyright (C) 2021-2024 Saturneric <eric@bktus.com>
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

#include "KeyUploadDialog.h"

#include <QtNetwork>
#include <utility>

#include "core/function/gpg/GpgKeyImportExporter.h"
#include "core/model/SettingsObject.h"
#include "core/utils/BuildInfoUtils.h"
#include "core/utils/GpgUtils.h"
#include "ui/UserInterfaceUtils.h"
#include "ui/struct/settings_object/KeyServerSO.h"

namespace GpgFrontend::UI {

KeyUploadDialog::KeyUploadDialog(int channel, GpgAbstractKeyPtrList keys,
                                 QWidget* parent)
    : GeneralDialog(typeid(KeyUploadDialog).name(), parent),
      current_gpg_context_channel_(channel),
      keys_(std::move(keys)) {
  assert(std::all_of(keys_.begin(), keys_.end(),
                     [](const auto& key) { return key->IsGood(); }));

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
  this->setWindowTitle(tr("Uploading Public Key"));
  this->setFixedSize(240, 42);
  this->movePosition2CenterOfParent();
  this->setAttribute(Qt::WA_DeleteOnClose);
}

void KeyUploadDialog::SlotUpload() {
  GpgKeyImportExporter::GetInstance(current_gpg_context_channel_)
      .ExportKeys(keys_, false, true, false, false,
                  [=](GpgError err, const DataObjectPtr& data_obj) {
                    if (CheckGpgError(err) != GPG_ERR_NO_ERROR) {
                      CommonUtils::RaiseMessageBox(this, err);
                      return;
                    }

                    if (CheckGpgError(err) == GPG_ERR_USER_1 ||
                        data_obj == nullptr || !data_obj->Check<GFBuffer>()) {
                      FLOG_W("data object checking failed");
                      QMessageBox::critical(this, tr("Error"),
                                            tr("Unknown error occurred"));
                      // Done
                      this->hide();
                      this->close();
                      return;
                    }

                    auto gf_buffer = ExtractParams<GFBuffer>(data_obj, 0);
                    slot_upload_key_to_server(gf_buffer);

                    // Done
                    this->hide();
                    this->close();
                  });
}

void KeyUploadDialog::slot_upload_key_to_server(
    const GpgFrontend::GFBuffer& keys_data) {
  KeyServerSO key_server(SettingsObject("key_server"));
  auto target_key_server = key_server.GetTargetServer();

  LOG_D() << "upload public key using key server" << target_key_server;
  QUrl req_url(target_key_server + "/pks/add");
  auto* q_nam = new QNetworkAccessManager(this);

  // Building Post Data
  QByteArray post_data;

  auto data = keys_data.ConvertToQByteArray();

  data.replace("\n", "%0A");
  data.replace("\r", "%0D");
  data.replace("(", "%28");
  data.replace(")", "%29");
  data.replace("/", "%2F");
  data.replace(":", "%3A");
  data.replace("+", "%2B");
  data.replace("=", "%3D");
  data.replace(" ", "+");

  QNetworkRequest request(req_url);
  request.setHeader(QNetworkRequest::UserAgentHeader,
                    GetHttpRequestUserAgent());
  request.setHeader(QNetworkRequest::ContentTypeHeader,
                    "application/x-www-form-urlencoded");

  post_data.append("keytext").append("=").append(data);

  // Send Post Data
  QNetworkReply* reply = q_nam->post(request, post_data);
  connect(reply, &QNetworkReply::finished, this,
          &KeyUploadDialog::slot_upload_finished);

  // Keep Waiting
  while (reply->isRunning()) {
    QApplication::processEvents();
  }
}

void KeyUploadDialog::slot_upload_finished() {
  auto* reply = qobject_cast<QNetworkReply*>(sender());

  this->close();
  QByteArray response = reply->readAll();

  auto error = reply->error();
  if (error != QNetworkReply::NoError) {
    QString message;
    switch (error) {
      case QNetworkReply::ContentNotFoundError:
        message = tr("Key Not Found");
        break;
      case QNetworkReply::TimeoutError:
        message = tr("Timeout");
        break;
      case QNetworkReply::HostNotFoundError:
        message = tr("Key Server Not Found");
        break;
      default:
        message = tr("Connection Error");
    }
    QMessageBox::critical(this->parentWidget(), tr("Upload Failed"), message);
    return;
  }

  QMessageBox::information(this->parentWidget(), tr("Upload Success"),
                           tr("Upload Public Key Successfully"));
}

}  // namespace GpgFrontend::UI
