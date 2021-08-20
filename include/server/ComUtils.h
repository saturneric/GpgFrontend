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

#ifndef GPGFRONTEND_ZH_CN_TS_COMUTILS_H
#define GPGFRONTEND_ZH_CN_TS_COMUTILS_H

#include "GpgFrontend.h"
#include "gpg/GpgContext.h"
#include "rapidjson/document.h"

class ComUtils : public QWidget {
Q_OBJECT
public:

    enum ServiceType { GetServiceToken, ShortenCryptText, GetFullCryptText, UploadPubkey };

    explicit ComUtils(QWidget *parent) : QWidget(parent), appPath(qApp->applicationDirPath()),
                                         settings(RESOURCE_DIR(appPath) + "/conf/gpgfrontend.ini",
                                                  QSettings::IniFormat) {

    }

    QString getUrl(ServiceType type) const;

    bool checkServerReply(const QByteArray &reply);

    QString getDataValueStr(const QString &key);

    bool checkDataValueStr(const QString &key);

    rapidjson::Value &getDataValue(const QString &key);

    bool checkDataValue(const QString &key);

    bool checkServiceTokenFormat(const QString& serviceToken);

    static QByteArray getSignStringBase64(GpgME::GpgContext *ctx, const QString &str, const GpgKey& key);

    [[nodiscard]] bool good() const { return is_good; }

    QNetworkAccessManager &getNetworkManager() {return networkMgr;}

private:

    QString appPath;
    QSettings settings;
    rapidjson::Document replyDoc;
    rapidjson::Value dataVal;

    QNetworkAccessManager networkMgr;

    QRegularExpression re_uuid{R"(\b[0-9a-f]{8}\b-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-\b[0-9a-f]{12}\b)"};

    bool is_good = false;
};


#endif //GPGFRONTEND_ZH_CN_TS_COMUTILS_H
