/**
 * Copyright (C) 2021 Saturneric
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
 * Saturneric<eric@bktus.com> starting on May 12, 2021.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include "GpgEncryptResultAnalyse.h"

GpgFrontend::GpgEncryptResultAnalyse::GpgEncryptResultAnalyse(
    GpgError error, GpgEncrResult result)
    : error_(error), result_(std::move(result)) {}

void GpgFrontend::GpgEncryptResultAnalyse::do_analyse() {
  SPDLOG_DEBUG("start encrypt result analyse");

  stream_ << "[#] " << _("Encrypt Operation") << " ";

  if (gpgme_err_code(error_) == GPG_ERR_NO_ERROR)
    stream_ << "[" << _("Success") << "]" << std::endl;
  else {
    stream_ << "[" << _("Failed") << "] " << gpgme_strerror(error_)
            << std::endl;
    set_status(-1);
  }

  if (!~status_) {
    stream_ << "------------>" << std::endl;
    if (result_ != nullptr) {
      stream_ << _("Invalid Recipients") << ": " << std::endl;
      auto inv_reci = result_->invalid_recipients;
      while (inv_reci != nullptr) {
        stream_ << _("Fingerprint") << ": " << inv_reci->fpr << std::endl;
        stream_ << _("Reason") << ": " << gpgme_strerror(inv_reci->reason)
                << std::endl;
        stream_ << std::endl;

        inv_reci = inv_reci->next;
      }
    }
    stream_ << "<------------" << std::endl;
  }

  stream_ << std::endl;
}
