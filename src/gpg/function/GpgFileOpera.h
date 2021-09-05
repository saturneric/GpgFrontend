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

#ifndef GPGFRONTEND_GPGFILEOPERA_H
#define GPGFRONTEND_GPGFILEOPERA_H

#include "gpg/GpgConstants.h"
#include "gpg/GpgContext.h"
#include "gpg/GpgModel.h"

namespace GpgFrontend {

class GpgFileOpera {
public:
  GpgError EncryptFile(KeyArgsList &keys, const std::string &path,
                       GpgEncrResult &result);

  GpgError DecryptFile(const QString &path, GpgDecrResult &result);

  GpgError SignFile(KeyArgsList &keys, const std::string &path,
                    GpgSignResult &result);

  GpgError VerifyFile(const std::string &path, GpgVerifyResult &result);

  GpgError EncryptSignFile(KeyArgsList &keys, const std::string &path,
                           GpgEncrResult &encr_res, GpgSignResult &sign_res);

  GpgError DecryptVerifyFile(const std::string &path, GpgDecrResult &decr_res,
                             GpgVerifyResult &verify_res);

private:
  GpgContext &ctx = GpgContext::GetInstance();
};

} // namespace GpgFrontend

#endif // GPGFRONTEND_GPGFILEOPERA_H
