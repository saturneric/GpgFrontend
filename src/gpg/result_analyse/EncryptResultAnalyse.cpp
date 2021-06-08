//
// Created by eric on 2021/6/9.
//

#include "gpg/result_analyse/EncryptResultAnalyse.h"

EncryptResultAnalyse::EncryptResultAnalyse(gpgme_error_t error, gpgme_encrypt_result_t result) {

    if(result == nullptr) {
        return;
    }

    stream << "# Encrypt Report: " << endl << "-----" << endl;

    if(gpgme_err_code(error) == GPG_ERR_NO_ERROR) {
        stream << "Status: Encrypt Success" << endl;
    }
    else {
        stream << "Status:" << gpgme_strerror(error) << endl;
        stream << "Invalid Recipients: " << endl;
        setStatus(0);
        auto inv_reci = result->invalid_recipients;
        while(inv_reci != nullptr) {
            stream << "Fingerprint: " << inv_reci->fpr << endl;
            stream << "Reason: " << gpgme_strerror(inv_reci->reason) << endl;
            stream << endl;

            inv_reci = inv_reci->next;
        }
    }

    stream << "-----" << endl;
    stream << endl;

}
