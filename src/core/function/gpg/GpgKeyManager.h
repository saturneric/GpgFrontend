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

#ifndef GPGFRONTEND_ZH_CN_TS_GPGKEYMANAGER_H
#define GPGFRONTEND_ZH_CN_TS_GPGKEYMANAGER_H

#include <functional>
#include <string>

#include "core/GpgContext.h"
#include "core/GpgFunctionObject.h"
#include "core/GpgModel.h"

namespace GpgFrontend {

/**
 * @brief
 *
 */
class GPGFRONTEND_CORE_EXPORT GpgKeyManager
    : public SingletonFunctionObject<GpgKeyManager> {
 public:
  /**
   * @brief Construct a new Gpg Key Manager object
   *
   * @param channel
   */
  explicit GpgKeyManager(
      int channel = SingletonFunctionObject::GetDefaultChannel());

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

  /**
   * @brief
   *
   * @return
   */
  bool SetOwnerTrustLevel(const GpgKey& key, int trust_level);

 private:
  static gpgme_error_t interactor_cb_fnc(void* handle, const char* status,
                                         const char* args, int fd);

  using Command = std::string;
  using AutomatonState = enum {
    AS_START,
    AS_COMMAND,
    AS_VALUE,
    AS_REALLY_ULTIMATE,
    AS_SAVE,
    AS_ERROR,
    AS_QUIT,
  };

  struct AutomatonHandelStruct;

  using AutomatonActionHandler =
      std::function<Command(AutomatonHandelStruct&, AutomatonState)>;
  using AutomatonNextStateHandler =
      std::function<AutomatonState(AutomatonState, std::string, std::string)>;

  struct AutomatonHandelStruct {
    void SetStatus(AutomatonState next_state) { current_state_ = next_state; }
    AutomatonState CuurentStatus() { return current_state_; }
    void SetHandler(AutomatonNextStateHandler next_state_handler,
                    AutomatonActionHandler action_handler) {
      next_state_handler_ = next_state_handler;
      action_handler_ = action_handler;
    }
    AutomatonState NextState(std::string gpg_status, std::string args) {
      return next_state_handler_(current_state_, gpg_status, args);
    }
    Command Action() { return action_handler_(*this, current_state_); }

    void SetSuccess(bool success) { success_ = success; }

    bool Success() { return success_; }

    std::string KeyFpr() { return key_fpr_; }

    AutomatonHandelStruct(std::string key_fpr) : key_fpr_(key_fpr) {}

   private:
    AutomatonState current_state_ = AS_START;
    AutomatonNextStateHandler next_state_handler_;
    AutomatonActionHandler action_handler_;
    bool success_ = false;
    std::string key_fpr_;
  };

  GpgContext& ctx_ =
      GpgContext::GetInstance(SingletonFunctionObject::GetChannel());  ///<
};

}  // namespace GpgFrontend

#endif  // GPGFRONTEND_ZH_CN_TS_GPGKEYMANAGER_H
