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

#include "GpgKeyManager.h"

#include <boost/algorithm/string.hpp>
#include <boost/date_time/posix_time/conversion.hpp>
#include <memory>
#include <string>

#include "GpgBasicOperator.h"
#include "GpgKeyGetter.h"
#include "spdlog/spdlog.h"

GpgFrontend::GpgKeyManager::GpgKeyManager(int channel)
    : SingletonFunctionObject<GpgKeyManager>(channel) {}

bool GpgFrontend::GpgKeyManager::SignKey(
    const GpgFrontend::GpgKey& target, GpgFrontend::KeyArgsList& keys,
    const std::string& uid,
    const std::unique_ptr<boost::posix_time::ptime>& expires) {
  using namespace boost::posix_time;

  GpgBasicOperator::GetInstance().SetSigners(keys);

  unsigned int flags = 0;
  unsigned int expires_time_t = 0;

  if (expires == nullptr)
    flags |= GPGME_KEYSIGN_NOEXPIRE;
  else
    expires_time_t = to_time_t(*expires);

  auto err = check_gpg_error(gpgme_op_keysign(
      ctx_, gpgme_key_t(target), uid.c_str(), expires_time_t, flags));

  return check_gpg_error_2_err_code(err) == GPG_ERR_NO_ERROR;
}

bool GpgFrontend::GpgKeyManager::RevSign(
    const GpgFrontend::GpgKey& key,
    const GpgFrontend::SignIdArgsListPtr& signature_id) {
  auto& key_getter = GpgKeyGetter::GetInstance();

  for (const auto& sign_id : *signature_id) {
    auto signing_key = key_getter.GetKey(sign_id.first);
    assert(signing_key.IsGood());
    auto err = check_gpg_error(gpgme_op_revsig(ctx_, gpgme_key_t(key),
                                               gpgme_key_t(signing_key),
                                               sign_id.second.c_str(), 0));
    if (check_gpg_error_2_err_code(err) != GPG_ERR_NO_ERROR) return false;
  }
  return true;
}

bool GpgFrontend::GpgKeyManager::SetExpire(
    const GpgFrontend::GpgKey& key, std::unique_ptr<GpgSubKey>& subkey,
    std::unique_ptr<boost::posix_time::ptime>& expires) {
  using namespace boost::posix_time;

  unsigned long expires_time = 0;

  if (expires != nullptr) expires_time = to_time_t(ptime(*expires));

  const char* sub_fprs = nullptr;

  if (subkey != nullptr) sub_fprs = subkey->GetFingerprint().c_str();

  auto err = check_gpg_error(
      gpgme_op_setexpire(ctx_, gpgme_key_t(key), expires_time, sub_fprs, 0));

  return check_gpg_error_2_err_code(err) == GPG_ERR_NO_ERROR;
}

