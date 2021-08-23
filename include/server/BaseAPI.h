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

#ifndef GPGFRONTEND_ZH_CN_TS_BASEAPI_H
#define GPGFRONTEND_ZH_CN_TS_BASEAPI_H

#include "GpgFrontend.h"
#include "ComUtils.h"

#include "rapidjson/document.h"

class BaseAPI : QObject {
Q_OBJECT
public:

    explicit BaseAPI(ComUtils::ServiceType serviceType);

    virtual ~BaseAPI();

    void start();

    void refresh();

    [[nodiscard]] bool result() const;

private:

    ComUtils *utils;

    QUrl reqUrl;

    QNetworkRequest request;

    bool good = false;

    QNetworkReply *send_json_data();

protected:

    rapidjson::Document document;

    const ComUtils &getUtils() { return *utils; };

    virtual void construct_json() = 0;

    virtual void deal_reply() = 0;


};


#endif //GPGFRONTEND_ZH_CN_TS_BASEAPI_H
