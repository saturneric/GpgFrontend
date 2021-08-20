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

#include "server/PubkeyUploader.h"

#include "rapidjson/prettywriter.h"

PubkeyUploader::PubkeyUploader(GpgME::GpgContext *ctx, const QVector<GpgKey> &keys) {
    auto utils = new ComUtils(nullptr);
    QUrl reqUrl(utils->getUrl(ComUtils::UploadPubkey));
    QNetworkRequest request(reqUrl);

    rapidjson::Document publicKeys;
    publicKeys.SetArray();
    QStringList keyIds;

    rapidjson::Document::AllocatorType& allocator = publicKeys.GetAllocator();

    for(const auto &key : keys) {
        rapidjson::Value publicKeyObj, pubkey, sha, signedFpr;

        QByteArray keyDataBuf;
        keyIds << key.id;
        ctx->exportKeys(&keyIds, &keyDataBuf);

        QCryptographicHash shaGen(QCryptographicHash::Sha256);
        shaGen.addData(keyDataBuf);

        auto shaStr = shaGen.result().toHex();

        auto signFprStr = ComUtils::getSignStringBase64(ctx, key.fpr, key);

        pubkey.SetString(keyDataBuf.constData(), keyDataBuf.count());

        sha.SetString(shaStr.constData(), shaStr.count());
        signedFpr.SetString(signFprStr.constData(), signFprStr.count());

        publicKeyObj.SetObject();

        publicKeyObj.AddMember("publicKey", pubkey, allocator);
        publicKeyObj.AddMember("sha", sha, allocator);
        publicKeyObj.AddMember("signedFpr", signedFpr, allocator);

        publicKeys.PushBack(publicKeyObj, allocator);
        keyIds.clear();
    }

    rapidjson::StringBuffer sb;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(sb);
    publicKeys.Accept(writer);

    QByteArray postData(sb.GetString());
    qDebug() << "postData" << QString::fromUtf8(postData);

    QNetworkReply *reply = utils->getNetworkManager().post(request, postData);

    while (reply->isRunning()) QApplication::processEvents();

    QByteArray replyData = reply->readAll().constData();
    if (utils->checkServerReply(replyData)) {
        /**
         * {
         *      "strings" : [
         *          "...",
         *          "..."
         *      ]
         * }
         */
    }

}
