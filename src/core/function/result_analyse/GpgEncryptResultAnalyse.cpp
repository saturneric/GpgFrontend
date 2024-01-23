/**
 * Copyright (C) 2021 Saturneric <eric@bktus.com>
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

GpgEncryptResultAnalyse::GpgEncryptResultAnalyse(GpgError error,
                                                 GpgEncryptResult result)
    : error_(error), result_(result) {}

void GpgEncryptResultAnalyse::doAnalyse() {
  stream_ << "# " << tr("Encrypt Operation") << " ";

  if (gpgme_err_code(error_) == GPG_ERR_NO_ERROR) {
    stream_ << "- " << tr("Success") << " " << Qt::endl;
  } else {
    stream_ << "- " << tr("Failed") << ": " << gpgme_strerror(error_)
            << Qt::endl;
    setStatus(-1);
  }

  if ((~status_) == 0) {
    stream_ << Qt::endl;

    const auto *result = result_.GetRaw();

    if (result != nullptr) {
      stream_ << "## " << tr("Invalid Recipients") << ": " << Qt::endl
              << Qt::endl;

      auto *inv_reci = result->invalid_recipients;
      auto index = 0;

      while (inv_reci != nullptr) {
        stream_ << "### " << tr("Recipients") << " " << ++index << ": "
                << Qt::endl;
        stream_ << "- " << tr("Fingerprint") << ": " << inv_reci->fpr
                << Qt::endl;
        stream_ << "- " << tr("Reason") << ": "
                << gpgme_strerror(inv_reci->reason) << Qt::endl;
        stream_ << Qt::endl << Qt::endl;

        inv_reci = inv_reci->next;
      }
    }
    stream_ << Qt::endl;
  }

  stream_ << Qt::endl;
}

}  // namespace GpgFrontend