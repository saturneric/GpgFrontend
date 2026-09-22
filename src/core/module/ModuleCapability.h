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

#include <cstdint>

#include "GFCoreExport.h"

namespace GpgFrontend::Module {

/**
 * @file ModuleCapability.h
 * @brief What a module's declared capabilities mean, and which of them bite.
 *
 * ## The distinction this header exists to keep
 *
 * A manifest's `capabilities` array has always been one flat list. Two quite
 * different kinds of thing were in it:
 *
 *   - names the Host can actually enforce, because the functionality behind
 *     them is Host-mediated and can be withheld -- `gpg`, `ui`, `storage`;
 *   - names the Host cannot enforce at all, because nothing goes through it --
 *     `network`, where a module links Qt Network and opens a socket.
 *
 * Treating them alike would produce a grant mask that claims more than it can
 * deliver, and a user-facing permission list where "may use the network" and
 * "may decrypt" look equally binding when only one of them is. So they are
 * classified here, once, and surfaced separately: @ref ModuleCapabilityMask
 * returns only what is enforceable, and @ref AdvisoryDeclarationsOf returns
 * the rest.
 *
 * An advisory declaration is not worthless -- it is signed, recorded and
 * shown to the user, which is the whole basis on which a person decides to
 * trust an external module. It is just not a gate.
 *
 * ## No new manifest field
 *
 * The JSON keeps one `capabilities` array and the schema does not move. The
 * classification is the Host's, applied at parse time, which is where it
 * belongs: which names the Host can enforce is a fact about this Host build,
 * not a statement the module gets to make.
 */

/**
 * @brief The enforceable capability groups, as the bits the SDK understands.
 *
 * These values are the GF_HOST_CAP_* macros from src/sdk/GFSDKHostApi.h.
 * They are restated rather than included because gf_core must not depend on
 * the SDK's headers for anything it can spell itself -- and a wrong value here
 * is caught by a static assertion in the one translation unit that sees both.
 */
enum class ModuleCapability : uint32_t {
  kGPG = 1U << 0,
  kPGP = 1U << 1,
  kUI = 1U << 2,
  kEDITOR = 1U << 3,
  kSTORAGE = 1U << 4,
  kPROCESS = 1U << 5,
  /// Module-owned native widgets mounted into Host containers. Declared
  /// explicitly and only together with `ui`; see ModuleManifest.
  kUI_CUSTOM = 1U << 6,
};

/// How a declared name was classified.
enum class ModuleCapabilityKind {
  kENFORCEABLE,  ///< maps to a Host api group that can be withheld
  kADVISORY,     ///< recorded and displayed; nothing routes through the Host
  kUNKNOWN,      ///< not in either vocabulary; a malformed manifest
};

/// Classify one declared name. Case-sensitive: the vocabulary is lower-case.
auto GF_CORE_EXPORT ModuleCapabilityKindOf(const QString& name)
    -> ModuleCapabilityKind;

/**
 * @brief The enforceable bits of @p declared, ignoring advisory names.
 *
 * An unknown name contributes nothing; it is the manifest parser's job to
 * refuse the package outright, and doing it again here would mean a module
 * whose manifest escaped that check silently getting a narrower grant instead
 * of being rejected.
 */
auto GF_CORE_EXPORT ModuleCapabilityMask(const QStringList& declared)
    -> uint32_t;

/// The advisory names in @p declared, in declaration order.
auto GF_CORE_EXPORT AdvisoryDeclarationsOf(const QStringList& declared)
    -> QStringList;

/// Every enforceable name this Host build knows, sorted. For diagnostics,
/// the packager's error messages and the manifest parser.
auto GF_CORE_EXPORT EnforceableCapabilityNames() -> QStringList;

/// Every advisory name this Host build knows, sorted.
auto GF_CORE_EXPORT AdvisoryCapabilityNames() -> QStringList;

/// A mask as a readable, sorted, comma-separated list. "none" when empty.
auto GF_CORE_EXPORT ModuleCapabilityMaskToString(uint32_t mask) -> QString;

}  // namespace GpgFrontend::Module
