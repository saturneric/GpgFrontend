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

#include <QList>
#include <QString>
#include <QUrl>
#include <functional>

/**
 * @file PlatformInbox.h
 * @brief What other applications shared with this one: documents (a share
 * sheet's files, a file opened with GpgFrontend) and text.
 *
 * The system hands a share to the application as it arrives, whatever the
 * application is doing; it waits here until the application is ready to take
 * it, and each share is handed out once. Documents are content:// URLs the
 * application may read only for a while: take them as copies.
 */

namespace GpgFrontend {

struct SharedItems {
  QList<QUrl> files;
  QString text;
  /// Shared documents that could not be taken: a file:// URL, which an
  /// application without storage access cannot open.
  int refused = 0;

  [[nodiscard]] auto Empty() const -> bool {
    return files.isEmpty() && text.isEmpty() && refused == 0;
  }
};

/// What was shared since the last call; empty where nothing can be.
auto TakeSharedItems() -> SharedItems;

/**
 * @brief Call @p arrived on the application's thread whenever something is
 * shared while it runs, for TakeSharedItems() to take; replaces the one set
 * before. Never called where nothing can be shared.
 */
void SetSharedItemsArrived(std::function<void()> arrived);

}  // namespace GpgFrontend
