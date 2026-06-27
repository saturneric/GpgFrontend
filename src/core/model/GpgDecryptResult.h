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

#pragma once

#include "core/model/GpgRecipient.h"
#include "core/typedef/GpgTypedef.h"

namespace GpgFrontend {

class GF_CORE_EXPORT GpgDecryptResult {
 public:
  auto IsGood() -> bool;

  auto GetRaw() -> gpgme_decrypt_result_t;

  auto Recipients() -> QContainer<GpgRecipient>;

  auto UnsupportedAlgorithm() -> QString;

  auto Filename() -> QString;

  auto MIME() -> bool;

  auto MessageIntegrityProtected() -> bool;

  auto SymmetricEncryptionAlgorithm() -> QString;

  /// Engine-specific, human-readable failure detail (rPGP only). Empty for the
  /// GnuPG engine, where `gpgme_strerror` already provides the description.
  auto ErrorDetail() -> QString;

  explicit GpgDecryptResult(gpgme_decrypt_result_t);

  explicit GpgDecryptResult(const GFDecryptResult&);

  GpgDecryptResult();

  virtual ~GpgDecryptResult();

 private:
  QSharedPointer<struct _gpgme_op_decrypt_result> result_ref_ = nullptr;  ///<
  QSharedPointer<struct GFDecryptResult> gf_result_ref_ = nullptr;        ///<
};

}  // namespace GpgFrontend