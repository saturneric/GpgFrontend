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

#include <QVariantMap>

#include "core/typedef/GFError.h"

namespace GpgFrontend::Mobile {

/**
 * @brief How a key action that did not work is told: {"error": text,
 * "reason": kind}, the kind one of
 *  - "bad_passphrase": the passphrase was wrong -- ask again;
 *  - "no_passphrase": none was given -- ask again;
 *  - "cancelled": the person stopped it -- nothing to say;
 *  - "invalid": what was entered cannot be used;
 *  - "failed": anything else, in the engine's own words.
 *
 * Only what the engine reported is said: an error it did not attribute to
 * the passphrase is never told as one.
 */
auto DescribeActionError(const GFError& err) -> QVariantMap;

}  // namespace GpgFrontend::Mobile
