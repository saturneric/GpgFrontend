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

#include "gpg/result_analyse/VerifyResultAnalyse.h"
#include "GpgFrontend.h"
#include "gpg/GpgConstants.h"
#include "gpg/function/GpgKeyGetter.h"

GpgFrontend::VerifyResultAnalyse::VerifyResultAnalyse(GpgError error,
                                                      GpgVerifyResult result)
    : error(error), result(std::move(result)) {}

void GpgFrontend::VerifyResultAnalyse::do_analyse() {
  qDebug() << "Verify Result Analyse Started";

  stream << tr("[#] Verify Operation ").constData();

  if (gpgme_err_code(error) == GPG_ERR_NO_ERROR)
    stream << tr("[Success]").constData() << std::endl;
  else {
    stream << tr("[Failed] ").constData() << gpgme_strerror(error) << std::endl;
    setStatus(-1);
  }

  if (result != nullptr && result->signatures) {
    stream << "------------>" << std::endl;
    auto sign = result->signatures;

    if (sign == nullptr) {
      stream << "[>] Not Signature Found" << std::endl;
      setStatus(0);
      return;
    }

    stream << "[>] Signed On "
           << QDateTime::fromTime_t(sign->timestamp).toString().constData()
           << std::endl;

    stream << std::endl << "[>] Signatures:" << std::endl;

    bool canContinue = true;

    while (sign && canContinue) {

      switch (gpg_err_code(sign->status)) {
      case GPG_ERR_BAD_SIGNATURE:
        stream << tr("One or More Bad Signatures.").constData() << std::endl;
        canContinue = false;
        setStatus(-1);
        break;
      case GPG_ERR_NO_ERROR:
        stream << tr("A ").constData();
        if (sign->summary & GPGME_SIGSUM_GREEN) {
          stream << tr("Good ").constData();
        }
        if (sign->summary & GPGME_SIGSUM_RED) {
          stream << tr("Bad ").constData();
        }
        if (sign->summary & GPGME_SIGSUM_SIG_EXPIRED) {
          stream << tr("Expired ").constData();
        }
        if (sign->summary & GPGME_SIGSUM_KEY_MISSING) {
          stream << tr("Missing Key's ").constData();
        }
        if (sign->summary & GPGME_SIGSUM_KEY_REVOKED) {
          stream << tr("Revoked Key's ").constData();
        }
        if (sign->summary & GPGME_SIGSUM_KEY_EXPIRED) {
          stream << tr("Expired Key's ").constData();
        }
        if (sign->summary & GPGME_SIGSUM_CRL_MISSING) {
          stream << tr("Missing CRL's ").constData();
        }

        if (sign->summary & GPGME_SIGSUM_VALID) {
          stream << tr("Signature Fully Valid.").constData() << std::endl;
        } else {
          stream << tr("Signature Not Fully Valid.").constData() << std::endl;
        }

        if (!(sign->status & GPGME_SIGSUM_KEY_MISSING)) {
          if (!print_signer(stream, sign))
            setStatus(0);
        } else {
          stream << tr("Key is NOT present with ID 0x").constData() << sign->fpr
                 << std::endl;
        }

        setStatus(1);

        break;
      case GPG_ERR_NO_PUBKEY:
        stream << tr("A signature could NOT be verified due to a Missing Key\n")
                      .constData();
        setStatus(-1);
        break;
      case GPG_ERR_CERT_REVOKED:
        stream << tr("A signature is valid but the key used to "
                     "verify the signature has been revoked\n")
                      .constData();
        if (!print_signer(stream, sign)) {
          setStatus(0);
        }
        setStatus(-1);
        break;
      case GPG_ERR_SIG_EXPIRED:
        stream << tr("A signature is valid but expired\n").constData();
        if (!print_signer(stream, sign)) {
          setStatus(0);
        }
        setStatus(-1);
        break;
      case GPG_ERR_KEY_EXPIRED:
        stream << tr("A signature is valid but the key used to "
                     "verify the signature has expired.\n")
                      .constData();
        if (!print_signer(stream, sign)) {
          setStatus(0);
        }
        break;
      case GPG_ERR_GENERAL:
        stream << tr("There was some other error which prevented "
                     "the signature verification.\n")
                      .constData();
        status = -1;
        canContinue = false;
        break;
      default:
        stream << tr("Error for key with fingerprint ").constData()
               << GpgFrontend::beautify_fingerprint(sign->fpr);
        setStatus(-1);
      }
      stream << std::endl;
      sign = sign->next;
    }
    stream << "<------------" << std::endl;
  }
}

bool GpgFrontend::VerifyResultAnalyse::print_signer(std::stringstream &stream,
                                                    gpgme_signature_t sign) {
  bool keyFound = true;
  auto key = GpgFrontend::GpgKeyGetter::GetInstance().GetKey(sign->fpr);

  if (!key.good()) {
    stream << tr("    Signed By: ").constData() << tr("<unknown>").constData()
           << std::endl;
    setStatus(0);
    keyFound = false;
  } else {
    stream << tr("    Signed By: ").constData() << key.uids()->front().uid()
           << std::endl;
  }
  stream << tr("    Public Key Algo: ").constData()
         << gpgme_pubkey_algo_name(sign->pubkey_algo) << std::endl;
  stream << tr("    Hash Algo: ").constData()
         << gpgme_hash_algo_name(sign->hash_algo) << std::endl;
  stream << tr("    Date & Time: ").constData()
         << QDateTime::fromTime_t(sign->timestamp).toString().constData()
         << std::endl;
  stream << std::endl;
  return keyFound;
}