bool GpgFrontend::GpgKeyManager::SetOwnerTrustLevel(const GpgKey& key,
                                                    int trust_level) {
  if (trust_level < 0 || trust_level > 5) {
    SPDLOG_ERROR("illegal owner trust level: {}", trust_level);
  }

  AutomatonNextStateHandler next_state_handler =
      [](AutomatonState state, std::string status, std::string args) {
        SPDLOG_DEBUG("next_state_handler state: {}, gpg_status: {}, args: {}",
                     state, status, args);
        std::vector<std::string> tokens;
        boost::split(tokens, args, boost::is_any_of(" "));

        switch (state) {
          case START:
            if (status == "GET_LINE" && args == "keyedit.prompt")
              return COMMAND;
            return ERROR;
          case COMMAND:
            if (status == "GET_LINE" && args == "edit_ownertrust.value") {
              return VALUE;
            }
            return ERROR;
          case VALUE:
            if (status == "GET_LINE" && args == "keyedit.prompt") {
              return QUIT;
            } else if (status == "GET_BOOL" &&
                       args == "edit_ownertrust.set_ultimate.okay") {
              return REALLY_ULTIMATE;
            }
            return ERROR;
          case REALLY_ULTIMATE:
            if (status == "GET_LINE" && args == "keyedit.prompt") {
              return QUIT;
            }
            return ERROR;
          case QUIT:
            if (status == "GET_LINE" && args == "keyedit.save.okay") {
              return SAVE;
            }
            return ERROR;
          case ERROR:
            if (status == "GET_LINE" && args == "keyedit.prompt") {
              return QUIT;
            }
            return ERROR;
          default:
            return ERROR;
        };
      };

  AutomatonActionHandler action_handler =
      [trust_level](AutomatonHandelStruct& handler, AutomatonState state) {
        SPDLOG_DEBUG("action_handler state: {}", state);
        switch (state) {
          case COMMAND:
            return std::string("trust");
          case VALUE:
            handler.SetSuccess(true);
            return std::to_string(trust_level);
          case REALLY_ULTIMATE:
            handler.SetSuccess(true);
            return std::string("Y");
          case QUIT:
            return std::string("quit");
          case SAVE:
            handler.SetSuccess(true);
            return std::string("Y");
          case START:
          case ERROR:
            return std::string("");
          default:
            return std::string("");
        }
        return std::string("");
      };

  auto key_fpr = key.GetFingerprint();
  AutomatonHandelStruct handel_struct(key_fpr);
  handel_struct.SetHandler(next_state_handler, action_handler);

  GpgData data_out;

  auto err = gpgme_op_interact(ctx_, gpgme_key_t(key), 0,
                               GpgKeyManager::interactor_cb_fnc,
                               (void*)&handel_struct, data_out);
  if (err != GPG_ERR_NO_ERROR) {
    SPDLOG_ERROR("fail to set owner trust level {} to key {}, err: {}",
                 trust_level, key.GetId(), gpgme_strerror(err));
  }

  return check_gpg_error_2_err_code(err) == GPG_ERR_NO_ERROR &&
         handel_struct.Success();
}

gpgme_error_t GpgFrontend::GpgKeyManager::interactor_cb_fnc(void* handle,
                                                            const char* status,
                                                            const char* args,
                                                            int fd) {
  auto handle_struct = static_cast<AutomatonHandelStruct*>(handle);
  std::string status_s = status;
  std::string args_s = args;
  SPDLOG_DEBUG("cb start status: {}, args: {}, fd: {}, handle struct state: {}",
               status_s, args_s, fd, handle_struct->CuurentStatus());

  if (status_s == "KEY_CONSIDERED") {
    std::vector<std::string> tokens;
    boost::split(tokens, args, boost::is_any_of(" "));

    if (tokens.empty() || tokens[0] != handle_struct->KeyFpr()) {
      SPDLOG_ERROR("handle struct key fpr {} mismatch token: {}, exit...",
                   handle_struct->KeyFpr(), tokens[0]);
      return -1;
    }

    return 0;
  }

  if (status_s == "GOT_IT" || status_s.empty()) {
    SPDLOG_DEBUG("status GOT_IT, continue...");
    return 0;
  }

  AutomatonState next_state = handle_struct->NextState(status_s, args_s);
  if (next_state == ERROR) {
    SPDLOG_DEBUG("handle struct next state caught error, skipping...");
    return GPG_ERR_FALSE;
  }

  if (next_state == SAVE) {
    handle_struct->SetSuccess(true);
  }

  // set state and preform action
  handle_struct->SetStatus(next_state);
  Command cmd = handle_struct->Action();
  SPDLOG_DEBUG("handle struct action done, next state: {}, action cmd: {}",
               next_state, cmd);
  if (!cmd.empty()) {
    gpgme_io_write(fd, cmd.c_str(), cmd.size());
    gpgme_io_write(fd, "\n", 1);
  } else if (status_s == "GET_LINE") {
    // avoid trapping in this state
    return GPG_ERR_FALSE;
  }

  return 0;
}