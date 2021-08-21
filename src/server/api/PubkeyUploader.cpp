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

#include "server/api/PubkeyUploader.h"

PubkeyUploader::PubkeyUploader(GpgME::GpgContext *ctx, const QVector<GpgKey> &keys) : BaseAPI(ComUtils::UploadPubkey),
                                                                                      mCtx(ctx),
                                                                                      mKeys(keys) {
}

void PubkeyUploader::construct_json() {
    document.SetArray();
    QStringList keyIds;

    rapidjson::Document::AllocatorType &allocator = document.GetAllocator();

    for (const auto &key : mKeys) {
        rapidjson::Value publicKeyObj, pubkey, sha, signedFpr;

        QByteArray keyDataBuf;
        keyIds << key.id;
        mCtx->exportKeys(&keyIds, &keyDataBuf);

        QCryptographicHash shaGen(QCryptographicHash::Sha256);
        shaGen.addData(keyDataBuf);

        auto shaStr = shaGen.result().toHex();

        auto signFprStr = ComUtils::getSignStringBase64(mCtx, key.fpr, key);
        qDebug() << "signFprStr" << signFprStr;

        pubkey.SetString(keyDataBuf.constData(), keyDataBuf.count());

        sha.SetString(shaStr.constData(), shaStr.count());
        signedFpr.SetString(signFprStr.constData(), signFprStr.count());

        publicKeyObj.SetObject();

        publicKeyObj.AddMember("publicKey", pubkey, allocator);
        publicKeyObj.AddMember("sha", sha, allocator);
        publicKeyObj.AddMember("signedFpr", signedFpr, allocator);

        document.PushBack(publicKeyObj, allocator);
        keyIds.clear();
    }
}

void PubkeyUploader::deal_reply() {

    const auto &utils = getUtils();

    /**
     * {
     *      "strings" : [
     *          "...",
     *          "..."
     *      ]
     * }
     */

    if (!utils.checkDataValue("strings")) {
        QMessageBox::critical(nullptr, tr("Error"),
                              tr("The communication content with the server does not meet the requirements"));
    } else {
        auto &strings = utils.getDataValue("strings");
        qDebug() << "Pubkey Uploader" << strings.IsArray() << strings.GetArray().Size();
        if (strings.IsArray() && strings.GetArray().Size() == mKeys.size()) {

        } else {
            QMessageBox::warning(nullptr, tr("Warning"),
                                 tr("Partial failure of automatic pubkey exchange"));
        }
    }
}
