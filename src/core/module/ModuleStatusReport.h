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

namespace GpgFrontend::Module {

/**
 * @file ModuleStatusReport.h
 * @brief What the module system did at startup, as machine-readable JSON.
 *
 * ## Why this exists
 *
 * A smoke test has to answer one question: did the thing we ship load its
 * modules. The obvious way is to launch the app and grep its log for
 * "loaded 4 module(s), refused 0", and that was the first way, and it was
 * wrong three times over:
 *
 * - it couples a test to a **sentence**, so improving the wording breaks it;
 * - it hardcodes a **count**, so a fifth module breaks every platform at once
 *   and the obvious repair is to bump the number until it passes again;
 * - it has to **find the log**, whose location depends on the flavour -- a
 *   portable build keeps its profile beside the executable and an installed
 *   one does not, which made a healthy build report as broken.
 *
 * A report the application writes on request has none of those problems. The
 * shape is a contract, the counts are data rather than prose, and the caller
 * says where it goes.
 *
 * It is a TESTING aid and deliberately nothing more: no secret, no key
 * material, no profile contents, nothing that is not already in the log.
 */

/// What the module system did, and to which modules.
struct GF_CORE_EXPORT ModuleStatusReport {
  /// Where the Host looked. Useful when the answer is "none": a smoke test
  /// that knows the path it searched can say whether the layout is wrong.
  QString integrated_module_path;

  int discovered = 0;  ///< candidates the scan offered
  int loaded = 0;      ///< taken all the way to registration
  int refused = 0;     ///< offered and refused, for any reason

  /// Identifiers of the modules that registered, sorted.
  QStringList loaded_modules;
};

/**
 * @brief Block until the module system has finished loading, or time out.
 *
 * Module loading runs on its own task runner, so a caller that asks for the
 * report the moment startup returns gets zeros -- which looks exactly like a
 * build whose modules are all broken. This waits for the loader to say it is
 * done rather than for a fixed interval.
 *
 * `ModuleLoadStats::IsFinished()` is the signal, and not
 * `IsAllModulesRegistered()`: the latter is true before the scan has run at
 * all, when nothing is needed and nothing is registered.
 *
 * @param timeout_ms how long to wait before giving up
 * @return whether loading finished within the timeout
 */
auto GF_CORE_EXPORT WaitForModuleLoading(int timeout_ms = 60000) -> bool;

/// Gather what the module system did. Safe to call once loading has settled.
auto GF_CORE_EXPORT CollectModuleStatusReport() -> ModuleStatusReport;

/**
 * @brief Write the report as JSON to @p path.
 *
 * @param path the file to write; its directory must exist
 * @param[out] reason set on failure, to something worth showing a person
 * @return whether it was written
 */
auto GF_CORE_EXPORT WriteModuleStatusReport(const QString& path,
                                            QString& reason) -> bool;

}  // namespace GpgFrontend::Module
