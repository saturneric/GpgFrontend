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

gpgme_error_t GpgFileOpera::encryptFile(GpgME::GpgContext *ctx, QVector<GpgKey> &keys, const QString &mPath,
                                        gpgme_encrypt_result_t *result) {

    QFileInfo fileInfo(mPath);

    if (!fileInfo.isFile() || !fileInfo.isReadable())
        throw std::runtime_error("no permission");

    QFile infile;
    infile.setFileName(mPath);
    if (!infile.open(QIODevice::ReadOnly))
        throw std::runtime_error("cannot open file");

    QByteArray inBuffer = infile.readAll();
    auto outBuffer = QByteArray();
    infile.close();

    auto error = ctx->encrypt(keys, inBuffer, &outBuffer, result);

    if (gpg_err_code(error) != GPG_ERR_NO_ERROR) return error;

    QFile outfile(mPath + ".asc");

    if (!outfile.open(QFile::WriteOnly))
        throw std::runtime_error("cannot open file");

    QDataStream out(&outfile);
    out.writeRawData(outBuffer.data(), outBuffer.length());
    outfile.close();
    return error;
}

gpgme_error_t GpgFileOpera::decryptFile(GpgME::GpgContext *ctx, const QString &mPath, gpgme_decrypt_result_t *result) {

    QFileInfo fileInfo(mPath);

    if (!fileInfo.isFile() || !fileInfo.isReadable())
        throw std::runtime_error("no permission");

    QFile infile;
    infile.setFileName(mPath);
    if (!infile.open(QIODevice::ReadOnly))
        throw std::runtime_error("cannot open file");

    QByteArray inBuffer = infile.readAll();
    auto outBuffer = QByteArray();
    infile.close();

    auto error = ctx->decrypt(inBuffer, &outBuffer, result);

    if (gpgme_err_code(error) != GPG_ERR_NO_ERROR) return error;

    QString outFileName, fileExtension = fileInfo.suffix();

    if (fileExtension == "asc" || fileExtension == "gpg") {
        int pos = mPath.lastIndexOf(QChar('.'));
        outFileName = mPath.left(pos);
    } else {
        outFileName = mPath + ".out";
    }

    QFile outfile(outFileName);

    if (!outfile.open(QFile::WriteOnly))
        throw std::runtime_error("cannot open file");

    QDataStream out(&outfile);
    out.writeRawData(outBuffer.data(), outBuffer.length());
    outfile.close();

    return error;
}

gpgme_error_t GpgFileOpera::signFile(GpgME::GpgContext *ctx, QVector<GpgKey> &keys, const QString &mPath,
                                     gpgme_sign_result_t *result) {

    QFileInfo fileInfo(mPath);

    if (!fileInfo.isFile() || !fileInfo.isReadable())
        throw std::runtime_error("no permission");

    QFile infile;
    infile.setFileName(mPath);
    if (!infile.open(QIODevice::ReadOnly))
        throw std::runtime_error("cannot open file");


    QByteArray inBuffer = infile.readAll();
    auto outBuffer = QByteArray();
    infile.close();

    auto error = ctx->sign(keys, inBuffer, &outBuffer, GPGME_SIG_MODE_DETACH, result);

    if (gpg_err_code(error) != GPG_ERR_NO_ERROR) return error;

    QFile outfile(mPath + ".sig");

    if (!outfile.open(QFile::WriteOnly))
        throw std::runtime_error("cannot open file");

    QDataStream out(&outfile);
    out.writeRawData(outBuffer.data(), outBuffer.length());
    outfile.close();

    return error;
}

gpgme_error_t GpgFileOpera::verifyFile(GpgME::GpgContext *ctx, const QString &mPath, gpgme_verify_result_t *result) {

    qDebug() << "Verify File Path" << mPath;

    QFileInfo fileInfo(mPath);

    if (!fileInfo.isFile() || !fileInfo.isReadable())
        throw std::runtime_error("no permission");

    QFile infile;
    infile.setFileName(mPath);
    if (!infile.open(QIODevice::ReadOnly))
        throw std::runtime_error("cannot open file");

    QByteArray inBuffer = infile.readAll();

    if(fileInfo.suffix() == "gpg") {
        auto error = ctx->verify(&inBuffer, nullptr, result);
        return error;
    }
    else {
        QFile signFile;
        signFile.setFileName(mPath + ".sig");
        if (!signFile.open(QIODevice::ReadOnly)) {
            throw std::runtime_error("cannot open file");
        }

        auto signBuffer = signFile.readAll();
        infile.close();

        auto error = ctx->verify(&inBuffer, &signBuffer, result);
        return error;
    }
}

gpg_error_t GpgFileOpera::encryptSignFile(GpgME::GpgContext *ctx, QVector<GpgKey> &keys, const QString &mPath,
                                          gpgme_encrypt_result_t *encr_res,
                                          gpgme_sign_result_t *sign_res) {

    qDebug() << "Encrypt Sign File Path" << mPath;

    QFileInfo fileInfo(mPath);

    if (!fileInfo.isFile() || !fileInfo.isReadable())
        throw std::runtime_error("no permission");

    QFile infile;
    infile.setFileName(mPath);
    if (!infile.open(QIODevice::ReadOnly))
        throw std::runtime_error("cannot open file");

    QByteArray inBuffer = infile.readAll();
    auto outBuffer = QByteArray();
    infile.close();

    QVector<GpgKey> signerKeys;

    // TODO dealing with signer keys
    auto error = ctx->encryptSign(keys, signerKeys, inBuffer, &outBuffer, encr_res, sign_res);

    if (gpg_err_code(error) != GPG_ERR_NO_ERROR)
        return error;

    QFile outfile(mPath + ".gpg");

    if (!outfile.open(QFile::WriteOnly))
        throw std::runtime_error("cannot open file");

    QDataStream out(&outfile);
    out.writeRawData(outBuffer.data(), outBuffer.length());
    outfile.close();

    return error;
}

gpg_error_t GpgFileOpera::decryptVerifyFile(GpgME::GpgContext *ctx, const QString &mPath, gpgme_decrypt_result_t *decr_res,
                                            gpgme_verify_result_t *verify_res) {

    qDebug() << "Decrypt Verify File Path" << mPath;

    QFileInfo fileInfo(mPath);

    if (!fileInfo.isFile() || !fileInfo.isReadable())
        throw std::runtime_error("no permission");

    QFile infile;
    infile.setFileName(mPath);
    if (!infile.open(QIODevice::ReadOnly))
        throw std::runtime_error("cannot open file");

    QByteArray inBuffer = infile.readAll();
    auto outBuffer = QByteArray();
    infile.close();

    auto error = ctx->decryptVerify(inBuffer, &outBuffer, decr_res, verify_res);
    if (gpg_err_code(error) != GPG_ERR_NO_ERROR) return error;

    QString outFileName, fileExtension = fileInfo.suffix();

    if (fileExtension == "asc" || fileExtension == "gpg") {
        int pos = mPath.lastIndexOf(QChar('.'));
        outFileName = mPath.left(pos);
    } else {
        outFileName = mPath + ".out";
    }

    QFile outfile(outFileName);

    if (!outfile.open(QFile::WriteOnly))
        throw std::runtime_error("cannot open file");

    QDataStream out(&outfile);
    out.writeRawData(outBuffer.data(), outBuffer.length());
    outfile.close();

    return error;
}
