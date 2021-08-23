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

#include "server/api/PubkeyGetter.h"

PubkeyGetter::PubkeyGetter(GpgME::GpgContext *ctx, const QVector<QString> &fprs) : BaseAPI(ComUtils::GetPubkey),
                                                                                   mCtx(ctx), mFprs(fprs) {
}

void PubkeyGetter::construct_json() {
    document.SetArray();
    QStringList keyIds;

    rapidjson::Document::AllocatorType &allocator = document.GetAllocator();

    for (const auto &fprStr : mFprs) {
        rapidjson::Value fpr;

        auto fprByteArray = fprStr.toUtf8();
        fpr.SetString(fprByteArray.constData(), fprByteArray.count());

        document.PushBack(fpr, allocator);
        keyIds.clear();
    }
}

void PubkeyGetter::deal_reply() {

    const auto &utils = getUtils();

    /**
     * {
     *      "pubkeys" : [
     *          {
     *              "publicKey" : ...,
     *              "fpr" : ...,
     *              "sha" : ...
     *          },
     *          ...
     *      ]
     * }
     */

    if (!utils.checkDataValue("pubkeys")) {
        QMessageBox::critical(nullptr, tr("Error"),
                              tr("The communication content with the server does not meet the requirements"));
    } else {
        auto &pubkeys = utils.getDataValue("pubkeys");
        qDebug() << "Pubkey Getter" << pubkeys.IsArray() << pubkeys.GetArray().Size();
        if (pubkeys.IsArray()) {
            for (const auto &pubkey : pubkeys.GetArray()) {
                if (pubkey.IsObject()
                    && pubkey.HasMember("publicKey") && pubkey.HasMember("fpr") && pubkey.HasMember("sha")
                    && pubkey["publicKey"].IsString() && pubkey["fpr"].IsString() && pubkey["sha"].IsString()) {

                    auto pubkeyData = QString(pubkey["publicKey"].GetString());

                    QCryptographicHash shaGen(QCryptographicHash::Sha256);
                    shaGen.addData(pubkeyData.toUtf8());

                    if (shaGen.result().toHex() == pubkey["sha"].GetString()) {
                        mCtx->importKey(pubkeyData.toUtf8());
                    }

                }
            }

        } else {
            QMessageBox::critical(nullptr, tr("Error"),
                                  tr("The communication content with the server does not meet the requirements"));
        }
    }
}


