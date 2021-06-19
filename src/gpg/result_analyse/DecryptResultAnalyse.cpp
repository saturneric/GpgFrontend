//
// Created by eric on 2021/6/9.
//

#include "gpg/result_analyse/DecryptResultAnalyse.h"

DecryptResultAnalyse::DecryptResultAnalyse(GpgME::GpgContext *ctx, gpgme_error_t error, gpgme_decrypt_result_t result)
        : mCtx(ctx) {

    stream << "Decrypt Report: " << Qt::endl << "-----" << Qt::endl;

    if (gpgme_err_code(error) == GPG_ERR_NO_ERROR) {
        stream << "Status: Success" << Qt::endl;
    } else {
        setStatus(-1);
        stream << "Status: " << gpgme_strerror(error) << Qt::endl;

        if (result != nullptr && result->unsupported_algorithm != nullptr)
            stream << "Unsupported algo: " << result->unsupported_algorithm << Qt::endl;
    }

    if(result != nullptr) {
        if (result->file_name != nullptr)
            stream << "File name: " << result->file_name << Qt::endl;
        stream << Qt::endl;

        auto reci = result->recipients;
        if (reci != nullptr)
            stream << "Recipient(s): " << Qt::endl;
        while (reci != nullptr) {
            printReci(stream, reci);
            reci = reci->next;
        }
    }

    stream << "-----" << Qt::endl << Qt::endl;

}

bool DecryptResultAnalyse::printReci(QTextStream &stream, gpgme_recipient_t reci) {
    bool keyFound = true;
    stream << QApplication::tr(">Recipient: ");

    try {
        auto key = mCtx->getKeyById(reci->keyid);
        stream << key.name;
        if (!key.email.isEmpty()) {
            stream << "<" << key.email << ">";
        }
    } catch(std::runtime_error &ignored) {
        stream << "<Unknown>";
        setStatus(0);
        keyFound = false;
    }
    stream << Qt::endl;

    stream << "Public algo: " << gpgme_pubkey_algo_name(reci->pubkey_algo) << Qt::endl << Qt::endl;
    return keyFound;
}
