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

#include "MainWindow.h"
#include "server/ComUtils.h"
#include "ui/ShowCopyDialog.h"

#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"

/**
 * get full size crypt text from server using short crypto text
 * @param shortenCryptoText short crypto text([GpgFrontend_ShortCrypto]://)
 * @return
 */
QString MainWindow::getCryptText(const QString &shortenCryptoText) {
    auto host = settings.value("general/currentGpgfrontendServer",
                                  "service.gpgfrontend.pub").toString();

    QString ownKeyId = settings.value("general/ownKeyId").toString();

    GpgKey key = mCtx->getKeyById(ownKeyId);
    if (!key.good) {
        QMessageBox::critical(this, tr("Invalid Own Key"), tr("Own Key can not be use to do any operation."));
        return {};
    }

    QString serviceToken = settings.value("general/serviceToken").toString();
    if (serviceToken.isEmpty()) {
        QMessageBox::critical(this, tr("Error"),
                              tr("Please obtain a Service Token from the server in the settings."));
        return {};
    }

    QUrl reqUrl("http://127.0.0.1:9048/text/get");
    QNetworkRequest request(reqUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    // Sign Shorten Text
    QVector keys{key};
    QByteArray outSignText;
    mCtx->sign(keys, shortenCryptoText.toUtf8(), &outSignText, GPGME_SIG_MODE_NORMAL);
    auto outSignTextBase64 = outSignText.toBase64();

    rapidjson::Document doc;
    doc.SetObject();

    rapidjson::Value s, t;

    // Signature
    s.SetString(outSignTextBase64.constData(), outSignTextBase64.count());
    // Service Token
    const auto t_byte_array = serviceToken.toUtf8();
    t.SetString(t_byte_array.constData(), t_byte_array.count());

    doc.AddMember("signature", s, doc.GetAllocator());
    doc.AddMember("serviceToken", t, doc.GetAllocator());

    rapidjson::StringBuffer sb;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(sb);
    doc.Accept(writer);

    QByteArray postData(sb.GetString());
    qDebug() << "postData" << QString::fromUtf8(postData);

    QNetworkReply *reply = networkAccessManager->post(request, postData);

    auto dialog = new WaitingDialog("Getting Crypt Text From Server", this);
    dialog->show();

    while (reply->isRunning()) {
        QApplication::processEvents();
    }

    dialog->close();

    QByteArray replyData = reply->readAll().constData();
    auto comUtils = new ComUtils(this);
    if (comUtils->checkServerReply(replyData)) {
        //TODO Logic
    } else QMessageBox::critical(this, tr("Error"), tr("Unknown Error"));

    return {};
}

void MainWindow::shortenCryptText() {

    QString serviceToken = settings.value("general/serviceToken").toString();
    QString ownKeyId = settings.value("general/ownKeyId").toString();

    QByteArray cryptoText = edit->curTextPage()->toPlainText().toUtf8();

    QUrl reqUrl("http://127.0.0.1:9048/text/new");
    QNetworkRequest request(reqUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    GpgKey key = mCtx->getKeyById(ownKeyId);
    if (!key.good) {
        QMessageBox::critical(this, tr("Invalid Own Key"), tr("Own Key can not be use to do any operation."));
        return;
    }

    QCryptographicHash ch(QCryptographicHash::Md5);
    ch.addData(cryptoText);
    QString md5 = ch.result().toHex();

    qDebug() << "md5" << md5;

    QByteArray signText = QString("[%1][%2]").arg(serviceToken, md5).toUtf8();

    QCryptographicHash sha(QCryptographicHash::Sha256);
    sha.addData(signText);
    QString shaText = sha.result().toHex();

    qDebug() << "shaText" << shaText;

    QVector keys{key};
    QByteArray outSignText;
    mCtx->sign(keys, signText, &outSignText, GPGME_SIG_MODE_NORMAL);
    QByteArray outSignTextBase64 = outSignText.toBase64();

    rapidjson::Value c, s, m, t;

    rapidjson::Document doc;
    doc.SetObject();

    c.SetString(cryptoText.constData(), cryptoText.count());
    auto m_byte_array = shaText.toUtf8();
    m.SetString(m_byte_array.constData(), m_byte_array.count());
    s.SetString(outSignTextBase64.constData(), outSignTextBase64.count());
    auto t_byte_array = serviceToken.toUtf8();
    t.SetString(t_byte_array.constData(), t_byte_array.count());

    doc.AddMember("cryptoText", c, doc.GetAllocator());
    doc.AddMember("sha", m, doc.GetAllocator());
    doc.AddMember("sign", s, doc.GetAllocator());
    doc.AddMember("serviceToken", t, doc.GetAllocator());

    rapidjson::StringBuffer sb;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(sb);
    doc.Accept(writer);

    QByteArray postData(sb.GetString());
    qDebug() << "postData" << QString::fromUtf8(postData);

    QNetworkReply *reply = networkAccessManager->post(request, postData);

    while (reply->isRunning()) {
        QApplication::processEvents();
    }

    if (reply->error() == QNetworkReply::NoError) {
        rapidjson::Document docReply;
        docReply.Parse(reply->readAll().constData());
        QString shortenText = docReply["shortenText"].GetString();
        auto *dialog = new ShowCopyDialog(shortenText, this);
        dialog->show();
    } else {
        QMessageBox::critical(this, tr("Error"), reply->errorString());
    }


}

