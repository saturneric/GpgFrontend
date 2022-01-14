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

#ifndef GPGFRONTEND_ZH_CN_TS_GPGKEYMANAGER_H
#define GPGFRONTEND_ZH_CN_TS_GPGKEYMANAGER_H

#include "gpg/GpgContext.h"
#include "gpg/GpgFunctionObject.h"
#include "gpg/GpgModel.h"

namespace GpgFrontend {

/**
 * @brief
 *
 */
class GpgKeyManager : public SingletonFunctionObject<GpgKeyManager> {
 public:
  /**
   * @brief Construct a new Gpg Key Manager object
   *
   * @param channel
   */
  explicit GpgKeyManager(
      int channel = SingletonFunctionObject::GetDefaultChannel())
      : SingletonFunctionObject<GpgKeyManager>(channel) {}

  /**
   * @brief Sign a key pair(actually a certain uid)
   * @param target target key pair
   * @param uid target
   * @param expires expire date and time of the signature
   * @return if successful
   */
  bool SignKey(const GpgKey& target, KeyArgsList& keys, const std::string& uid,
               const std::unique_ptr<boost::posix_time::ptime>& expires);

  /**
   * @brief
   *
   * @param key
   * @param signature_id
   * @return true
   * @return false
   */
  bool RevSign(const GpgFrontend::GpgKey& key,
               const GpgFrontend::SignIdArgsListPtr& signature_id);

  /**
   * @brief Set the Expire object
   *
   * @param key
   * @param subkey
   * @param expires
   * @return true
   * @return false
   */
  bool SetExpire(const GpgKey& key, std::unique_ptr<GpgSubKey>& subkey,
                 std::unique_ptr<boost::posix_time::ptime>& expires);

 private:
  GpgContext& ctx_ =
      GpgContext::GetInstance(SingletonFunctionObject::GetChannel());  ///<
};

}  // namespace GpgFrontend

#endif  // GPGFRONTEND_ZH_CN_TS_GPGKEYMANAGER_H
