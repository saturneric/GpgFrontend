/**
 * Copyright (C) 2021-2024 Saturneric <eric@bktus.com>
 *
 * This file is part of GpgFrontend.
 *
 * GpgFrontend is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * GpgFrontend is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GpgFrontend. If not, see <https://www.gnu.org/licenses/>.
 *
 * The initial version of the source code is inherited from
 * the gpg4usb project, which is under GPL-3.0-or-later.
 *
 * All the source code of GpgFrontend was modified and released by
 * Saturneric <eric@bktus.com> starting on May 12, 2021.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include "GpgSignResultAnalyse.h"

#include "core/GpgModel.h"
#include "core/function/gpg/GpgKeyGetter.h"
#include "core/utils/LocalizedUtils.h"

namespace GpgFrontend {

GpgSignResultAnalyse::GpgSignResultAnalyse(int channel, GpgError error,
                                           GpgSignResult result)
    : GpgResultAnalyse(channel), error_(error), result_(std::move(result)) {}

void GpgSignResultAnalyse::doAnalyse() {
  auto *result = this->result_.GetRaw();

  stream_ << "# " << tr("Sign Operation") << " ";

  if (gpgme_err_code(error_) == GPG_ERR_NO_ERROR) {
    stream_ << "- " << tr("Success") << " " << Qt::endl;
  } else {
    stream_ << "- " << tr("Failed") << " " << gpgme_strerror(error_)
            << Qt::endl;
    setStatus(-1);
  }

  if (result != nullptr &&
      (result->signatures != nullptr || result->invalid_signers != nullptr)) {
    stream_ << Qt::endl;
    auto *new_sign = result->signatures;
    auto index = 0;

    while (new_sign != nullptr) {
      stream_ << "## " << tr("New Signature") << " [" << ++index
              << "]: " << Qt::endl;

      stream_ << "- " << tr("Sign Mode") << ": ";
      if (new_sign->type == GPGME_SIG_MODE_NORMAL) {
        stream_ << tr("Normal");
      } else if (new_sign->type == GPGME_SIG_MODE_CLEAR) {
        stream_ << tr("Clear");
      } else if (new_sign->type == GPGME_SIG_MODE_DETACH) {
        stream_ << tr("Detach");
      }

      stream_ << Qt::endl;

      QString fpr = new_sign->fpr == nullptr ? "" : new_sign->fpr;
      auto singer_key = GpgKeyGetter::GetInstance(GetChannel()).GetKey(fpr);
      if (singer_key.IsGood()) {
        stream_ << "- " << tr("Signed By") << ": "
                << singer_key.UIDs().front().GetUID() << Qt::endl;

        auto s_keys = singer_key.SubKeys();
        auto it = std::find_if(
            s_keys.begin(), s_keys.end(),
            [fpr](const GpgSubKey &k) { return k.Fingerprint() == fpr; });

        if (it != s_keys.end()) {
          auto &subkey = *it;
          if (subkey.Fingerprint() != singer_key.Fingerprint()) {
            stream_ << "- " << tr("Key ID") << ": " << singer_key.ID() << " ("
                    << tr("Subkey") << ")" << Qt::endl;
          } else {
            stream_ << "- " << tr("Key ID") << ": " << singer_key.ID() << " ("
                    << tr("Primary Key") << ")" << Qt::endl;
          }
          stream_ << "- " << tr("Key Create Date") << ": "
                  << QLocale().toString(subkey.CreationTime()) << Qt::endl;
        }
      } else {
        stream_ << "- " << tr("Signed By") << "(" << tr("Fingerprint") << ")"
                << ": " << (fpr.isEmpty() ? tr("<unknown>") : fpr) << Qt::endl;
      }
      stream_ << "- " << tr("Public Key Algo") << ": "
              << gpgme_pubkey_algo_name(new_sign->pubkey_algo) << Qt::endl;
      stream_ << "- " << tr("Hash Algo") << ": "
              << gpgme_hash_algo_name(new_sign->hash_algo) << Qt::endl;
      stream_ << "- " << tr("Sign Date") << "(" << tr("UTC") << ")" << ": "
              << GetUTCDateByTimestamp(new_sign->timestamp) << Qt::endl;
      stream_ << "- " << tr("Sign Date") << "(" << tr("Localized") << ")"
              << ": " << GetLocalizedDateByTimestamp(new_sign->timestamp)
              << Qt::endl;

      stream_ << Qt::endl
              << "---------------------------------------" << Qt::endl
              << Qt::endl;

      new_sign = new_sign->next;
    }

    auto *invalid_signer = result->invalid_signers;
    stream_ << Qt::endl;

    if (invalid_signer != nullptr) {
      stream_ << "## " << tr("Invalid Signers") << ": " << Qt::endl;
    }

    index = 0;
    while (invalid_signer != nullptr) {
      setStatus(0);
      stream_ << "### " << tr("Signer") << " [" << ++index << "]: " << Qt::endl
              << Qt::endl;
      stream_ << "- " << tr("Fingerprint") << ": " << invalid_signer->fpr
              << Qt::endl;
      stream_ << "- " << tr("Reason") << ": "
              << gpgme_strerror(invalid_signer->reason) << Qt::endl;
      stream_ << "---------------------------------------" << Qt::endl;

      invalid_signer = invalid_signer->next;
    }
    stream_ << Qt::endl;
  }
}

}  // namespace GpgFrontend