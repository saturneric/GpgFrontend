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

#include "ui/function/SecurityDisplayNames.h"

namespace GpgFrontend::UI {

auto SecureLevelDisplayName(int level) -> QString {
  // Brief, plainly escalating names for the status readout. What each level
  // actually does is spelled out on the Advanced settings page, where the
  // level is chosen; here a one-word tier reads at a glance.
  switch (level) {
    case 0:
      return QObject::tr("Standard");
    case 1:
      return QObject::tr("Enhanced");
    case 2:
      return QObject::tr("Strong");
    case 3:
      return QObject::tr("Maximum");
    default:
      return QObject::tr("Unknown");
  }
}

auto AppKeyProtectionDisplayName(AppKeyProtection protection) -> QString {
  switch (protection) {
    case AppKeyProtection::kKEYCHAIN:
      return QObject::tr("System keychain");
    case AppKeyProtection::kPIN:
      return QObject::tr("PIN at startup");
    case AppKeyProtection::kNONE:
      break;
  }
  return QObject::tr("No extra protection");
}

}  // namespace GpgFrontend::UI
