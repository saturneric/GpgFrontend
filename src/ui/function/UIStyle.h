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
 * @brief Palette-derived accent colour for status chips.
 *
 * Derived from the palette instead of hard coded so chips stay legible under
 * both light and dark themes without a stylesheet. A negative state is not
 * painted red, it is simply de-emphasised text.
 *
 * @param palette the palette of the widget the chip belongs to
 * @param positive whether the chip reports a good state
 * @return the colour to paint the chip text with
 */
auto GF_UI_EXPORT AccentColor(const QPalette& palette, bool positive) -> QColor;

/**
 * @brief The secondary text colour: a caption beside a value, a sentence under
 * one.
 *
 * Not QPalette::Disabled: that role says "you cannot use this" and is faint
 * enough to be hard to read at small sizes. This is the ordinary text colour
 * moved towards the background, which reads as secondary without reading as
 * switched off. Mixed rather than made translucent because a label drawing
 * through rich text drops an alpha channel.
 *
 * @param palette the palette of the widget the text belongs to
 * @return an opaque colour a shade quieter than the text beside it
 */
auto GF_UI_EXPORT MutedTextColor(const QPalette& palette) -> QColor;

/**
 * @brief Hairline colour for a card border.
 *
 * Far enough towards the background to draw a boundary without drawing a box.
 *
 * @param palette the palette of the widget being outlined
 * @return an opaque colour just off the background
 */
auto GF_UI_EXPORT BorderColor(const QPalette& palette) -> QColor;

/**
 * @brief Palette-derived colour for a state that is a fallback, not the intent.
 *
 * Amber rather than red for the same reason AccentColor() paints no negative
 * state red: nothing here is broken, something merely settled for less than it
 * asked for.
 *
 * @param palette the palette of the widget the text belongs to
 * @return a colour that stays legible under both light and dark themes
 */
auto GF_UI_EXPORT WarningColor(const QPalette& palette) -> QColor;

/**
 * @brief Palette-derived colour for something that cannot be undone.
 *
 * The narrow exception to the rule the colours above follow: AccentColor()
 * paints no negative state red and WarningColor() is amber because nothing
 * there is broken, something merely settled for less than it asked for. This
 * one is for the other case -- key material about to become permanently
 * unreadable, or about to travel in the clear -- where amber would understate
 * what is at stake. Per-theme like the rest, because a red tuned for a light
 * background loses its contrast against a dark one.
 *
 * @param palette the palette of the widget the text belongs to
 * @return a red that stays legible under both light and dark themes
 */
auto GF_UI_EXPORT DangerColor(const QPalette& palette) -> QColor;

/**
 * @brief Render a small coloured status chip into a label.
 *
 * Uses an inline coloured span rather than a stylesheet, so the label keeps
 * the platform font and the colour can follow the palette.
 *
 * @param label the label to fill
 * @param text plain text, escaped by this function
 * @param color the text colour, usually from AccentColor()
 */
void GF_UI_EXPORT SetChip(QLabel* label, const QString& text,
                          const QColor& color);

/**
 * @brief Recolour a plain label's text through its palette.
 *
 * The sibling of SetChip() for labels that are not rich text. Going through the
 * palette rather than a stylesheet keeps the platform font, works in both
 * themes, and avoids the boxed-panel artefacts QSS introduces on some styles.
 *
 * @param label the label to recolour
 * @param color the text colour, usually from one of the functions above
 */
void GF_UI_EXPORT SetLabelTextColor(QLabel* label, const QColor& color);

/**
 * @brief Wrap content in a titled card.
 *
 * A hairline drawn from the palette rather than the platform's StyledPanel
 * groove, which is a bevel on some styles and nothing at all on others. Shared
 * rather than rebuilt per dialog so that every card in the application reads as
 * one set -- the alternative is each dialog inventing its own border radius and
 * padding, which is how a set of panels stops looking like a set.
 *
 * Lighter than a QGroupBox, which draws a heavy frame with a notched title and
 * makes a dialog of three sections look like a form from another decade.
 *
 * @param title the card's heading
 * @param content the widget to seat inside it; reparented to the card
 * @param parent parent widget
 * @return the card, ready to add to a layout
 */
auto GF_UI_EXPORT CreateCard(const QString& title, QWidget* content,
                             QWidget* parent = nullptr) -> QFrame*;

/**
 * @brief A file size as a person reads it.
 *
 * One function rather than the four private copies this replaced, because a
 * dialog that prints "50.2 kB" beside another that prints "50.2 KiB" makes the
 * reader wonder which of the two numbers is the real one.
 *
 * @param bytes the size
 * @return the localised size, traditional units
 */
auto GF_UI_EXPORT HumanSize(qint64 bytes) -> QString;

/**
 * @brief A quieter, slightly smaller label: the sentence under a value.
 *
 * Reads as an explanation of the line above rather than as a second value. The
 * colour goes through the widget's palette rather than an inline rich-text
 * span, which keeps the platform font a stylesheet would take over.
 *
 * @param text the sentence
 * @param parent parent widget
 * @return the label
 */
auto GF_UI_EXPORT CreateDetailLabel(const QString& text,
                                    QWidget* parent = nullptr) -> QLabel*;

/**
 * @brief The badge, heading, explanation and rule that open a serious dialog.
 *
 * Handing over a whole profile, unlocking one, and choosing what to do with a
 * file somebody sent are the same class of act, and sharing the furniture is
 * what makes them read as one family rather than three dialogs that happen to
 * be in the same application.
 *
 * @param icon a `:/icons/…` path for the 40px badge
 * @param title the heading, usually also the window title
 * @param subtitle one wrapped line saying what this is for; may be empty
 * @param parent parent widget
 * @return a layout to insert at the top of the dialog, rule included
 */
/**
 * @brief Hide something behind a disclosure the reader can open.
 *
 * For everything that is true, occasionally needed, and not what the page is
 * for. A page that answers the reader's question in four rows and keeps the
 * other twenty one click away is read; the same twenty-four rows in a column
 * are skimmed and trusted rather than read.
 *
 * @param title what the toggle offers, e.g. "More details"
 * @param content the widget to hide; reparented, and hidden to start with
 * @param parent parent widget
 * @return the toggle and its content, ready to add to a layout
 */
auto GF_UI_EXPORT CreateDisclosure(const QString& title, QWidget* content,
                                   QWidget* parent = nullptr) -> QWidget*;

auto GF_UI_EXPORT CreateDialogHeader(const QString& icon, const QString& title,
                                     const QString& subtitle, QWidget* parent)
    -> QLayout*;

}  // namespace GpgFrontend::UI
