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

// GF_CORE_EXPORT and QString both arrive through the core precompiled header,
// as they do for every other header in this directory.

namespace GpgFrontend::Module {

/**
 * @brief The directory a module owns, derived from the identity it signs.
 *
 * A module id is authoritative but is a poor filesystem name:
 * `com.bktus.gpgfrontend.module.email` is thirty-five characters, and its dots
 * are meaningful to macOS tooling, which reads a trailing dotted component as a
 * bundle extension. This maps one to a single bounded path component.
 *
 * ```
 * com.bktus.gpgfrontend.module.email -> email-a1b2c3d4e5f60718293a
 * ```
 *
 * ## It is a locator, never an identity
 *
 * Nothing trusts this value. The signed manifest's `id` is the identity; this
 * is recomputed from it and compared against the directory a descriptor was
 * found in, so a mismatch is a refusal. It is deliberately **not** a manifest
 * field: a second, signable statement about where a module lives would be a
 * second truth to keep in step with the first.
 *
 * ## The shape
 *
 * ```
 * <leaf>-<suffix>
 *
 * leaf    the final dotted component, lowercased, '_' -> '-',
 *         restricted to [a-z0-9-], stripped of leading and trailing '-',
 *         prefixed with 'm' if it would not start with a letter,
 *         truncated to 24 characters
 *
 * suffix  SHA-256 of the full module id, first 80 bits, lower-case hex,
 *         exactly 20 characters
 * ```
 *
 * The leaf is there to be read by a human looking at a directory listing; the
 * suffix is what makes the mapping injective in practice. 80 bits is chosen
 * over something shorter because this has to stay unique across independently
 * authored modules, where a 32-bit suffix collides at a few tens of thousands.
 *
 * ## Why hex and not base32
 *
 * Base32 would spend 16 characters where hex spends 20. It was rejected
 * anyway, because this function has to exist **twice** -- here, and in CMake,
 * which places the build output -- and CMake has `string(SHA256)` and
 * substrings but no base32. Hand-rolling a second base32 in CMake to save four
 * characters would buy a way for the two to silently disagree. Hex is what
 * both languages already have.
 *
 * @param module_id the module's full, canonical identity
 * @return a single path component, at most 45 characters, matching
 * `^[a-z][a-z0-9-]*-[0-9a-f]{20}$`; empty only if @p module_id is empty
 */
auto GF_CORE_EXPORT ModuleDirectoryKey(const QString& module_id) -> QString;

/// The subdirectory of a module namespace holding its native files, on every
/// layout but the macOS bundle.
constexpr auto kModuleNativeDirName = "native";

/// The bundle subtree macOS module natives live in, relative to `Contents`.
///
/// Apple executable code belongs under `Frameworks`, never under `Resources`:
/// `Resources` is for data, and code placed there is neither signed nor
/// validated the way the platform expects.
constexpr auto kAppleModuleNativeRoot = "Frameworks/GpgFrontendModules";

/// The namespace root inside a macOS bundle, relative to `Contents`.
constexpr auto kAppleModuleDescriptorRoot = "Resources/modules";

/**
 * @brief Where a namespace's native files live, given its descriptor.
 *
 * The ONE place this is known. Callers never build these paths, which is the
 * point: on every layout but one the descriptor and the natives are two levels
 * of a single tree, and on macOS they are genuinely different trees, because
 * Apple wants code under `Frameworks` and data under `Resources`.
 *
 * ```
 * everywhere   <root>/<key>/module.gfmodule
 *              <root>/<key>/native/...
 *
 * macOS bundle Contents/Resources/modules/<key>/module.gfmodule
 *              Contents/Frameworks/GpgFrontendModules/<key>/...
 * ```
 *
 * Decided from the descriptor's own path rather than from an `#ifdef`, so a
 * macOS *development* build -- which uses the unified layout, as the existing
 * `GF_BUILD_DEBUG` guard already arranges -- takes the unified answer without
 * a second code path, and so this is testable on any host.
 *
 * @param descriptor_path an existing descriptor's path
 * @return the directory its entry native must be a direct child of
 */
auto GF_CORE_EXPORT ModuleNativeRootFor(const QString& descriptor_path)
    -> QString;

}  // namespace GpgFrontend::Module
