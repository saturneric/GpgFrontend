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

namespace GpgFrontend::UI {

/**
 * @brief How a text surface decides which way its paragraphs run.
 *
 * Stored as an int in AppearanceSO, so the values are part of the settings
 * format and must not be renumbered.
 */
enum TextDirectionMode : uint8_t {
  kTEXT_DIRECTION_AUTO = 0,  ///< Follow the content.
  kTEXT_DIRECTION_LTR = 1,   ///< Always left-to-right.
  kTEXT_DIRECTION_RTL = 2,   ///< Always right-to-left.
};

/**
 * @brief The direction @p text reads in, from its first strong character.
 *
 * A narrowed form of the Unicode paragraph direction rules: the first strong
 * character decides, weak and neutral ones are skipped, and text without any
 * strong character at all is left-to-right. Two simplifications are deliberate.
 * Explicit isolate and embedding controls are skipped like any other neutral
 * rather than opening a directional scope, and the whole content is treated as
 * one paragraph, because the surfaces this serves carry a single message in a
 * single script and take one base direction for the entire document.
 *
 * @param text content to classify, may be empty
 * @return Qt::RightToLeft when the first strong character is right-to-left,
 *         otherwise Qt::LeftToRight
 */
auto GF_UI_EXPORT DetectTextDirection(const QString& text)
    -> Qt::LayoutDirection;

/**
 * @brief The direction @p doc reads in, by the same rule.
 *
 * Walks the blocks in order and stops at the first strong character, so an
 * opened file is not copied out in full every time its content changes.
 *
 * @param doc document to classify, may be null
 * @return Qt::RightToLeft when the first strong character is right-to-left,
 *         otherwise Qt::LeftToRight
 */
auto GF_UI_EXPORT DetectTextDirection(const QTextDocument* doc)
    -> Qt::LayoutDirection;

/**
 * @brief The direction a surface showing @p text should lay out in.
 *
 * @param mode configured mode; the explicit ones ignore @p text entirely
 * @param text content to classify when @p mode is kTEXT_DIRECTION_AUTO
 * @return the direction to apply
 */
auto GF_UI_EXPORT ResolveTextDirection(TextDirectionMode mode,
                                       const QString& text)
    -> Qt::LayoutDirection;

/**
 * @brief The direction a surface showing @p doc should lay out in.
 *
 * @param mode configured mode; the explicit ones ignore @p doc entirely
 * @param doc content to classify when @p mode is kTEXT_DIRECTION_AUTO
 * @return the direction to apply
 */
auto GF_UI_EXPORT ResolveTextDirection(TextDirectionMode mode,
                                       const QTextDocument* doc)
    -> Qt::LayoutDirection;

/**
 * @brief A stored setting value as a mode.
 *
 * The settings file is user editable, so anything unrecognised is read as
 * kTEXT_DIRECTION_AUTO rather than cast blindly.
 *
 * @param value stored integer
 * @return the matching mode, or kTEXT_DIRECTION_AUTO
 */
auto GF_UI_EXPORT TextDirectionModeFromInt(int value) -> TextDirectionMode;

/**
 * @brief Lays @p doc out in @p dir inside @p view.
 *
 * Sets the direction on both the widget, which decides the side its scroll bar
 * and viewport margins open on, and the document's default text option, which
 * is what re-lays out the blocks that already exist.
 *
 * The option is read back and modified rather than replaced: the line wrap mode
 * and the tab stop distance live in the same QTextOption, and assigning a fresh
 * one would silently drop them. Paragraph alignment is deliberately left alone.
 * Qt resolves the default leading alignment against the base direction, so
 * right-to-left text right-aligns on its own; asking for Qt::AlignRight instead
 * is mapped back to the left edge and gets this exactly backwards.
 *
 * @param view text widget to lay out
 * @param doc its document
 * @param dir direction to apply
 */
void GF_UI_EXPORT ApplyTextDirectionToDocument(QWidget* view,
                                               QTextDocument* doc,
                                               Qt::LayoutDirection dir);

}  // namespace GpgFrontend::UI
