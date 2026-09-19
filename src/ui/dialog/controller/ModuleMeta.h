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

#include <optional>

#include "core/module/ModuleManifest.h"
#include "ui/widgets/MetaListPanel.h"

namespace GpgFrontend::UI {

/**
 * @brief Everything the Controller needs to describe one module.
 *
 * A plain struct rather than a Module pointer, so BuildModuleRows() stays pure
 * and its wording can be asserted by a test that constructs no widget and
 * loads no module. gtest bodies run off the GUI thread, so a test that needed
 * a widget could not exist at all.
 */
struct GF_UI_EXPORT ModuleView {
  QString identifier;  ///< the runtime identifier, always present
  QString version;     ///< what the binary reports
  bool integrated = false;
  bool activated = false;

  /// The `*.gfmodule` this was verified from. Empty for a loose library.
  QString source_package_path;

  /// The signed manifest, when there is one. Its presence is what packaged
  /// means, and what divides fact from claim below.
  std::optional<Module::ModuleManifest> manifest;

  int sdk_abi = 0;  ///< the module's own ABI, not the host's
  QString hash;     ///< digest of the bytes that were loaded
};

/**
 * @brief Describe a module in rows: what was verified, then what is claimed.
 *
 * The order is the whole point, and it is the same argument as
 * BuildProfilePackageRows(): a fact and a claim shown as one list read as two
 * facts. What the application established for itself comes first -- whether
 * this came from a signed package, which package, its ABI, what it was built
 * against. Only then, under a heading that says so, what the module says about
 * itself.
 *
 * For a packaged module the name, description and author are *not* claims:
 * they come from a manifest whose signature the host checked before any of the
 * module's code ran. For a loose library the same three values have nothing
 * behind them, so they are marked unverified and the panel refuses to render
 * them without their caveat.
 *
 * A signature here shows the package was not altered after it was built. It
 * does not show who built it -- the key travels inside the package -- and the
 * rows say so rather than leaving it to be inferred.
 *
 * @param module what is known about the module
 * @return the rows, facts before claims
 */
auto GF_UI_EXPORT BuildModuleRows(const ModuleView& module)
    -> QVector<MetaListRow>;

}  // namespace GpgFrontend::UI
