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

#include <algorithm>
#include <cstring>

/**
 * @file GFHostVersioned.h
 * @brief Reading a struct a module passed, of whatever SDK generation.
 */

namespace gf_sdk_internal {

/**
 * @brief @p in, read no further than its own `struct_size`.
 *
 * Every struct crossing the ABI leads with its size and grows only by
 * appending, so one from an older SDK is shorter: what it lacks stays zero.
 * Callers check `struct_size >= sizeof(size_t)` first where @p in could be
 * truncated below the field itself.
 */
template <typename T>
auto CopyVersioned(const T& in) -> T {
  T out{};
  std::memcpy(&out, &in, std::min(sizeof(T), in.struct_size));
  return out;
}

}  // namespace gf_sdk_internal
