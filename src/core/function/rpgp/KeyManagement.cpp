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

#include "KeyManagement.h"

#include "core/function/GFKeyDatabase.h"

namespace GpgFrontend {

auto DeleteKeysRpgpImpl(GpgContext& ctx, const GpgAbstractKeyPtrList& keys)
    -> GpgError {
  for (const auto& key : keys) {
    LOG_D() << "Deleting key: " << key->ID();

    auto key_db = ctx.KeyDatabase();
    if (key_db == nullptr) {
      LOG_E() << "key database is not initialized";
      return GPG_ERR_GENERAL;
    }

    if (!key_db->GetKeyMetadata(key->ID())) {
      LOG_E() << "key not found in database: " << key->ID();
    }

    if (!key_db->DeleteKey(key->ID())) {
      LOG_E() << "failed to delete key from database: " << key->ID();
    }
  }
  return GPG_ERR_NO_ERROR;
}
}  // namespace GpgFrontend