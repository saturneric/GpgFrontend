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

#include "core/function/gpg/GpgAbstractKeyGetter.h"
#include "core/utils/LocalizedUtils.h"

namespace GpgFrontend {

GpgSignResultAnalyse::GpgSignResultAnalyse(int channel, GpgError error,
                                           GpgSignResult result)
    : GpgResultAnalyse(channel), error_(error), result_(std::move(result)) {}

void GpgSignResultAnalyse::doAnalyse() {
  auto signatures = this->result_.Signatures();
  auto invalid_signers = this->result_.InvalidSigners();

  stream_ << "# " << tr("Sign Operation") << " (" << EngineInfo() << ") ";

  if (gpgme_err_code(error_) == GPG_ERR_NO_ERROR) {
    stream_ << "- " << tr("Success") << " " << Qt::endl;
  } else {
    stream_ << "- " << tr("Failed") << " " << gpgme_strerror(error_)
            << Qt::endl;
    setStatus(-1);
  }

  if (!signatures.empty() || !invalid_signers.empty()) {
    stream_ << Qt::endl;
    auto index = 0;

    for (const auto& sign : signatures) {
      stream_ << "## " << tr("New Signature") << " [" << ++index
              << "]: " << Qt::endl;

      stream_ << "- " << tr("Sign Mode") << ": ";
      stream_ << sign.GetSigType();
      stream_ << Qt::endl;

      QString fpr = sign.GetFingerprint();
      auto sign_key =
          GpgAbstractKeyGetter::GetInstance(GetChannel()).GetKey(fpr);
      if (sign_key != nullptr) {
        stream_ << "- " << tr("Signed By") << ": " << sign_key->UID()
                << Qt::endl;

        if (sign_key->KeyType() == GpgAbstractKeyType::kGPG_SUBKEY) {
          stream_ << "- " << tr("Key ID") << ": " << sign_key->ID() << " ("
                  << tr("Subkey") << ")" << Qt::endl;
        } else {
          stream_ << "- " << tr("Key ID") << ": " << sign_key->ID() << " ("
                  << tr("Primary Key") << ")" << Qt::endl;
        }
        stream_ << "- " << tr("Key Create Date") << ": "
                << QLocale().toString(sign_key->CreationTime()) << Qt::endl;

      } else {
        stream_ << "- " << tr("Signed By") << "(" << tr("Fingerprint") << ")"
                << ": " << (fpr.isEmpty() ? tr("<unknown>") : fpr) << Qt::endl;
      }

      stream_ << "- " << tr("Public Key Algo") << ": " << sign.GetPubkeyAlgo()
              << Qt::endl;
      stream_ << "- " << tr("Hash Algo") << ": " << sign.GetHashAlgo()
              << Qt::endl;
      stream_ << "- " << tr("Sign Date") << "(" << tr("UTC") << ")"
              << ": "
              << GetUTCDateByTimestamp(sign.GetCreateTime().toSecsSinceEpoch())
              << Qt::endl;
      stream_ << "- " << tr("Sign Date") << "(" << tr("Localized") << ")"
              << ": "
              << GetLocalizedDateByTimestamp(
                     sign.GetCreateTime().toSecsSinceEpoch())
              << Qt::endl;

      stream_ << Qt::endl
              << "---------------------------------------" << Qt::endl
              << Qt::endl;
    }

    stream_ << Qt::endl;

    if (!invalid_signers.empty()) {
      stream_ << "## " << tr("Invalid Signers") << ": " << Qt::endl;
    }

    index = 0;
    for (const auto& invalid_signer : invalid_signers) {
      setStatus(0);
      stream_ << "### " << tr("Signer") << " [" << ++index << "]: " << Qt::endl
              << Qt::endl;
      stream_ << "- " << tr("Fingerprint") << ": " << invalid_signer.first
              << Qt::endl;
      stream_ << "- " << tr("Reason") << ": "
              << gpgme_strerror(invalid_signer.second) << Qt::endl;
      stream_ << "---------------------------------------" << Qt::endl;
    }
    stream_ << Qt::endl;
  }
}

}  // namespace GpgFrontend