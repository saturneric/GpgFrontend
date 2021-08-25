/**
 * This file is part of GPGFrontend.
 *
 * GPGFrontend is free software: you can redistribute it and/or modify
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

#include "ui/KeyUploadDialog.h"

KeyUploadDialog::KeyUploadDialog(GpgME::GpgContext *ctx, const QVector<GpgKey> &keys, QWidget *parent)
    : appPath(qApp->applicationDirPath()),
    settings(RESOURCE_DIR(appPath) + "/conf/gpgfrontend.ini", QSettings::IniFormat),
    mCtx(ctx),
    mKeys(keys),
    QDialog(parent) {


    auto *pb = new QProgressBar();
    pb->setRange(0, 0);
    pb->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    pb->setTextVisible(false);

    auto *layout = new QVBoxLayout();
    layout->addWidget(pb);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    this->setLayout(layout);

    this->setModal(true);
    this->setWindowTitle(tr("Uploading Public Key"));
    this->setFixedSize(240, 42);
}

void KeyUploadDialog::slotUpload() {
    mCtx->exportKeys(mKeys, mKeyData);
    uploadKeyToServer(mKeyData);
}

void KeyUploadDialog::uploadKeyToServer(QByteArray &keys) {

    // set default keyserver
    QString keyserver = settings.value("keyserver/defaultKeyServer").toString();

    QUrl reqUrl(keyserver + "/pks/add");
    auto qnam = new QNetworkAccessManager(this);

    // Building Post Data
    QByteArray postData;

    keys.replace("\n", "%0A")
            .replace("\r", "%0D")
            .replace("(", "%28")
            .replace(")", "%29")
            .replace("/", "%2F")
            .replace(":", "%3A")
            .replace("+", "%2B")
            .replace('=', "%3D")
            .replace(' ', '+');

    QNetworkRequest request(reqUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    postData.append("keytext").append("=").append(keys);

    // Send Post Data
    QNetworkReply *reply = qnam->post(request, postData);
    connect(reply, SIGNAL(finished()),
            this, SLOT(slotUploadFinished()));


    // Keep Waiting
    while(reply->isRunning()) {
        QApplication::processEvents();
    }

    // Done
    this->hide();
    this->close();
}

void KeyUploadDialog::slotUploadFinished() {
    auto *reply = qobject_cast<QNetworkReply *>(sender());

    QByteArray response = reply->readAll();
    qDebug() << "Response: " << response.data();

    auto error = reply->error();
    if (error != QNetworkReply::NoError) {
        qDebug() << "Error From Reply" << reply->errorString();
        QString message;
        switch (error) {
            case QNetworkReply::ContentNotFoundError :
                message = tr("Key Not Found");
                break;
            case QNetworkReply::TimeoutError :
                message = tr("Timeout");
                break;
            case QNetworkReply::HostNotFoundError :
                message = tr("Key Server Not Found");
                break;
            default:
                message = tr("Connection Error");
        }
        QMessageBox::critical(this, "Upload Failed", message);
        return;
    } else {
        QMessageBox::information(this, "Upload Success", "Upload Public Key Successfully");
        qDebug() << "Success while contacting keyserver!";
    }
    reply->deleteLater();
}
