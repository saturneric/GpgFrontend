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

#include <QTimer>

#include "plat/common/PlatformShare.h"

// Platforms with a share sheet provide their own; everywhere else sharing is
// unsupported, said once, and the interface offers to copy instead.
#if !defined(Q_OS_ANDROID)

namespace GpgFrontend {

namespace {

auto Sheet() -> StubShareSheet& {
  static StubShareSheet sheet;
  return sheet;
}

/// The test's sheet, else what a platform without one says.
void HandOver(const QStringList& paths, const QString& mime_type, bool open,
              ShareCallback done) {
  if (auto sheet = Sheet(); sheet) {
    sheet(paths, mime_type, open, std::move(done));
    return;
  }
  QTimer::singleShot(0, [done = std::move(done)]() {
    if (done) done(ShareResult{ShareOutcome::kUnsupported, false, {}});
  });
}

}  // namespace

void SetStubShareSheet(StubShareSheet sheet) { Sheet() = std::move(sheet); }

auto CanShare() -> bool { return static_cast<bool>(Sheet()); }

void ShareText(const QString&, const QString&, ShareCallback done) {
  QTimer::singleShot(0, [done = std::move(done)]() {
    if (done) done(ShareResult{ShareOutcome::kUnsupported, false, {}});
  });
}

void ShareFile(const QString& path, const QString& mime_type, const QString&,
               ShareCallback done) {
  HandOver({path}, mime_type, false, std::move(done));
}

void ShareFiles(const QStringList& paths, const QString& mime_type,
                const QString&, ShareCallback done) {
  HandOver(paths, mime_type, false, std::move(done));
}

void OpenFile(const QString& path, const QString& mime_type, const QString&,
              ShareCallback done) {
  HandOver({path}, mime_type, true, std::move(done));
}

void RevokeSharedFile(const QString&) {}

}  // namespace GpgFrontend

#endif
