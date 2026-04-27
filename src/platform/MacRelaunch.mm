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

#include "GpgFrontend.h"

#ifdef Q_OS_MACOS

#include <QStringList>
#include <QDebug>

#import <AppKit/AppKit.h>

namespace GpgFrontend {

// NOLINTNEXTLINE
bool RelaunchApplication(const QStringList& arguments) {
  @autoreleasepool {
    NSURL* bundle_url = [[NSBundle mainBundle] bundleURL];
    if (bundle_url == nil) {
      qWarning() << "failed to get main bundle url";
      return false;
    }

    NSWorkspaceOpenConfiguration* config =
        [NSWorkspaceOpenConfiguration configuration];

    config.activates = YES;

    if ([config respondsToSelector:@selector(setCreatesNewApplicationInstance:)]) {
      config.createsNewApplicationInstance = YES;
    }

    NSMutableArray<NSString*>* ns_args = [NSMutableArray array];

    for (const auto& arg : arguments) {
      [ns_args addObject:arg.toNSString()];
    }

    config.arguments = ns_args;

    [[NSWorkspace sharedWorkspace]
        openApplicationAtURL:bundle_url
               configuration:config
           completionHandler:^(NSRunningApplication* app, NSError* error) {
             if (error != nil) {
               qWarning() << "failed to relaunch application:"
                          << QString::fromNSString([error localizedDescription]);
             }
           }];

    return true;
  }
}

} // namespace GpgFrontend

#endif