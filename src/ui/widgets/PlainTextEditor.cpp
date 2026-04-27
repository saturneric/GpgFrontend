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

#include "PlainTextEditor.h"

#include <QPainter>
#include <QTextBlock>

namespace GpgFrontend::UI {

LineNumberArea::LineNumberArea(PlainTextEditor* editor)
    : QWidget(editor), editor_(editor) {}

auto LineNumberArea::sizeHint() const -> QSize {
  return {editor_->LineNumberAreaWidth(), 0};
}

void LineNumberArea::paintEvent(QPaintEvent* event) {
  editor_->LineNumberAreaPaintEvent(event);
}

PlainTextEditor::PlainTextEditor(QWidget* parent)
    : QPlainTextEdit(parent), line_number_area_(new LineNumberArea(this)) {
  connect(this, &QPlainTextEdit::blockCountChanged, this,
          &PlainTextEditor::slot_update_line_number_area_width);
  connect(this, &QPlainTextEdit::updateRequest, this,
          &PlainTextEditor::slot_update_line_number_area);
  connect(this, &QPlainTextEdit::cursorPositionChanged, this,
          &PlainTextEditor::slot_highlight_current_line);

  slot_update_line_number_area_width(0);
  slot_highlight_current_line();
}

auto PlainTextEditor::LineNumberAreaWidth() const -> int {
  int digits = 1;
  int max = qMax(1, blockCount());

  while (max >= 10) {
    max /= 10;
    ++digits;
  }

  const int space =
      12 + (fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits);

  return space;
}

void PlainTextEditor::slot_update_line_number_area_width(int) {
  setViewportMargins(LineNumberAreaWidth(), 0, 0, 0);
}

void PlainTextEditor::slot_update_line_number_area(const QRect& rect, int dy) {
  if (dy != 0) {
    line_number_area_->scroll(0, dy);
  } else {
    line_number_area_->update(0, rect.y(), line_number_area_->width(),
                              rect.height());
  }

  if (rect.contains(viewport()->rect())) {
    slot_update_line_number_area_width(0);
  }
}

void PlainTextEditor::resizeEvent(QResizeEvent* event) {
  QPlainTextEdit::resizeEvent(event);

  const QRect cr = contentsRect();
  line_number_area_->setGeometry(
      QRect(cr.left(), cr.top(), LineNumberAreaWidth(), cr.height()));
}

void PlainTextEditor::LineNumberAreaPaintEvent(QPaintEvent* event) {
  QPainter painter(line_number_area_);

  const auto pal = palette();
  painter.fillRect(event->rect(), pal.alternateBase());

  QTextBlock block = firstVisibleBlock();
  int block_number = block.blockNumber();
  int top =
      qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
  int bottom = top + qRound(blockBoundingRect(block).height());

  painter.setFont(font());

  while (block.isValid() && top <= event->rect().bottom()) {
    if (block.isVisible() && bottom >= event->rect().top()) {
      const QString number = QString::number(block_number + 1);

      const bool is_current =
          textCursor().block().blockNumber() == block_number;

      QFont number_font = font();
      number_font.setBold(is_current);
      painter.setFont(number_font);

      painter.setPen(is_current ? pal.highlight().color() : pal.mid().color());

      painter.drawText(0, top, line_number_area_->width() - 6,
                       fontMetrics().height(), Qt::AlignRight, number);
    }

    block = block.next();
    top = bottom;
    bottom = top + qRound(blockBoundingRect(block).height());
    ++block_number;
  }
}

void PlainTextEditor::slot_highlight_current_line() {
  QList<QTextEdit::ExtraSelection> selections;

  if (!isReadOnly()) {
    QTextEdit::ExtraSelection selection;

    QColor line_color = palette().highlight().color();
    line_color.setAlpha(40);

    selection.format.setBackground(line_color);
    selection.format.setProperty(QTextFormat::FullWidthSelection, true);
    selection.cursor = textCursor();
    selection.cursor.clearSelection();

    selections.append(selection);
  }

  setExtraSelections(selections);
}

}  // namespace GpgFrontend::UI