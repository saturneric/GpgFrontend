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

#include "GpgSmartCardManager.h"

#include "core/function/gpg/GpgAutomatonHandler.h"

namespace GpgFrontend {

GpgSmartCardManager::GpgSmartCardManager(int channel)
    : SingletonFunctionObject<GpgSmartCardManager>(channel) {}

auto GpgSmartCardManager::Fetch(const QString& serial_number) -> bool {
  GpgAutomatonHandler::AutomatonNextStateHandler next_state_handler =
      [=](AutomatonState state, QString status, QString args) {
        auto tokens = args.split(' ');

        switch (state) {
          case GpgAutomatonHandler::AS_START:
            if (status == "CARDCTRL" && args.contains(serial_number)) {
              return GpgAutomatonHandler::AS_START;
            } else if (status == "GET_LINE" && args == "cardedit.prompt") {
              return GpgAutomatonHandler::AS_COMMAND;
            }
            return GpgAutomatonHandler::AS_ERROR;
          case GpgAutomatonHandler::AS_COMMAND:
            if (status == "GET_LINE" && args == "cardedit.prompt") {
              return GpgAutomatonHandler::AS_QUIT;
            }
            return GpgAutomatonHandler::AS_ERROR;
          case GpgAutomatonHandler::AS_QUIT:
          case GpgAutomatonHandler::AS_ERROR:
            if (status == "GET_LINE" && args == "keyedit.prompt") {
              return GpgAutomatonHandler::AS_QUIT;
            }
            return GpgAutomatonHandler::AS_ERROR;
          default:
            return GpgAutomatonHandler::AS_ERROR;
        };
      };

  AutomatonActionHandler action_handler = [](AutomatonHandelStruct& handler,
                                             AutomatonState state) {
    switch (state) {
      case GpgAutomatonHandler::AS_COMMAND:
        return QString("fetch");
      case GpgAutomatonHandler::AS_QUIT:
        return QString("quit");
      case GpgAutomatonHandler::AS_START:
      case GpgAutomatonHandler::AS_ERROR:
        return QString("");
      default:
        return QString("");
    }
    return QString("");
  };

  return GpgAutomatonHandler::GetInstance(GetChannel())
      .DoCardInteract(next_state_handler, action_handler);
}

}  // namespace GpgFrontend