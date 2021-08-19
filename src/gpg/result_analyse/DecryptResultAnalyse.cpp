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

#include "gpg/result_analyse/DecryptResultAnalyse.h"

DecryptResultAnalyse::DecryptResultAnalyse(GpgME::GpgContext *ctx, gpgme_error_t error, gpgme_decrypt_result_t result)
        : mCtx(ctx) {

    stream << tr("[#] Decrypt Operation ");

    if (gpgme_err_code(error) == GPG_ERR_NO_ERROR) {
        stream << tr("[Success]") << Qt::endl;
    } else {
        stream << tr("[Failed] ") << gpgme_strerror(error) << Qt::endl;
        setStatus(-1);
        if (result != nullptr && result->unsupported_algorithm != nullptr) {
            stream << "------------>" << Qt::endl;
            stream << tr("Unsupported Algo: ") << result->unsupported_algorithm << Qt::endl;
        }
    }

    if (result != nullptr && result->recipients != nullptr) {
        stream << "------------>" << Qt::endl;
        if (result->file_name != nullptr) {
            stream << tr("File Name: ") << result->file_name << Qt::endl;
            stream << Qt::endl;
        }

        auto reci = result->recipients;
        if (reci != nullptr)
            stream << tr("Recipient(s): ") << Qt::endl;
        while (reci != nullptr) {
            printReci(stream, reci);
            reci = reci->next;
        }
        stream << "<------------" << Qt::endl;
    }

    stream << Qt::endl;

}

bool DecryptResultAnalyse::printReci(QTextStream &stream, gpgme_recipient_t reci) {
    bool keyFound = true;
    stream << QApplication::tr("  {>} Recipient: ");

    try {
        auto key = mCtx->getKeyById(reci->keyid);
        stream << key.name;
        if (!key.email.isEmpty()) {
            stream << "<" << key.email << ">";
        }
    } catch (std::runtime_error &ignored) {
        stream << "<Unknown>";
        setStatus(0);
        keyFound = false;
    }
    stream << Qt::endl;

    stream << tr("      Keu ID: ") << reci->keyid << Qt::endl;
    stream << tr("      Public Algo: ") << gpgme_pubkey_algo_name(reci->pubkey_algo) << Qt::endl;

    return keyFound;
}
