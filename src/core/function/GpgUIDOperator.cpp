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

#include "core/function/GpgUIDOperator.h"

#include "boost/format.hpp"

bool GpgFrontend::GpgUIDOperator::AddUID(const GpgFrontend::GpgKey& key,
                                      const std::string& uid) {
  auto err = gpgme_op_adduid(ctx_, gpgme_key_t(key), uid.c_str(), 0);
  if (check_gpg_error_2_err_code(err) == GPG_ERR_NO_ERROR)
    return true;
  else
    return false;
}

bool GpgFrontend::GpgUIDOperator::RevUID(const GpgFrontend::GpgKey& key,
                                      const std::string& uid) {
  auto err =
      check_gpg_error(gpgme_op_revuid(ctx_, gpgme_key_t(key), uid.c_str(), 0));
  if (check_gpg_error_2_err_code(err) == GPG_ERR_NO_ERROR)
    return true;
  else
    return false;
}

bool GpgFrontend::GpgUIDOperator::SetPrimaryUID(const GpgFrontend::GpgKey& key,
                                             const std::string& uid) {
  auto err = check_gpg_error(gpgme_op_set_uid_flag(
      ctx_, gpgme_key_t(key), uid.c_str(), "primary", nullptr));
  if (check_gpg_error_2_err_code(err) == GPG_ERR_NO_ERROR)
    return true;
  else
    return false;
}
bool GpgFrontend::GpgUIDOperator::AddUID(const GpgFrontend::GpgKey& key,
                                      const std::string& name,
                                      const std::string& comment,
                                      const std::string& email) {
  LOG(INFO) << "GpgFrontend::UidOperator::AddUID" << name << comment << email;
  auto uid = boost::format("%1%(%2%)<%3%>") % name % comment % email;
  return AddUID(key, uid.str());
}
