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

#include <QLoggingCategory>

// GF_CORE_EXPORT and QString both arrive through the core precompiled header,
// as they do for every other header in this directory.

namespace GpgFrontend::Module {

/**
 * @brief The Qt logging category name a module's output is emitted under.
 *
 * ```
 * com.bktus.gpgfrontend.module.key_server_sync -> "module.key-server-sync"
 * ```
 *
 * `module.` plus @ref ModuleIdLeaf, so the category a reader filters on is the
 * same string as the namespace directory they are already looking at:
 *
 * ```
 * build/artifacts/modules/key-server-sync-b59012630cec/   <- namespace
 *                         module.key-server-sync          <- log category
 * ```
 *
 * An empty id yields `"module.unknown"` rather than a leaf of `"m"`: a message
 * that could not be attributed should say so, not claim a one-letter module.
 *
 * @param module_id the module's full, canonical identity
 * @return a dotted Qt category name
 */
auto GF_CORE_EXPORT ModuleLogCategoryName(const QString& module_id) -> QString;

/**
 * @brief The logging category for one module, created on first use.
 *
 * ## What this is, and what it is not
 *
 * It is a **diagnostic and filtering handle**, not an identity and not a trust
 * boundary. Two modules whose ids end in the same component share one category,
 * because @ref ModuleIdLeaf is deliberately not injective. Nothing decides
 * anything on the strength of a category name; the signed `manifest.id` is the
 * identity and @ref ModuleDirectoryKey is the unique locator.
 *
 * ## Lifetime
 *
 * Categories are created on first use and **never destroyed**. That is what
 * makes them safe rather than a leak: `QLoggingCategory` does not copy the
 * `const char*` name it is given, so the name has to outlive the category, and
 * the category has to outlive every log statement that could name it -- which
 * is to say, the process. The count is bounded by the number of modules ever
 * loaded.
 *
 * Qt applies the installed filter rules to a category when it is registered, so
 * one created the first time a module logs still obeys the level resolved at
 * startup.
 *
 * @param module_id the module's full, canonical identity
 * @return a reference valid for the remaining life of the process
 */
auto GF_CORE_EXPORT ModuleLogCategory(const QString& module_id)
    -> const QLoggingCategory&;

/**
 * @brief The child category carrying one module's trace-level output.
 *
 * `module.<leaf>.trace`, a child of @ref ModuleLogCategory so that Qt's own
 * rule matching can silence it independently. Trace is the level a module uses
 * for per-item chatter, and a reader who asked for `debug` has not asked to be
 * buried in it -- so `BuildQtLoggingFilterRules` disables every `*.trace`
 * category at every level except `trace` itself.
 *
 * @param module_id the module's full, canonical identity
 * @return a reference valid for the remaining life of the process
 */
auto GF_CORE_EXPORT ModuleTraceLogCategory(const QString& module_id)
    -> const QLoggingCategory&;

}  // namespace GpgFrontend::Module
