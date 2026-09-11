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

#include "ui/function/TextDirection.h"

#include <QPlainTextEdit>
#include <QTextBlock>
#include <QTextDocument>
#include <QTextOption>
#include <optional>

namespace GpgFrontend::UI {

namespace {

/// The direction of the first strong character in @p text, or nothing when it
/// holds none: the caller has to tell "runs left-to-right" apart from "says
/// nothing either way" to know whether it may stop looking.
auto FirstStrongDirection(const QString& text)
    -> std::optional<Qt::LayoutDirection> {
  // Walked by code point rather than by QChar: right-to-left scripts live above
  // the BMP too, and a lone surrogate half classifies as left-to-right, so a
  // plain QChar loop would answer the wrong way for them.
  for (qsizetype i = 0; i < text.size(); ++i) {
    auto code_point = static_cast<char32_t>(text.at(i).unicode());
    if (text.at(i).isHighSurrogate() && i + 1 < text.size() &&
        text.at(i + 1).isLowSurrogate()) {
      code_point = QChar::surrogateToUcs4(text.at(i), text.at(i + 1));
      ++i;
    }

    switch (QChar::direction(code_point)) {
      case QChar::DirL:
        return Qt::LeftToRight;
      case QChar::DirR:
      case QChar::DirAL:
        return Qt::RightToLeft;
      default:
        break;
    }
  }

  return std::nullopt;
}

/// Writes @p base as the direction every paragraph starts from and anchors the
/// alignment to @p anchor, leaving the rest of the option alone.
///
/// @p base may be Qt::LayoutDirectionAuto, which asks QTextEngine to resolve
/// each paragraph from its own text. @p anchor is always a real direction: the
/// alignment is one value for the whole document either way, so it needs an
/// answer even when the paragraphs disagree.
void SetDocumentTextOption(QTextDocument* doc, Qt::LayoutDirection base,
                           Qt::LayoutDirection anchor) {
  auto option = doc->defaultTextOption();

  // The two document layouts want opposite things, and getting it the wrong way
  // round is silent: the text simply does not move.
  //
  // QTextDocumentLayout, behind QTextEdit and QTextBrowser, resolves the
  // default leading alignment against each block's own direction, so asking it
  // for Qt::AlignRight is reversed straight back to the left edge. Left alone,
  // it right-aligns right-to-left paragraphs by itself, per paragraph.
  //
  // QPlainTextDocumentLayout, behind QPlainTextEdit, does no such thing. It
  // reorders the characters for the base direction but leaves every line parked
  // at the left margin unless an alignment says otherwise, and it ignores the
  // per-block formats that would say so, so one document-wide answer is all it
  // can be given.
  const auto alignment =
      qobject_cast<QPlainTextDocumentLayout*>(doc->documentLayout()) != nullptr
          ? Qt::Alignment(anchor == Qt::RightToLeft ? Qt::AlignRight
                                                    : Qt::AlignLeft)
          : option.alignment();

  if (option.textDirection() == base && option.alignment() == alignment) return;

  // Re-lays out every block, so it is only worth doing on an actual change.
  option.setTextDirection(base);
  option.setAlignment(alignment);
  doc->setDefaultTextOption(option);
}

}  // namespace

auto DetectTextDirection(const QString& text) -> Qt::LayoutDirection {
  return FirstStrongDirection(text).value_or(Qt::LeftToRight);
}

auto DetectTextDirection(const QTextDocument* doc) -> Qt::LayoutDirection {
  if (doc == nullptr) return Qt::LeftToRight;

  for (auto block = doc->begin(); block.isValid(); block = block.next()) {
    if (const auto direction = FirstStrongDirection(block.text())) {
      return *direction;
    }
  }

  return Qt::LeftToRight;
}

void ApplyTextDirectionToDocument(QWidget* view, QTextDocument* doc,
                                  Qt::LayoutDirection dir) {
  if (view != nullptr) view->setLayoutDirection(dir);
  if (doc == nullptr) return;

  SetDocumentTextOption(doc, dir, dir);
}

void ApplyTextDirectionToDocument(QWidget* view, QTextDocument* doc,
                                  TextDirectionMode mode) {
  if (mode != kTEXT_DIRECTION_AUTO) {
    ApplyTextDirectionToDocument(
        view, doc,
        mode == kTEXT_DIRECTION_RTL ? Qt::RightToLeft : Qt::LeftToRight);
    return;
  }

  // Automatic still needs one answer for the things a document has only one of.
  // The widget decides which side its scroll bar and its line number gutter
  // open on, and a plain text editor aligns every line to the same edge; a
  // document that opens in Arabic should get both, even though each paragraph
  // goes on to resolve its own reading order.
  const auto anchor = DetectTextDirection(doc);

  if (view != nullptr) view->setLayoutDirection(anchor);
  if (doc == nullptr) return;

  SetDocumentTextOption(doc, Qt::LayoutDirectionAuto, anchor);
}

}  // namespace GpgFrontend::UI
