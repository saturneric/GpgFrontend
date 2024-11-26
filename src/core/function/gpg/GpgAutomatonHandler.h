

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

#pragma once

#include "core/GpgFrontendCore.h"
#include "core/function/basic/GpgFunctionObject.h"
#include "core/function/gpg/GpgContext.h"
#include "core/typedef/GpgTypedef.h"

namespace GpgFrontend {

class GpgAutomatonHandler
    : public SingletonFunctionObject<GpgAutomatonHandler> {
 public:
  using Command = QString;
  using AutomatonState = enum {
    AS_START,
    AS_SELECT,
    AS_COMMAND,
    AS_VALUE,
    AS_REASON_CODE,
    AS_REASON_TEXT,
    AS_REALLY_ULTIMATE,
    AS_SAVE,
    AS_ERROR,
    AS_QUIT,
  };

  struct AutomatonHandelStruct;

  using AutomatonActionHandler =
      std::function<Command(AutomatonHandelStruct&, AutomatonState)>;
  using AutomatonNextStateHandler =
      std::function<AutomatonState(AutomatonState, QString, QString)>;

  struct AutomatonHandelStruct {
    explicit AutomatonHandelStruct(QString key_fpr);
    void SetStatus(AutomatonState next_state);
    auto CurrentStatus() -> AutomatonState;
    void SetHandler(AutomatonNextStateHandler next_state_handler,
                    AutomatonActionHandler action_handler);
    auto NextState(QString gpg_status, QString args) -> AutomatonState;
    auto Action() -> Command;
    void SetSuccess(bool success);
    [[nodiscard]] auto Success() const -> bool;
    auto KeyFpr() -> QString;

   private:
    AutomatonState current_state_ = AS_START;
    AutomatonNextStateHandler next_state_handler_;
    AutomatonActionHandler action_handler_;
    bool success_ = false;
    QString key_fpr_;
  };

  /**
   * @brief Construct a new Gpg Key Manager object
   *
   * @param channel
   */
  explicit GpgAutomatonHandler(
      int channel = SingletonFunctionObject::GetDefaultChannel());

  auto DoInteract(const GpgKey& key,
                  AutomatonNextStateHandler next_state_handler,
                  AutomatonActionHandler action_handler) -> bool;

 private:
  static auto interator_cb_func(void* handle, const char* status,
                                const char* args, int fd) -> gpgme_error_t;

  GpgContext& ctx_ =
      GpgContext::GetInstance(SingletonFunctionObject::GetChannel());  ///<
};

using AutomatonNextStateHandler =
    GpgAutomatonHandler::AutomatonNextStateHandler;
using AutomatonState = GpgAutomatonHandler::AutomatonState;
using AutomatonActionHandler = GpgAutomatonHandler::AutomatonActionHandler;
using AutomatonHandelStruct = GpgAutomatonHandler::AutomatonHandelStruct;

}  // namespace GpgFrontend