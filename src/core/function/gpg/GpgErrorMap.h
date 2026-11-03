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

#include <gpgme.h>

#include <tuple>
#include <type_traits>

#include "core/typedef/GFError.h"

namespace GpgFrontend {

/**
 * @brief A GnuPG error in GpgFrontend's terms.
 *
 * The whole value, source included, is kept as the native value. A code with
 * no counterpart is a general failure.
 *
 * @param err what GnuPG returned
 * @return the error
 */
auto GF_CORE_EXPORT FromGpgError(gpgme_error_t err) -> GFError;

/**
 * @brief The GnuPG error a GFError stands for.
 *
 * For code that still speaks GnuPG's codes, and for the values the module SDK
 * has always handed out. An error GnuPG reported comes back exactly as it
 * reported it; any other is the bare code, with no source.
 *
 * @param err the error
 * @return the GnuPG error
 */
auto GF_CORE_EXPORT ToGpgError(const GFError& err) -> gpgme_error_t;

/**
 * @brief A GnuPG implementation registered in an engine dispatch table whose
 * entries report GFError.
 *
 * The GnuPG backend answers in gpgme's codes; this is where they leave it.
 * Call() takes exactly the implementation's parameters, so its address has
 * the table's function type.
 */
template <auto Fn>
struct FromGnuPGImpl;

template <typename R, typename... Args, R (*Fn)(Args...)>
struct FromGnuPGImpl<Fn> {
  static auto Call(Args... args) {
    if constexpr (std::is_same_v<R, gpgme_error_t>) {
      return FromGpgError(Fn(std::forward<Args>(args)...));
    } else {
      auto [err, value] = Fn(std::forward<Args>(args)...);
      return std::tuple<GFError, decltype(value)>{FromGpgError(err),
                                                  std::move(value)};
    }
  }
};

}  // namespace GpgFrontend
