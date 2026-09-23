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

#include <QString>
#include <optional>

#include "GFSDKTypes.h"
#include "core/model/GFBuffer.h"

/**
 * @file GFHostTransfer.h
 * @brief Moving buffer handles between modules without copying them.
 *
 * A buffer handle belongs to one module: presented by another, it is refused.
 * When a command's payload goes from module A to module B, the bytes must
 * change owner, and copying them would leave a second plaintext behind. So
 * the handle is TAKEN from A -- which only A can do -- and a new one wrapping
 * the SAME storage is issued to B. GFBuffer shares its storage, so nothing is
 * duplicated and the one copy is still wiped when the last owner lets go.
 */
namespace gf_sdk_internal {

/// Take @p buf from the calling module. Refused (nullopt) for a handle that
/// is stale or belongs to another module; the handle is gone on success.
auto TakeBufferForTransfer(GFBufferRef buf, const char* what)
    -> std::optional<GpgFrontend::GFBuffer>;

/// A new handle owned by @p module_id, sharing @p buffer's storage.
auto NewBufferFor(GpgFrontend::GFBuffer buffer, const QString& module_id,
                  const char* origin) -> GFBufferRef;

/// The bytes behind a handle the calling module may read, shared.
auto ReadBuffer(GFBufferView buf, const char* what)
    -> std::optional<GpgFrontend::GFBuffer>;

}  // namespace gf_sdk_internal
