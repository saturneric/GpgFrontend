//
// Created by eric on 2021/6/9.
//

#include "gpg/result_analyse/DecryptResultAnalyse.h"

DecryptResultAnalyse::DecryptResultAnalyse(GpgME::GpgContext *ctx, gpgme_error_t error, gpgme_decrypt_result_t result)
        : mCtx(ctx) {

    if(result == nullptr) {
        return;
    }

    stream << "Decrypt Report: " << endl << "-----" << endl;

    if (gpgme_err_code(error) == GPG_ERR_NO_ERROR) {
        stream << "Status: Success" << endl;
    } else {
        stream << "Status" << gpgme_strerror(error) << endl;
        if (result->unsupported_algorithm != nullptr) {
            stream << "Unsupported algo: " << result->unsupported_algorithm << endl;
        }
    }

    if (result->file_name != nullptr) {
        stream << "File name: " << result->file_name << endl;
    }

    stream << endl;

    auto reci = result->recipients;

    if (reci != nullptr) {
        stream << "Recipient(s): " << endl;
    }
    while (reci != nullptr) {
        printReci(stream, reci);
        reci = reci->next;
    }

}

bool DecryptResultAnalyse::printReci(QTextStream &stream, gpgme_recipient_t reci) {
    bool keyFound = true;
    stream << QApplication::tr("Signed By: ");

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
    stream << endl;

    stream << "Public algo: " << gpgme_pubkey_algo_name(reci->pubkey_algo) << endl;
    return keyFound;
}
