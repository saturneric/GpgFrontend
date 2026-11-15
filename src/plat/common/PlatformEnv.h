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

/// Questions about the environment this process runs in that only the
/// operating system can answer. Built into gf_core.

namespace GpgFrontend {
enum class AppKeyProtection : std::uint8_t;
}  // namespace GpgFrontend

namespace GpgFrontend::Platform {

/**
 * @brief Whether this process carries the macOS App Sandbox entitlement.
 *
 * Asked only in a sandbox-enabled macOS build; every other build answers
 * false without asking.
 */
auto GF_CORE_EXPORT IsRunningInAppSandbox() -> bool;

/**
 * @brief How the application key is protected when nothing says otherwise.
 *
 * None on the desktop, where the credential store is an opt-in the user makes
 * in the settings. The device keystore on Android, which has no such setting:
 * every device has a hardware-backed keystore, and a key file left in plain
 * text there protects nothing the keystore would not.
 *
 * Also decides whether a protection that could not be applied is written back
 * as "none": where the default is a protection, there is no setting to turn it
 * on again, so it is retried on the next start instead.
 */
auto GF_CORE_EXPORT DefaultAppKeyProtection() -> AppKeyProtection;

}  // namespace GpgFrontend::Platform
