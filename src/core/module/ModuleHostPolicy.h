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

#include "GFCoreExport.h"

namespace GpgFrontend::Module {

/**
 * @file ModuleHostPolicy.h
 * @brief Who a module came from, and what this Host build demands of it.
 *
 * Two questions that used to have one answer. A module shipped inside the
 * Host distribution and a module dropped into the user's profile were verified
 * identically, because there had only ever been one kind: the descriptor
 * signing key is ephemeral and per build, so nothing anyone else produced
 * could be accepted at all.
 *
 * That is an accident of the key, not a decision. This header makes the
 * distinction explicit so the two can diverge on purpose.
 *
 * ## Origin is not a claim a module can make
 *
 * It comes from the discovery source -- which directory the Host scanned --
 * and from nowhere else. There is deliberately no `integrated` field in the
 * manifest, and there will not be one: it would be a signed statement by
 * which a module selects its own trust level.
 */

/// Where a module was found, and therefore which trust boundary it crosses.
enum class ModuleOrigin {
  kINTEGRATED,  ///< the Host-owned namespace; part of this distribution
  kEXTERNAL,    ///< a user or system directory; third-party code
};

/**
 * @brief Whether a descriptor is obliged to bind the bytes of its entry.
 *
 * kNOT_REQUIRED: a binding may be absent -- but if one is present it MUST
 *                still be computed and compared.
 * kREQUIRED:     a binding must be present, and must verify.
 *
 * Spelled kNOT_REQUIRED rather than kOPTIONAL deliberately. "Optional" invites
 * the reading "ignore it", and ignoring a binding that is actually there is
 * the one thing this enum must never authorise.
 */
enum class ModuleBindingRequirement {
  kNOT_REQUIRED,
  kREQUIRED,
};

/**
 * @brief What a reader demands of one descriptor's entry binding.
 *
 * Both members default to the strict answer on purpose: a default-constructed
 * policy is the strongest one, so a call site that forgets to fill it in is
 * refused rather than admitted.
 */
struct GF_CORE_EXPORT ModuleEntryTrustPolicy {
  ModuleOrigin origin = ModuleOrigin::kEXTERNAL;
  ModuleBindingRequirement integrated_binding =
      ModuleBindingRequirement::kREQUIRED;

  /// External modules ALWAYS require a binding. There is no knob for it, and
  /// no fallback. A member rather than an `if` repeated at each call site,
  /// so the rule is a property of the type instead of something three callers
  /// have to remember.
  [[nodiscard]] auto BindingRequired() const -> bool {
    return origin == ModuleOrigin::kEXTERNAL ||
           integrated_binding == ModuleBindingRequirement::kREQUIRED;
  }
};

/**
 * @brief What this Host build demands of its own integrated modules.
 *
 * Compiled in, not configured and not looked up, exactly like the trust root
 * beside it: a build either requires its integrated modules to be bound or it
 * does not, and that is not a runtime question.
 */
auto GF_CORE_EXPORT HostIntegratedBindingRequirement()
    -> ModuleBindingRequirement;

/// "integrated" or "external", for logs and reports.
auto GF_CORE_EXPORT ModuleOriginToString(ModuleOrigin origin) -> const char*;

}  // namespace GpgFrontend::Module
