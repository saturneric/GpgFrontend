/**
 * This file is part of GpgFrontend.
 *
 * GpgFrontend is free software: you can redistribute it and/or modify
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

#ifndef _GPGKEYOPERA_H
#define _GPGKEYOPERA_H

#include "gpg/GpgConstants.h"
#include "gpg/GpgContext.h"
#include "gpg/GpgModel.h"

namespace GpgFrontend {
class GenKeyInfo;
class GpgKeyOpera : public SingletonFunctionObject<GpgKeyOpera> {
 public:
  void DeleteKeys(KeyIdArgsListPtr key_ids);

  GpgError SetExpire(const GpgKey& key, const SubkeyId& subkey_fpr,
                     std::unique_ptr<boost::gregorian::date>& expires);

  static void GenerateRevokeCert(const GpgKey& key,
                                 const std::string& output_file_name);

  GpgFrontend::GpgError GenerateKey(const std::unique_ptr<GenKeyInfo>& params);

  GpgFrontend::GpgError GenerateSubkey(
      const GpgKey& key, const std::unique_ptr<GenKeyInfo>& params);

 private:
  GpgContext& ctx = GpgContext::GetInstance();
};
}  // namespace GpgFrontend

#endif  // _GPGKEYOPERA_H