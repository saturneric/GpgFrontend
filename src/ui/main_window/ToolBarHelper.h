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

#include "core/typedef/GpgTypedef.h"

class QAction;
class QToolBar;
class QToolButton;
class QMenu;
class QIcon;

namespace GpgFrontend::UI {

/**
 * @brief Apply the shared tool bar look to @p toolbar.
 *
 * Every main window renders its tool bars the same way; keeping the setup in
 * one place is what stops a window from silently ignoring the user's icon size
 * or button style preference.
 */
void GF_UI_EXPORT SetupToolBar(QToolBar* toolbar, Qt::ToolButtonStyle style,
                               QSize size);

/**
 * @brief Turn @p button into an instant-popup tool button backed by @p menu.
 *
 * One QMenu can back several buttons, so callers are expected to pass a menu
 * that already lives in the menu bar or a context menu rather than building a
 * parallel copy.
 */
void GF_UI_EXPORT SetupMenuToolButton(QToolButton* button, QMenu* menu,
                                      const QIcon& icon, const QString& text,
                                      const QString& tooltip,
                                      Qt::ToolButtonStyle style, QSize size);

/// Operations that never become a button's face, nor its click: symmetric
/// encryption takes a passphrase instead of the checked keys, so it is not a
/// way of doing what an "Encrypt" button promises. It stays in the list.
constexpr int kNeverDefaultOperas = kSYMMETRIC_ENCRYPT;

/**
 * @brief Split the crypto operations ticked in @p mask into tool bar families.
 *
 * The variants of one idea share a button instead of each taking its own:
 * Encrypt (plain, & Sign, Symmetric), Decrypt (plain, & Verify), Sign, Verify.
 * Families come back in that order, each listing its ticked members in that
 * order; empty families and bits that are no tool bar operation are dropped.
 * A ticked member also set in @p defaults moves to the front of its family,
 * which is the one its button runs on a click; kNeverDefaultOperas are
 * ignored there, and come last in their family.
 */
auto GF_UI_EXPORT GroupCryptoOperas(int mask, int defaults = kNONE)
    -> std::vector<std::vector<GpgOperation>>;

/**
 * @brief Which member of a family its button shows, given which are enabled.
 *
 * Only the first @p faces members may be shown, the rest (kNeverDefaultOperas)
 * are reachable from the list alone. Of those, the first (the user's default)
 * while it is enabled, else the first enabled one, else the first again.
 */
auto GF_UI_EXPORT FamilyFaceIndex(const std::vector<bool>& enabled,
                                  std::size_t faces) -> std::size_t;

/**
 * @brief Turn @p button into one tool bar button standing for @p actions.
 *
 * The face shows the action FamilyFaceIndex() picks among the first @p faces,
 * and a click triggers it; holding the button or right-clicking it lists them
 * all. The button is enabled while any action is, and a click on a disabled
 * face opens the list, so an enabled variant is never out of reach: that is
 * why this is not QToolButton::setDefaultAction(). @p actions must not be
 * empty.
 */
void GF_UI_EXPORT SetupActionFamilyToolButton(QToolButton* button,
                                              const QList<QAction*>& actions,
                                              qsizetype faces,
                                              Qt::ToolButtonStyle style,
                                              QSize size);

/**
 * @brief The width a row of buttons with these natural @p widths is evened to.
 *
 * The median, not the maximum: one long label ("Encrypt & Sign") would
 * otherwise pad every short one into a wide gap, and stretch a menu button
 * until its arrow, drawn in the corner, drifts away from the label. 0 for an
 * empty row.
 */
auto GF_UI_EXPORT EvenButtonWidth(std::vector<int> widths) -> int;

/**
 * @brief Bring the buttons on @p toolbar up to one common width.
 *
 * With text under the icon a button is as wide as its label, so a row of
 * labels of different lengths leaves the icons on no rhythm. Buttons narrower
 * than EvenButtonWidth() grow to it; wider ones keep their own width. Call it
 * again after the button style or icon size changes.
 */
void GF_UI_EXPORT EvenToolBarButtonWidths(QToolBar* toolbar);

}  // namespace GpgFrontend::UI
