/**
 * This file is part of GPGFrontend.
 *
 * GPGFrontend is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Foobar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Foobar.  If not, see <https://www.gnu.org/licenses/>.
 *
 * The initial version of the source code is inherited from gpg4usb-team.
 * Their source code version also complies with GNU General Public License.
 *
 * The source code version of this software was modified and released
 * by Saturneric<eric@bktus.com> starting on May 12, 2021.
 *
 */

#include "gpg/function/UidOperator.h"

#include "boost/format.hpp"

bool GpgFrontend::UidOperator::addUID(const GpgFrontend::GpgKey &key,
                                      const GpgFrontend::GpgUID &uid) {
  auto userid = (boost::format("%1% (%2%) <%3%>") % uid.name() % uid.comment() %
                 uid.email())
                    .str();
  auto err = gpgme_op_adduid(ctx, gpgme_key_t(key), userid.c_str(), 0);
  if (check_gpg_error_2_err_code(err) == GPG_ERR_NO_ERROR)
    return true;
  else
    return false;
}

bool GpgFrontend::UidOperator::revUID(const GpgFrontend::GpgKey &key,
                                      const GpgFrontend::GpgUID &uid) {
  auto err = check_gpg_error(
      gpgme_op_revuid(ctx, gpgme_key_t(key), uid.uid().c_str(), 0));
  if (check_gpg_error_2_err_code(err) == GPG_ERR_NO_ERROR)
    return true;
  else
    return false;
}

bool GpgFrontend::UidOperator::setPrimaryUID(const GpgFrontend::GpgKey &key,
                                             const GpgFrontend::GpgUID &uid) {
  auto err = check_gpg_error(gpgme_op_set_uid_flag(
      ctx, gpgme_key_t(key), uid.uid().c_str(), "primary", nullptr));
  if (check_gpg_error_2_err_code(err) == GPG_ERR_NO_ERROR)
    return true;
  else
    return false;
}
