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

#include "GpgEncryptResultAnalyse.h"

#include "core/model/GpgEncryptResult.h"

namespace GpgFrontend {

GpgEncryptResultAnalyse::GpgEncryptResultAnalyse(int channel, GpgError error,
                                                 GpgEncryptResult result)
    : GpgResultAnalyse(channel), error_(error), result_(result) {}

void GpgEncryptResultAnalyse::doAnalyse() {
  setOperation(kOperation);
  recordEngineError(error_, result_.ErrorDetail());

  stream_ << "# " << tr("Encrypt Operation") << " (" << EngineInfo() << ") ";

  if (gpgme_err_code(error_) == GPG_ERR_NO_ERROR) {
    stream_ << "- " << tr("Success") << " " << Qt::endl;
  } else {
    stream_ << "- " << tr("Failed") << ": " << gpgme_strerror(error_)
            << Qt::endl;
    if (!result_.ErrorDetail().isEmpty()) {
      stream_ << "    - " << tr("Detail") << ": " << result_.ErrorDetail()
              << Qt::endl;
    }
    setStatus(-1);
  }

  // Read the engine-agnostic list: rPGP results carry no gpgme handle, so
  // GetRaw() alone would drop every recipient the rPGP engine rejected.
  const auto invalid_recipients = result_.InvalidRecipients();

  // Data encrypted to fewer recipients than asked for is a warning, not a
  // clean success.
  if (status_ > 0 && !invalid_recipients.empty()) setStatus(0);

  if (status_ <= 0) {
    stream_ << Qt::endl;

    if (!invalid_recipients.empty()) {
      stream_ << "## " << tr("Invalid Recipients") << ": " << Qt::endl
              << Qt::endl;

      auto index = 0;
      for (const auto& [fpr, reason] : invalid_recipients) {
        stream_ << "### " << tr("Recipients") << " " << ++index << ": "
                << Qt::endl;
        stream_ << "- " << tr("Fingerprint") << ": " << fpr << Qt::endl;
        stream_ << "- " << tr("Reason") << ": " << gpgme_strerror(reason)
                << Qt::endl;
        stream_ << Qt::endl << Qt::endl;
      }
    }
    stream_ << Qt::endl;
  }

  stream_ << Qt::endl;

  if (status_ >= 0) {
    op_info_.description =
        !invalid_recipients.empty()
            ? tr("Encrypted with warnings. Some recipients could not be "
                 "added. Please review the invalid recipient list.")
            : tr("Your data has been encrypted. Only the intended recipients "
                 "can decrypt and read it.");
  } else {
    const auto detail = result_.ErrorDetail();
    op_info_.description =
        tr("Encryption failed: %1.")
            .arg(detail.isEmpty() ? QString::fromUtf8(gpgme_strerror(error_))
                                  : detail);
  }
}

}  // namespace GpgFrontend