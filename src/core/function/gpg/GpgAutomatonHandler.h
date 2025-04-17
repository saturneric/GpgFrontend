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

#include <utility>

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
    kAS_START,
    kAS_SELECT,
    kAS_COMMAND,
    kAS_VALUE,
    kAS_ADMIN,
    kAS_REASON_CODE,
    kAS_REASON_TEXT,
    kAS_REALLY_ULTIMATE,
    kAS_SAVE,
    kAS_INFO,
    kAS_ERROR,
    kAS_QUIT,
  };

  struct AutomatonHandelStruct;

  using AutomatonActionHandler =
      std::function<Command(AutomatonHandelStruct&, AutomatonState)>;
  using AutomatonNextStateHandler =
      std::function<AutomatonState(AutomatonState, QString, QString)>;

  struct AutomatonHandelStruct {
    explicit AutomatonHandelStruct(bool card_edit, QString id);

    auto NextState(QString gpg_status, QString args) -> AutomatonState;
    auto Action() -> Command;

    void SetStatus(AutomatonState next_state);
    void SetSuccess(bool success);
    void SetPromptStatus(QString status, QString args);
    void SetHandler(AutomatonNextStateHandler next_state_handler,
                    AutomatonActionHandler action_handler);

    [[nodiscard]] auto CurrentStatus() const -> AutomatonState;
    [[nodiscard]] auto Success() const -> bool;
    [[nodiscard]] auto KeyFpr() const -> QString;
    [[nodiscard]] auto SerialNumber() const -> QString;
    [[nodiscard]] auto PromptStatus() const -> std::tuple<QString, QString>;

   private:
    AutomatonState current_state_ = kAS_START;
    AutomatonNextStateHandler next_state_handler_;
    AutomatonActionHandler action_handler_;
    bool success_ = false;
    bool card_edit_;
    QString id_;
    QString prompt_status_;
    QString prompt_args_;
  };

  /**
   * @brief Construct a new Gpg Key Manager object
   *
   * @param channel
   */
  explicit GpgAutomatonHandler(
      int channel = SingletonFunctionObject::GetDefaultChannel());

  /**
   * @brief
   *
   * @param key
   * @param next_state_handler
   * @param action_handler
   * @param flags
   * @return true
   * @return false
   */
  auto DoInteract(const GpgKeyPtr& key,
                  AutomatonNextStateHandler next_state_handler,
                  AutomatonActionHandler action_handler, int flags = 0) -> bool;

  /**
   * @brief
   *
   * @param next_state_handler
   * @param action_handler
   * @return true
   * @return false
   */
  auto DoCardInteract(const QString& serial_number,
                      AutomatonNextStateHandler next_state_handler,
                      AutomatonActionHandler action_handler) -> bool;

 private:
  GpgContext& ctx_ =
      GpgContext::GetInstance(SingletonFunctionObject::GetChannel());  ///<
};

using AutomatonNextStateHandler =
    GpgAutomatonHandler::AutomatonNextStateHandler;
using AutomatonState = GpgAutomatonHandler::AutomatonState;
using AutomatonActionHandler = GpgAutomatonHandler::AutomatonActionHandler;
using AutomatonHandelStruct = GpgAutomatonHandler::AutomatonHandelStruct;

}  // namespace GpgFrontend