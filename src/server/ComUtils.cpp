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

    /**
     * Server Reply Format(Except Timeout)
     * {
     *      "status": 200,
     *      "message": "OK",
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
    if (replyDoc.HasMember("status") && replyDoc.HasMember("message")
        && replyDoc["status"].IsInt() && replyDoc["message"].IsString()) {

        int status = replyDoc["status"].GetInt();
        QString message = replyDoc["status"].GetString();

        // check status code if successful (200-299)
        // check data object
        if (status / 10 == 2 && replyDoc.HasMember("data") && replyDoc["data"].IsObject()) {
            dataVal = replyDoc["data"].GetObjectA();
            is_good = true;
            return true;
        } else QMessageBox::critical(this, tr("Error"), message);

    } else QMessageBox::critical(this, tr("Error"), tr("Unknown Reply Format"));
}

/**
 * get value in data
 * @param key key of value
 * @return value in string format
 */
QString ComUtils::getDataValue(const QString &key) {
    if (is_good) {
        auto k_byte_array = key.toUtf8();
        if (dataVal.HasMember(k_byte_array.data())) {
            return dataVal[k_byte_array.data()].GetString();
        } else return {};
    } else return {};
}
