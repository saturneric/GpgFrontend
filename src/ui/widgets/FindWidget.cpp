/**
 * Copyright (C) 2021 Saturneric
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
 * Saturneric<eric@bktus.com> starting on May 12, 2021.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include "FindWidget.h"

namespace GpgFrontend::UI {

FindWidget::FindWidget(QWidget* parent, PlainTextEditorPage* edit)
    : QWidget(parent), m_text_page_(edit) {
  find_edit_ = new QLineEdit(this);
  auto* closeButton = new QPushButton(
      this->style()->standardIcon(QStyle::SP_TitleBarCloseButton), QString(),
      this);
  auto* nextButton = new QPushButton(QIcon(":button_next.png"), QString());
  auto* previousButton = new QPushButton(QIcon(":button_previous.png"), "");

  auto* notificationWidgetLayout = new QHBoxLayout(this);
  notificationWidgetLayout->setContentsMargins(10, 0, 0, 0);
  notificationWidgetLayout->addWidget(new QLabel(QString(_("Find")) + ": "));
  notificationWidgetLayout->addWidget(find_edit_, 2);
  notificationWidgetLayout->addWidget(nextButton);
  notificationWidgetLayout->addWidget(previousButton);
  notificationWidgetLayout->addWidget(closeButton);

  this->setLayout(notificationWidgetLayout);
  connect(find_edit_, &QLineEdit::textEdited, this, &FindWidget::slot_find);
  connect(find_edit_, &QLineEdit::returnPressed, this,
          &FindWidget::slot_find_next);
  connect(nextButton, &QPushButton::clicked, this, &FindWidget::slot_find_next);
  connect(previousButton, &QPushButton::clicked, this,
          &FindWidget::slot_find_previous);
  connect(closeButton, &QPushButton::clicked, this, &FindWidget::slot_close);

  // The timer is necessary for setting the focus
  QTimer::singleShot(0, find_edit_, SLOT(setFocus()));
}

void FindWidget::set_background() {
  // auto cursor = m_text_page_->GetTextPage()->textCursor();
  // if match is found set background of QLineEdit to white, otherwise to red
  QPalette bgPalette(find_edit_->palette());

  if (!find_edit_->text().isEmpty() && m_text_page_->GetTextPage()
                                               ->document()
                                               ->find(find_edit_->text())
                                               .position() < 0) {
    bgPalette.setColor(QPalette::Base, "#ececba");
  } else {
    bgPalette.setColor(QPalette::Base, Qt::white);
  }
  find_edit_->setPalette(bgPalette);
}

void FindWidget::slot_find_next() {
  QTextCursor cursor = m_text_page_->GetTextPage()->textCursor();
  cursor = m_text_page_->GetTextPage()->document()->find(
      find_edit_->text(), cursor, QTextDocument::FindCaseSensitively);

  // if end of document is reached, restart search from beginning
  if (cursor.position() == -1) {
    cursor = m_text_page_->GetTextPage()->document()->find(
        find_edit_->text(), cursor, QTextDocument::FindCaseSensitively);
  }

  // cursor should not stay at -1, otherwise text is not editable
  // todo: check how gedit handles this
  if (cursor.position() != -1) {
    m_text_page_->GetTextPage()->setTextCursor(cursor);
  }
  this->set_background();
}

void FindWidget::slot_find() {
  QTextCursor cursor = m_text_page_->GetTextPage()->textCursor();

  if (cursor.anchor() == -1) {
    cursor = m_text_page_->GetTextPage()->document()->find(
        find_edit_->text(), cursor, QTextDocument::FindCaseSensitively);
  } else {
    cursor = m_text_page_->GetTextPage()->document()->find(
        find_edit_->text(), cursor.anchor(),
        QTextDocument::FindCaseSensitively);
  }

  // if end of document is reached, restart search from beginning
  if (cursor.position() == -1) {
    cursor = m_text_page_->GetTextPage()->document()->find(
        find_edit_->text(), cursor, QTextDocument::FindCaseSensitively);
  }

  // cursor should not stay at -1, otherwise text is not editable
  // todo: check how gedit handles this
  if (cursor.position() != -1) {
    m_text_page_->GetTextPage()->setTextCursor(cursor);
  }
  this->set_background();
}

void FindWidget::slot_find_previous() {
  QTextDocument::FindFlags flags;
  flags |= QTextDocument::FindBackward;
  flags |= QTextDocument::FindCaseSensitively;

  QTextCursor cursor = m_text_page_->GetTextPage()->textCursor();
  cursor = m_text_page_->GetTextPage()->document()->find(find_edit_->text(),
                                                         cursor, flags);

  // if begin of document is reached, restart search from end
  if (cursor.position() == -1) {
    cursor = m_text_page_->GetTextPage()->document()->find(
        find_edit_->text(), QTextCursor::End, flags);
  }

  // cursor should not stay at -1, otherwise text is not editable
  // todo: check how gedit handles this
  if (cursor.position() != -1) {
    m_text_page_->GetTextPage()->setTextCursor(cursor);
  }
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
