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

#include "GpgAutomatonHandler.h"

#include <utility>

#include "core/model/GpgData.h"
#include "core/model/GpgKey.h"
#include "core/utils/GpgUtils.h"

namespace GpgFrontend {

GpgAutomatonHandler::GpgAutomatonHandler(int channel)
    : SingletonFunctionObject<GpgAutomatonHandler>(channel) {}

auto GpgAutomatonHandler::interator_cb_func(void* handle, const char* status,
                                            const char* args,
                                            int fd) -> gpgme_error_t {
  auto* handle_struct = static_cast<AutomatonHandelStruct*>(handle);
  QString status_s = status;
  QString args_s = args;

  if (status_s == "KEY_CONSIDERED") {
    auto tokens = QString(args).split(' ');

    if (tokens.empty() || tokens[0] != handle_struct->KeyFpr()) {
      LOG_W() << "handle struct key fpr " << handle_struct->KeyFpr()
              << "mismatch token: " << tokens[0] << ", exit...";

      return -1;
    }

    return 0;
  }

  if (status_s == "GOT_IT" || status_s.isEmpty()) {
    FLOG_D("gpg reply is GOT_IT, continue...");
    return 0;
  }

  LOG_D() << "current state" << handle_struct->CurrentStatus()
          << "gpg status: " << status_s << ", args: " << args_s;

  AutomatonState next_state = handle_struct->NextState(status_s, args_s);
  if (next_state == AS_ERROR) {
    FLOG_D("handle struct next state caught error, abort...");
    return -1;
  }
  LOG_D() << "next state" << next_state;

  if (next_state == AS_SAVE) {
    handle_struct->SetSuccess(true);
  }

  // set state and preform action
  handle_struct->SetStatus(next_state);
  Command cmd = handle_struct->Action();

  LOG_D() << "next action, cmd:" << cmd;

  if (!cmd.isEmpty()) {
    auto btye_array = cmd.toUtf8();
    gpgme_io_write(fd, btye_array, btye_array.size());
    gpgme_io_write(fd, "\n", 1);
  } else if (status_s == "GET_LINE") {
    // avoid trapping in this state
    return GPG_ERR_FALSE;
  }

  return 0;
}

auto GpgAutomatonHandler::DoInteract(
    const GpgKey& key, AutomatonNextStateHandler next_state_handler,
    AutomatonActionHandler action_handler, int flags) -> bool {
  gpgme_key_t p_key =
      flags == GPGME_INTERACT_CARD ? nullptr : static_cast<gpgme_key_t>(key);

  AutomatonHandelStruct handel_struct(
      flags == GPGME_INTERACT_CARD ? "" : key.Fingerprint());
  handel_struct.SetHandler(std::move(next_state_handler),
                           std::move(action_handler));

  GpgData data_out;

  auto err = gpgme_op_interact(ctx_.DefaultContext(), p_key, flags,
                               GpgAutomatonHandler::interator_cb_func,
                               static_cast<void*>(&handel_struct), data_out);
  return CheckGpgError(err) == GPG_ERR_NO_ERROR && handel_struct.Success();
}

auto GpgAutomatonHandler::DoCardInteract(
    AutomatonNextStateHandler next_state_handler,
    AutomatonActionHandler action_handler) -> bool {
  return DoInteract({}, std::move(next_state_handler),
                    std::move(action_handler), GPGME_INTERACT_CARD);
}

auto GpgAutomatonHandler::AutomatonHandelStruct::NextState(
    QString gpg_status, QString args) -> AutomatonState {
  return next_state_handler_(current_state_, std::move(gpg_status),
                             std::move(args));
}

void GpgAutomatonHandler::AutomatonHandelStruct::SetHandler(
    AutomatonNextStateHandler next_state_handler,
    AutomatonActionHandler action_handler) {
  next_state_handler_ = std::move(next_state_handler);
  action_handler_ = std::move(action_handler);
}

auto GpgAutomatonHandler::AutomatonHandelStruct::CurrentStatus()
    -> AutomatonState {
  return current_state_;
}

void GpgAutomatonHandler::AutomatonHandelStruct::SetStatus(
    AutomatonState next_state) {
  current_state_ = next_state;
}

auto GpgAutomatonHandler::AutomatonHandelStruct::Action() -> Command {
  return action_handler_(*this, current_state_);
}

void GpgAutomatonHandler::AutomatonHandelStruct::SetSuccess(bool success) {
  success_ = success;
}

auto GpgAutomatonHandler::AutomatonHandelStruct::Success() const -> bool {
  return success_;
}
auto GpgAutomatonHandler::AutomatonHandelStruct::KeyFpr() -> QString {
  return key_fpr_;
}

GpgAutomatonHandler::AutomatonHandelStruct::AutomatonHandelStruct(
    QString key_fpr)
    : key_fpr_(std::move(key_fpr)) {}
}  // namespace GpgFrontend