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

#include "FindWidget.h"

#include "ui/function/TextFind.h"

namespace GpgFrontend::UI {

FindWidget::FindWidget(QWidget* parent, PlainTextEditorPage* edit)
    : QWidget(parent), m_text_page_(edit) {
  find_edit_ = new QLineEdit(this);
  auto* close_button =
      new QPushButton(QIcon(":/icons/close.png"), QString(), this);
  auto* next_button =
      new QPushButton(QIcon(":/icons/button_next.png"), QString());
  auto* previous_button =
      new QPushButton(QIcon(":/icons/button_previous.png"), "");

  auto* notification_widget_layout = new QHBoxLayout(this);
  notification_widget_layout->setContentsMargins(10, 0, 0, 0);
  notification_widget_layout->addWidget(new QLabel(tr("Find") + ": "));
  notification_widget_layout->addWidget(find_edit_, 2);
  notification_widget_layout->addWidget(next_button);
  notification_widget_layout->addWidget(previous_button);
  notification_widget_layout->addWidget(close_button);

  this->setLayout(notification_widget_layout);
  connect(find_edit_, &QLineEdit::textEdited, this, &FindWidget::slot_find);
  connect(find_edit_, &QLineEdit::returnPressed, this,
          &FindWidget::slot_find_next);
  connect(next_button, &QPushButton::clicked, this,
          &FindWidget::slot_find_next);
  connect(previous_button, &QPushButton::clicked, this,
          &FindWidget::slot_find_previous);
  connect(close_button, &QPushButton::clicked, this, &FindWidget::slot_close);

  // The timer is necessary for setting the focus
  QTimer::singleShot(32, find_edit_, SLOT(setFocus()));
}

void FindWidget::set_background() {}

void FindWidget::slot_find_next() {
  auto* text_page = m_text_page_->GetTextPage();

  // Searched from the end of the current match, so repeated presses walk
  // forwards through the document.
  text_page->setTextCursor(FindInDocument(text_page->document(),
                                          text_page->textCursor(),
                                          find_edit_->text(), false));
  this->set_background();
}

void FindWidget::slot_find() {
  auto* text_page = m_text_page_->GetTextPage();

  // Restarted from where the current match begins rather than from its end:
  // typing one more character extends the match in place, and searching past
  // it would skip the very occurrence being typed out.
  auto cursor = text_page->textCursor();
  cursor.setPosition(cursor.selectionStart());

  text_page->setTextCursor(
      FindInDocument(text_page->document(), cursor, find_edit_->text(), false));
  this->set_background();
}

void FindWidget::slot_find_previous() {
  auto* text_page = m_text_page_->GetTextPage();

  text_page->setTextCursor(FindInDocument(text_page->document(),
                                          text_page->textCursor(),
                                          find_edit_->text(), true));
  this->set_background();
}

void FindWidget::keyPressEvent(QKeyEvent* e) {
  switch (e->key()) {
    case Qt::Key_Escape:
      this->slot_close();
      break;
    case Qt::Key_F3:
      if (e->modifiers() & Qt::ShiftModifier) {
        this->slot_find_previous();
      } else {
        this->slot_find_next();
      }
      break;
  }
}

void FindWidget::slot_close() {
  QTextCursor cursor = m_text_page_->GetTextPage()->textCursor();

  if (cursor.position() == -1) {
    cursor.setPosition(0);
    m_text_page_->GetTextPage()->setTextCursor(cursor);
  }
  m_text_page_->setFocus();
  close();
}

}  // namespace GpgFrontend::UI
