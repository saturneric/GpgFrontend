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

auto InteratorCbFunc(void* handle, const char* status, const char* args,
                     int fd) -> gpgme_error_t {
  auto* handle_struct = static_cast<AutomatonHandelStruct*>(handle);
  QString status_s = status;
  QString args_s = args;

  if (status_s == "KEY_CONSIDERED") {
    auto tokens = QString(args).split(' ');

    if (handle_struct->KeyFpr().isEmpty()) return 0;

    if (tokens.empty() || tokens[0] != handle_struct->KeyFpr()) {
      LOG_W() << "handle struct key fpr: " << handle_struct->KeyFpr()
              << "mismatch token: " << tokens[0] << ", exit...";

      return -1;
    }

    return 0;
  }

  if (status_s == "CARDCTRL") {
    auto tokens = QString(args).split(' ');

    if (handle_struct->SerialNumber().isEmpty()) return 0;

    if (tokens.empty() || tokens[0] != handle_struct->SerialNumber()) {
      LOG_W() << "handle struct serial number: "
              << handle_struct->SerialNumber()
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

  handle_struct->SetPromptStatus(status_s, args_s);

  AutomatonState next_state = handle_struct->NextState(status_s, args_s);
  if (next_state == GpgAutomatonHandler::kAS_ERROR) {
    FLOG_D("handle struct next state caught error, abort...");
    return -1;
  }
  LOG_D() << "next state" << next_state;

  if (next_state == GpgAutomatonHandler::kAS_SAVE) {
    handle_struct->SetSuccess(true);
  }

  // set state and preform action
  handle_struct->SetStatus(next_state);
  GpgAutomatonHandler::Command cmd = handle_struct->Action();

  LOG_D() << "next action, cmd:" << cmd;

  if (!cmd.isEmpty()) {
    auto btye_array = cmd.toUtf8();
    gpgme_io_write(fd, btye_array, btye_array.size());
    gpgme_io_write(fd, "\n", 1);
  } else if (status_s.startsWith("GET_")) {
    // avoid trapping in this state
    return GPG_ERR_FALSE;
  }

  return 0;
}

auto DoInteractImpl(GpgContext& ctx_, const GpgKeyPtr& key, bool card_edit,
                    const QString& id,
                    AutomatonNextStateHandler next_state_handler,
                    AutomatonActionHandler action_handler, int flags) -> bool {
  gpgme_key_t p_key = key == nullptr ? nullptr : static_cast<gpgme_key_t>(*key);

  AutomatonHandelStruct handel_struct(card_edit, id);
  handel_struct.SetHandler(std::move(next_state_handler),
                           std::move(action_handler));

  GpgData data_out;

  auto err =
      gpgme_op_interact(ctx_.DefaultContext(), p_key, flags, InteratorCbFunc,
                        static_cast<void*>(&handel_struct), data_out);
  return CheckGpgError(err) == GPG_ERR_NO_ERROR && handel_struct.Success();
}

auto GpgAutomatonHandler::DoInteract(
    const GpgKeyPtr& key, AutomatonNextStateHandler next_state_handler,
    AutomatonActionHandler action_handler, int flags) -> bool {
  assert(key != nullptr);
  if (key == nullptr) return false;
  return DoInteractImpl(ctx_, key, false, key->ID(),
                        std::move(next_state_handler),
                        std::move(action_handler), flags);
}

auto GpgAutomatonHandler::DoCardInteract(
    const QString& serial_number, AutomatonNextStateHandler next_state_handler,
    AutomatonActionHandler action_handler) -> bool {
  return DoInteractImpl(ctx_, nullptr, true, serial_number,
                        std::move(next_state_handler),
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

auto GpgAutomatonHandler::AutomatonHandelStruct::CurrentStatus() const
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
auto GpgAutomatonHandler::AutomatonHandelStruct::KeyFpr() const -> QString {
  return card_edit_ ? "" : id_;
}

GpgAutomatonHandler::AutomatonHandelStruct::AutomatonHandelStruct(
    bool card_edit, QString id)
    : card_edit_(card_edit), id_(std::move(id)) {}

auto GpgAutomatonHandler::AutomatonHandelStruct::PromptStatus() const
    -> std::tuple<QString, QString> {
  return {prompt_status_, prompt_args_};
}

void GpgAutomatonHandler::AutomatonHandelStruct::SetPromptStatus(QString status,
                                                                 QString args) {
  prompt_status_ = std::move(status);
  prompt_args_ = std::move(args);
}

auto GpgAutomatonHandler::AutomatonHandelStruct::SerialNumber() const
    -> QString {
  return card_edit_ ? id_ : "";
}
}  // namespace GpgFrontend