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

#include "gpg/result_analyse/SignResultAnalyse.h"
#include "gpg/function/GpgKeyGetter.h"

GpgFrontend::SignResultAnalyse::SignResultAnalyse(GpgError error,
                                                  GpgSignResult result)
    : error(error), result(std::move(result)) {}

void GpgFrontend::SignResultAnalyse::do_analyse() {
  qDebug() << "Start Sign Result Analyse";

  stream << tr("[#] Sign Operation ").constData();

  if (gpgme_err_code(error) == GPG_ERR_NO_ERROR)
    stream << tr("[Success]").constData() << std::endl;
  else {
    stream << tr("[Failed] ").constData() << gpgme_strerror(error) << std::endl;
    setStatus(-1);
  }

  if (result != nullptr &&
      (result->signatures != nullptr || result->invalid_signers != nullptr)) {

    qDebug() << "Sign Result Analyse Getting Result";
    stream << "------------>" << std::endl;
    auto new_sign = result->signatures;

    while (new_sign != nullptr) {
      stream << tr("[>] New Signature: ").constData() << std::endl;

      qDebug() << "Signers Fingerprint: " << new_sign->fpr;

      stream << tr("    Sign Mode: ").constData();
      if (new_sign->type == GPGME_SIG_MODE_NORMAL)
        stream << tr("Normal").constData();
      else if (new_sign->type == GPGME_SIG_MODE_CLEAR)
        stream << tr("Clear").constData();
      else if (new_sign->type == GPGME_SIG_MODE_DETACH)
        stream << tr("Detach").constData();

      stream << std::endl;

      auto singerKey =
          GpgFrontend::GpgKeyGetter::GetInstance().GetKey(new_sign->fpr);
      if (singerKey.good()) {
        stream << tr("    Signer: ").constData()
               << singerKey.uids()->front().uid() << std::endl;
      } else {
        stream << tr("    Signer: ").constData() << tr("<unknown>").constData()
               << std::endl;
      }
      stream << tr("    Public Key Algo: ").constData()
             << gpgme_pubkey_algo_name(new_sign->pubkey_algo) << std::endl;
      stream << tr("    Hash Algo: ").constData()
             << gpgme_hash_algo_name(new_sign->hash_algo) << std::endl;
      stream
          << tr("    Date & Time: ").constData()
          << QDateTime::fromTime_t(new_sign->timestamp).toString().constData()
          << std::endl;

      stream << std::endl;

      new_sign = new_sign->next;
    }

    qDebug() << "Sign Result Analyse Getting Invalid Signer";

    auto invalid_signer = result->invalid_signers;

    if (invalid_signer != nullptr)
      stream << tr("Invalid Signers: ").constData() << std::endl;

    while (invalid_signer != nullptr) {
      setStatus(0);
      stream << tr("[>] Signer: ").constData() << std::endl;
      stream << tr("      Fingerprint: ").constData() << invalid_signer->fpr
             << std::endl;
      stream << tr("      Reason: ").constData()
             << gpgme_strerror(invalid_signer->reason) << std::endl;
      stream << std::endl;

      invalid_signer = invalid_signer->next;
    }
    stream << "<------------" << std::endl;
  }
}