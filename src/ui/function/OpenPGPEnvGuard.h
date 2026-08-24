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

#include "core/typedef/GFTypedef.h"

namespace GpgFrontend::UI {

/**
 * @brief What to show the user for a failed OpenPGP environment.
 */
struct GF_UI_EXPORT BadOpenPGPEnvText {
  QString title;
  QString body;
  bool offer_retry = true;  ///< false when retrying cannot change anything
};

/**
 * @brief Turn a startup failure into a title and message that describe it.
 *
 * Every cause used to be titled "No Supported OpenPGP Engine Found", which
 * was true for one of them and misleading for the rest -- a key database that
 * cannot be found is not a missing engine, and telling the user otherwise
 * sends them looking in the wrong place.
 *
 * Pure, so every reason's wording is assertable without starting the engine.
 *
 * @param reason what failed
 * @param detail specifics to append to the body
 */
auto GF_UI_EXPORT DescribeBadOpenPGPEnv(BadOpenPGPEnvReason reason,
                                        const QString& detail)
    -> BadOpenPGPEnvText;

/**
 * @brief Subscribe the modal "your OpenPGP environment is unusable" dialog.
 *
 * Must be installed before the core can report a failure: that path ends in
 * std::exit(0), so a late subscriber never hears it at all. Idempotent.
 */
void GF_UI_EXPORT InstallBadOpenPGPEnvHandler();

/**
 * @brief Whether the user asked to retry a failed environment.
 *
 * Read once during startup to decide whether to go straight back round rather
 * than bring a main window up over a broken environment.
 */
auto GF_UI_EXPORT IsApplicationNeedRestart() -> bool;

}  // namespace GpgFrontend::UI
