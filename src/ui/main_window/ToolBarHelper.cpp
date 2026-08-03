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

#include "ui/main_window/ToolBarHelper.h"

namespace GpgFrontend::UI {

void SetupToolBar(QToolBar* toolbar, Qt::ToolButtonStyle style, QSize size) {
  toolbar->setMovable(true);
  toolbar->setFloatable(false);
  toolbar->setToolButtonStyle(style);
  toolbar->setIconSize(size);
}

void SetupMenuToolButton(QToolButton* button, QMenu* menu, const QIcon& icon,
                         const QString& text, const QString& tooltip,
                         Qt::ToolButtonStyle style, QSize size) {
  button->setMenu(menu);
  button->setPopupMode(QToolButton::InstantPopup);
  button->setIcon(icon);
  button->setText(text);
  button->setToolTip(tooltip);
  button->setFocusPolicy(Qt::NoFocus);
  button->setAutoRaise(false);
  button->setToolButtonStyle(style);
  button->setIconSize(size);
}

}  // namespace GpgFrontend::UI
