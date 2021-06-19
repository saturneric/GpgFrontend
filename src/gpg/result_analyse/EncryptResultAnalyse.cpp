//
// Created by eric on 2021/6/9.
//

#include "gpg/result_analyse/EncryptResultAnalyse.h"

EncryptResultAnalyse::EncryptResultAnalyse(gpgme_error_t error, gpgme_encrypt_result_t result) {

    stream << "# Encrypt Report: " << Qt::endl << "-----" << Qt::endl;

    if(gpgme_err_code(error) == GPG_ERR_NO_ERROR) {
        stream << "Status: Encrypt Success" << Qt::endl;
    }
    else {
        stream << "Status:" << gpgme_strerror(error) << Qt::endl;
        setStatus(-1);
        if (result != nullptr) {
            stream << "Invalid Recipients: " << Qt::endl;
            auto inv_reci = result->invalid_recipients;
            while (inv_reci != nullptr) {
                stream << "Fingerprint: " << inv_reci->fpr << Qt::endl;
                stream << "Reason: " << gpgme_strerror(inv_reci->reason) << Qt::endl;
                stream << Qt::endl;

                inv_reci = inv_reci->next;
            }
        }
    }

    stream << "-----" << Qt::endl;
    stream << Qt::endl;

}
