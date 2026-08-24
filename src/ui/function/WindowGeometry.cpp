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

#include "ui/function/WindowGeometry.h"

namespace GpgFrontend::UI {

auto ClampRectToAvailableGeometry(QRect rect, const QRect& available) -> QRect {
  const int max_width = static_cast<int>(available.width() * 0.95);
  const int max_height = static_cast<int>(available.height() * 0.95);

  if (rect.width() > max_width) {
    rect.setWidth(max_width);
  }

  if (rect.height() > max_height) {
    rect.setHeight(max_height);
  }

  if (rect.left() < available.left()) {
    rect.moveLeft(available.left());
  }

  if (rect.top() < available.top()) {
    rect.moveTop(available.top());
  }

  if (rect.right() > available.right()) {
    rect.moveRight(available.right());
  }

  if (rect.bottom() > available.bottom()) {
    rect.moveBottom(available.bottom());
  }

  return rect;
}

}  // namespace GpgFrontend::UI
