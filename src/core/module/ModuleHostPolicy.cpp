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

#include "core/module/ModuleHostPolicy.h"

namespace GpgFrontend::Module {

// Defined by ModuleHostPolicyGenerated.cpp, which CMake writes into the build
// tree from GPGFRONTEND_INTEGRATED_MODULE_NATIVE_BINDING. Declared here by
// hand, for the same reason the trust root beside it is: one definition and
// one accessor, with no generated header anything could go looking through.
//
// A generated source rather than a compile definition, and the difference
// matters: gf_module_packager links gf_core, so a PRIVATE definition would
// let the tool and the library hold different answers while both compiled
// cleanly -- and the tool's whole purpose is to agree with the Host.
namespace generated {
extern const int kIntegratedModuleBindingRequired;
}  // namespace generated

auto HostIntegratedBindingRequirement() -> ModuleBindingRequirement {
  return generated::kIntegratedModuleBindingRequired != 0
             ? ModuleBindingRequirement::kREQUIRED
             : ModuleBindingRequirement::kNOT_REQUIRED;
}

auto ModuleOriginToString(ModuleOrigin origin) -> const char* {
  switch (origin) {
    case ModuleOrigin::kINTEGRATED:
      return "integrated";
    case ModuleOrigin::kEXTERNAL:
      return "external";
  }
  return "external";
}

}  // namespace GpgFrontend::Module
