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

#include "core/module/ModuleManifest.h"

namespace GpgFrontend::Module {

/**
 * @file ModulePreparedEntry.h
 * @brief The seal a platform preparation step leaves behind for one module.
 *
 * ## Why this exists at all
 *
 * Between linking a module's entry native and finalizing its descriptor, the
 * file is rewritten by tools that live outside the build graph: `patchelf` and
 * `linuxdeployqt` on Linux, `install_name_tool` on macOS, deployment scripts
 * everywhere. CMake cannot see those edits, so "the descriptor is newer than
 * the library" proves nothing about whether it describes it.
 *
 * So correctness here is content-based rather than build-graph-based. The
 * pipeline is:
 *
 * ```
 * build -> platform preparation -> seal-prepared -> finalize descriptors
 *                                      |                   |
 *                                      |                   uses
 * --prepared-manifest writes native/prepared.json
 * ```
 *
 * `seal-prepared` records what the entry native binds to *now*, after
 * preparation. `pack --prepared-manifest` recomputes it and refuses to write a
 * descriptor if the two disagree -- which is exactly the case where something
 * touched the file between the two steps. A release that skips the seal loses
 * the check and nothing else; a release that runs it cannot ship a descriptor
 * that describes a file as it used to be.
 *
 * It is **not** a trust mechanism. `prepared.json` is unsigned, lives in the
 * build tree, and is never shipped. Anyone who can rewrite the native can
 * rewrite the seal beside it. What it catches is a pipeline that reordered
 * itself, which is the failure that actually happens.
 */

/// What a preparation step sealed about one module's entry native.
struct GF_CORE_EXPORT PreparedEntrySeal {
  QString module_id;
  QString build_id;
  QString entry_native_name;
  ModuleEntryVerificationMode mode = ModuleEntryVerificationMode::kFILE_SHA256;
  QString value;     ///< 64 lower-case hex, meaning fixed by @ref mode
  qint64 size = -1;  ///< only under kFILE_SHA256; negative means absent
};

/// The filename a seal takes inside a module's `native/` directory.
constexpr auto kPreparedEntrySealFileName = "prepared.json";

/// The only schema this build writes, and the only one it reads.
///
/// Deliberately not forward-compatible: a seal is consumed minutes after it
/// is written, by the same build, so a version skew here means the tree is
/// inconsistent and continuing would be worse than stopping.
constexpr auto kPreparedEntrySealSchema = 1;

/**
 * @brief Write a seal, replacing any that was there.
 *
 * @param path where to write, usually `<namespace>/native/prepared.json`
 * @param seal what preparation produced
 * @param[out] reason set on failure, to something worth showing a person
 * @return whether it was written
 */
auto GF_CORE_EXPORT WritePreparedEntrySeal(const QString& path,
                                           const PreparedEntrySeal& seal,
                                           QString& reason) -> bool;

/**
 * @brief Read a seal back, refusing anything it does not fully understand.
 *
 * Every field is required and checked: a missing one, an unknown mode, a value
 * that is not a 256-bit hex digest, or a `size` under a mode that may not
 * carry one is a refusal rather than a defaulted field. A seal that is half
 * understood is worse than no seal, because the half that was understood would
 * look like a passing check.
 *
 * @param path the file to read
 * @param[out] seal filled in only on success
 * @param[out] reason set on failure
 * @return whether it was read
 */
auto GF_CORE_EXPORT ReadPreparedEntrySeal(const QString& path,
                                          PreparedEntrySeal& seal,
                                          QString& reason) -> bool;

}  // namespace GpgFrontend::Module
