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

#include "platform/common/PlatformEnv.h"

#ifdef APPLE_SANDBOX

#include <CoreFoundation/CoreFoundation.h>
#include <Security/Security.h>

#include "core/GFCoreLog.h"

namespace GpgFrontend::Platform {

auto IsRunningInAppSandbox() -> bool {
  SecTaskRef task = SecTaskCreateFromSelf(kCFAllocatorDefault);
  if (task == nullptr) {
    LOG_W() << "SecTaskCreateFromSelf failed, cannot determine if running in "
               "app sandbox. Assuming not running in sandbox.";
    return false;
  }

  CFTypeRef value = SecTaskCopyValueForEntitlement(
      task, CFSTR("com.apple.security.app-sandbox"), nullptr);

  CFRelease(task);

  if (value == nullptr) {
    LOG_W() << "SecTaskCopyValueForEntitlement failed, cannot determine if "
               "running in app sandbox. Assuming not running in sandbox.";
    return false;
  }

  bool result = false;

  if (CFGetTypeID(value) == CFBooleanGetTypeID()) {
    result = CFBooleanGetValue(static_cast<CFBooleanRef>(value));
  }

  LOG_D() << "Running in app sandbox: " << result;

  CFRelease(value);
  return result;
}

}  // namespace GpgFrontend::Platform

#endif  // APPLE_SANDBOX
