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

#include "GpgUIDOperator.h"

#include "core/GpgModel.h"
#include "core/utils/GpgUtils.h"

namespace GpgFrontend {

GpgUIDOperator::GpgUIDOperator(int channel)
    : SingletonFunctionObject<GpgUIDOperator>(channel) {}

auto GpgUIDOperator::AddUID(const GpgKey& key, const QString& uid) -> bool {
  auto err = gpgme_op_adduid(ctx_.DefaultContext(),
                             static_cast<gpgme_key_t>(key), uid.toUtf8(), 0);
  return CheckGpgError(err) == GPG_ERR_NO_ERROR;
}

auto GpgUIDOperator::RevUID(const GpgKey& key, const QString& uid) -> bool {
  auto err = CheckGpgError(gpgme_op_revuid(
      ctx_.DefaultContext(), static_cast<gpgme_key_t>(key), uid.toUtf8(), 0));
  return CheckGpgError(err) == GPG_ERR_NO_ERROR;
}

auto GpgUIDOperator::SetPrimaryUID(const GpgKey& key,
                                   const QString& uid) -> bool {
  auto err = CheckGpgError(gpgme_op_set_uid_flag(
      ctx_.DefaultContext(), static_cast<gpgme_key_t>(key), uid.toUtf8(),
      "primary", nullptr));
  return CheckGpgError(err) == GPG_ERR_NO_ERROR;
}
auto GpgUIDOperator::AddUID(const GpgKey& key, const QString& name,
                            const QString& comment,
                            const QString& email) -> bool {
  qCDebug(core) << "new uuid:" << name << comment << email;
  return AddUID(key, QString("%1(%2)<%3>").arg(name).arg(comment).arg(email));
}

}  // namespace GpgFrontend
