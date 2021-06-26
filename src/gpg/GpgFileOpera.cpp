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
#include "gpg/GpgFileOpera.h"

bool GpgFileOpera::encryptFile(GpgME::GpgContext *ctx, QVector<GpgKey> &keys, const QString &mPath) {

    QFileInfo fileInfo(mPath);

    if(!fileInfo.isFile() || !fileInfo.isReadable()) return false;

    QFile infile;
    infile.setFileName(mPath);
    if (!infile.open(QIODevice::ReadOnly))
        return false;

    QByteArray inBuffer = infile.readAll();
    auto *outBuffer = new QByteArray();
    infile.close();

    if (gpg_err_code(ctx->encrypt(keys, inBuffer, outBuffer, nullptr)) != GPG_ERR_NO_ERROR) return false;

    QFile outfile(mPath + ".asc");

    if (!outfile.open(QFile::WriteOnly))
        return false;

    QDataStream out(&outfile);
    out.writeRawData(outBuffer->data(), outBuffer->length());
    outfile.close();
    return true;
}
