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

#include "server/ComUtils.h"

/**
 * check server reply if it can parse into a json object
 * @param reply reply data in byte array
 * @return if successful
 */
bool ComUtils::checkServerReply(const QByteArray &reply) {

    if(reply.isEmpty()) {
        QMessageBox::critical(this, tr("Error"), tr("Nothing Reply. Please check the Internet connection."));
        return false;
    }

    qDebug() << "Reply" << reply;

    /**
     * Server Reply Format(Except Timeout)
     * {
     *      "status": 200,
     *      "msg": "OK",
     *      "timestamp": 1628652783895
     *      "data" : {
     *          ...
     *      }
     * }
     */

    // check if reply is a json object
    if (replyDoc.Parse(reply).HasParseError() || !replyDoc.IsObject()) {
        QMessageBox::critical(this, tr("Error"), tr("Unknown Error"));
        return false;
    }

    // check status(int) and message(string)
    if (replyDoc.HasMember("status") && replyDoc.HasMember("msg") && replyDoc.HasMember("timestamp") &&
        replyDoc.HasMember("data")
        && replyDoc["status"].IsNumber() && replyDoc["msg"].IsString() && replyDoc["timestamp"].IsNumber() &&
        replyDoc["data"].IsObject()) {

        int status = replyDoc["status"].GetInt();
        QDateTime time;
        time.setMSecsSinceEpoch(replyDoc["timestamp"].GetInt64());
        auto message = replyDoc["msg"].GetString();
        dataVal = replyDoc["data"].GetObject();

        qDebug() << "Reply Date & Time" << time;

        // check reply timestamp
        if (time < QDateTime::currentDateTime().addSecs(-10)) {
            QMessageBox::critical(this, tr("Network Error"), tr("Outdated Reply"));
            return false;
        }

        // check status code if successful (200-299)
        // check data object
        if (status / 100 == 2) {
            is_good = true;
            return true;
        } else  {
            if (dataVal.HasMember("exceptionMessage") && dataVal["exceptionMessage"].IsString())
                QMessageBox::critical(this, message, dataVal["exceptionMessage"].GetString());
            else QMessageBox::critical(this, message, tr("Unknown Reason"));
        }

    } else QMessageBox::critical(this, tr("Network Error"), tr("Unknown Reply Format"));

    return false;
}

/**
 * get value in data
 * @param key key of value
 * @return value in string format
 */
QString ComUtils::getDataValueStr(const QString &key) const {
    if (is_good) {
        auto k_byte_array = key.toUtf8();
        if (dataVal.HasMember(k_byte_array.data())) {
            return dataVal[k_byte_array.data()].GetString();
        } else return {};
    } else return {};
}

/**
 * Get eventually url by service type
 * @param type service which server provides
 * @return url
 */
QString ComUtils::getUrl(ComUtils::ServiceType type) const {
    auto host = settings.value("general/currentGpgfrontendServer",
                               "service.gpgfrontend.pub").toString();

    auto protocol = QString();
    // Localhost Debug Server
    if (host == "localhost") protocol = "http://";
    else protocol = "https://";

    auto url = protocol + host + ":9049/";

    switch (type) {
        case GetServiceToken:
            url += "/user";
            break;
        case ShortenCryptText:
            url += "/text/new";
            break;
        case GetFullCryptText:
            url += "/text/get";
            break;
        case UploadPubkey:
            url += "/key/upload";
            break;
        case GetPubkey:
            url += "/key/get";
            break;
    }

    qDebug() << "ComUtils getUrl" << url;

    return url;
}

bool ComUtils::checkDataValueStr(const QString &key) const {
    auto key_byte_array_data = key.toUtf8().constData();
    if (is_good) {
        return dataVal.HasMember(key_byte_array_data) && dataVal[key_byte_array_data].IsString();
    } else return false;
}

bool ComUtils::checkServiceTokenFormat(const QString &uuid) const {
    return re_uuid.match(uuid).hasMatch();
}

QByteArray ComUtils::getSignStringBase64(GpgME::GpgContext *ctx, const QString &str, const GpgKey &key) {
    QVector<GpgKey> keys{key};
    QByteArray outSignText;
    auto signData = str.toUtf8();

    // The use of multi-threading brings an improvement in UI smoothness
    gpgme_error_t error;
    auto thread = QThread::create([&]() {
        error = ctx->sign(keys, signData, &outSignText, GPGME_SIG_MODE_NORMAL, nullptr, false);
    });
    thread->start();
    while (thread->isRunning()) QApplication::processEvents();
    thread->deleteLater();

    return outSignText.toBase64();
}

const rapidjson::Value &ComUtils::getDataValue(const QString &key) const {
    if (is_good) {
        auto k_byte_array = key.toUtf8();
        if (dataVal.HasMember(k_byte_array.data())) {
            return dataVal[k_byte_array.data()];
        }
    }
    throw std::runtime_error("Inner Error");
}

bool ComUtils::checkDataValue(const QString &key) const{
    auto key_byte_array_data = key.toUtf8().constData();
    if (is_good) {
        return dataVal.HasMember(key_byte_array_data);
    } else return false;
}

void ComUtils::clear() {
    this->dataVal.Clear();
    this->replyDoc.Clear();
    is_good = false;
}