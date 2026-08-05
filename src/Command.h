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

#include <optional>

#include "GpgFrontendContext.h"

namespace GpgFrontend {

// functions

auto PrintVersion() -> int;

/**
 * @brief Read a log level name.
 *
 * Pure, so that the command line can take part in the same layered resolution
 * as every other knob instead of being applied once and then overwritten by
 * whatever the settings happened to say.
 *
 * @param level name: debug, info, warn, error, or none
 * @return the GFLogLevel as an int, or nothing when the name is unusable
 */
auto ParseLogLevelName(const QString& level) -> std::optional<int>;

/**
 * @brief Put a log level into effect, for both loggers.
 *
 * The Qt filter rules, the C++ level and the Rust crate's RUST_LOG all follow
 * from one number, so they are set in one place: they were previously set at
 * two call sites that had drifted, which is why a level coming from the
 * settings never reached the Rust logger at all.
 *
 * @param level the GFLogLevel to apply
 */
void ApplyLogLevel(int level);

auto RunTest(const GFCxtWPtr&) -> int;

auto PrintEnvInfo() -> int;

}  // namespace GpgFrontend