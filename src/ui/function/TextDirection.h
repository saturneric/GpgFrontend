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
 * A per-tab, per-session view choice. Nothing persists it: automatic is what
 * every surface starts as, and the explicit modes exist for the cases the
 * content cannot speak for itself.
 */
enum TextDirectionMode : uint8_t {
  kTEXT_DIRECTION_AUTO = 0,  ///< Every paragraph follows its own content.
  kTEXT_DIRECTION_LTR = 1,   ///< Always left-to-right.
  kTEXT_DIRECTION_RTL = 2,   ///< Always right-to-left.
};

/**
 * @brief The direction @p text reads in, from its first strong character.
 *
 * A narrowed form of the Unicode paragraph direction rules: the first strong
 * character decides, weak and neutral ones are skipped, and text without any
 * strong character at all is left-to-right. Explicit isolate and embedding
 * controls are skipped like any other neutral rather than opening a
 * directional scope.
 *
 * @param text content to classify, may be empty
 * @return Qt::RightToLeft when the first strong character is right-to-left,
 *         otherwise Qt::LeftToRight
 */
auto GF_UI_EXPORT DetectTextDirection(const QString& text)
    -> Qt::LayoutDirection;

/**
 * @brief The direction @p doc as a whole is anchored to, by the same rule.
 *
 * Walks the blocks in order and stops at the first strong character, so an
 * opened file is not copied out in full every time its content changes.
 *
 * This is the document's anchor, not the direction its paragraphs are laid out
 * in. Under kTEXT_DIRECTION_AUTO each paragraph resolves separately; the anchor
 * only decides the things a document has just one of, such as which edge a
 * plain text editor aligns to and which side its scroll bar opens on.
 *
 * @param doc document to classify, may be null
 * @return Qt::RightToLeft when the first strong character is right-to-left,
 *         otherwise Qt::LeftToRight
 */
auto GF_UI_EXPORT DetectTextDirection(const QTextDocument* doc)
    -> Qt::LayoutDirection;

/**
 * @brief Lays @p doc out in @p dir inside @p view, one direction throughout.
 *
 * Sets the direction on both the widget, which decides the side its scroll bar
 * and viewport margins open on, and the document's default text option, which
 * is what re-lays out the blocks that already exist.
 *
 * The option is read back and modified rather than replaced: the line wrap mode
 * and the tab stop distance live in the same QTextOption, and assigning a fresh
 * one would silently drop them.
 *
 * The two document layouts want opposite things from the alignment, and getting
 * it the wrong way round is silent, so see the note in the implementation
 * before touching it.
 *
 * @param view text widget to lay out, may be null
 * @param doc its document, may be null
 * @param dir direction to apply to every paragraph
 */
void GF_UI_EXPORT ApplyTextDirectionToDocument(QWidget* view,
                                               QTextDocument* doc,
                                               Qt::LayoutDirection dir);

/**
 * @brief Lays @p doc out in @p view according to @p mode.
 *
 * The explicit modes pin every paragraph, exactly as the overload above does.
 *
 * kTEXT_DIRECTION_AUTO instead writes Qt::LayoutDirectionAuto, which is not a
 * third direction but the value that hands the decision back to QTextEngine:
 * with it, each paragraph takes the direction of its own first strong
 * character, the way a messenger lays out a conversation. Qt does this during
 * layout, so nothing has to be recomputed when the text changes.
 *
 * What automatic cannot do in a QPlainTextEdit is align each paragraph
 * separately. QPlainTextDocumentLayout hands every block the document's default
 * option and ignores QTextBlockFormat entirely, and its layoutBlock() is
 * private and non-virtual, so there is nowhere to intervene; see
 * GFUiTextDirectionProbeTest, which pins that behaviour. Every line therefore
 * shares one alignment, taken from the document's anchor, and only the ordering
 * within each line is decided per paragraph. A QTextEdit or QTextBrowser has no
 * such limit: QTextDocumentLayout resolves both per block on its own.
 *
 * There is one way to align a plain text editor's lines individually, and it
 * was measured and turned down. QTextEngine::alignLine() reads Qt::AlignJustify
 * as Qt::AlignRight for any line whose own base direction came out
 * right-to-left, which is decided per paragraph, so a single document-wide
 * Qt::AlignJustify does produce per-line alignment. It also justifies every
 * line of a wrapped paragraph except the last, stretching ordinary prose out to
 * the margin, which is too high a price in an editor that wraps by default.
 *
 * @param view text widget to lay out, may be null
 * @param doc its document, may be null
 * @param mode how the direction should be decided
 */
void GF_UI_EXPORT ApplyTextDirectionToDocument(QWidget* view,
                                               QTextDocument* doc,
                                               TextDirectionMode mode);

}  // namespace GpgFrontend::UI
