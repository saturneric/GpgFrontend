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

#include "GpgImportInformation.h"

namespace GpgFrontend {

GpgImportInformation::GpgImportInformation() = default;

GpgImportInformation::GpgImportInformation(gpgme_import_result_t result) {
  if (result->unchanged != 0) unchanged = result->unchanged;
  if (result->considered != 0) considered = result->considered;
  if (result->no_user_id != 0) no_user_id = result->no_user_id;
  if (result->imported != 0) imported = result->imported;
  if (result->imported_rsa != 0) imported_rsa = result->imported_rsa;
  if (result->unchanged != 0) unchanged = result->unchanged;
  if (result->new_user_ids != 0) new_user_ids = result->new_user_ids;
  if (result->new_sub_keys != 0) new_sub_keys = result->new_sub_keys;
  if (result->new_signatures != 0) new_signatures = result->new_signatures;
  if (result->new_revocations != 0) new_revocations = result->new_revocations;
  if (result->secret_read != 0) secret_read = result->secret_read;
  if (result->secret_imported != 0) secret_imported = result->secret_imported;
  if (result->secret_unchanged != 0) {
    secret_unchanged = result->secret_unchanged;
  }
  if (result->not_imported != 0) not_imported = result->not_imported;
}

}  // namespace GpgFrontend