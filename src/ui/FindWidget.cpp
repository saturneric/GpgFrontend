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

#include "ui/FindWidget.h"

namespace GpgFrontend::UI {

FindWidget::FindWidget(QWidget* parent, PlainTextEditorPage* edit)
    : QWidget(parent), mTextpage(edit) {
  findEdit = new QLineEdit(this);
  auto* closeButton = new QPushButton(
      this->style()->standardIcon(QStyle::SP_TitleBarCloseButton), QString(),
      this);
  auto* nextButton = new QPushButton(QIcon(":button_next.png"), QString());
  auto* previousButton = new QPushButton(QIcon(":button_previous.png"), "");

  auto* notificationWidgetLayout = new QHBoxLayout(this);
  notificationWidgetLayout->setContentsMargins(10, 0, 0, 0);
  notificationWidgetLayout->addWidget(new QLabel(QString(_("Find")) + ": "));
  notificationWidgetLayout->addWidget(findEdit, 2);
  notificationWidgetLayout->addWidget(nextButton);
  notificationWidgetLayout->addWidget(previousButton);
  notificationWidgetLayout->addWidget(closeButton);

  this->setLayout(notificationWidgetLayout);
  connect(findEdit, SIGNAL(textEdited(QString)), this, SLOT(slotFind()));
  connect(findEdit, SIGNAL(returnPressed()), this, SLOT(slotFindNext()));
  connect(nextButton, SIGNAL(clicked()), this, SLOT(slotFindNext()));
  connect(previousButton, SIGNAL(clicked()), this, SLOT(slotFindPrevious()));
  connect(closeButton, SIGNAL(clicked()), this, SLOT(slotClose()));

  // The timer is necessary for setting the focus
  QTimer::singleShot(0, findEdit, SLOT(setFocus()));
}

void FindWidget::setBackground() {
  auto cursor = mTextpage->getTextPage()->textCursor();
  // if match is found set background of QLineEdit to white, otherwise to red
  QPalette bgPalette(findEdit->palette());

  if (!findEdit->text().isEmpty() &&
      mTextpage->getTextPage()->document()->find(findEdit->text()).position() <
          0) {
    bgPalette.setColor(QPalette::Base, "#ececba");
  } else {
    bgPalette.setColor(QPalette::Base, Qt::white);
  }
  findEdit->setPalette(bgPalette);
}

void FindWidget::slotFindNext() {
  QTextCursor cursor = mTextpage->getTextPage()->textCursor();
  cursor = mTextpage->getTextPage()->document()->find(
      findEdit->text(), cursor, QTextDocument::FindCaseSensitively);

  // if end of document is reached, restart search from beginning
  if (cursor.position() == -1) {
    cursor = mTextpage->getTextPage()->document()->find(
        findEdit->text(), cursor, QTextDocument::FindCaseSensitively);
  }

  // cursor should not stay at -1, otherwise text is not editable
  // todo: check how gedit handles this
  if (cursor.position() != -1) {
    mTextpage->getTextPage()->setTextCursor(cursor);
  }
  this->setBackground();
}

void FindWidget::slotFind() {
  QTextCursor cursor = mTextpage->getTextPage()->textCursor();

  if (cursor.anchor() == -1) {
    cursor = mTextpage->getTextPage()->document()->find(
        findEdit->text(), cursor, QTextDocument::FindCaseSensitively);
  } else {
    cursor = mTextpage->getTextPage()->document()->find(
        findEdit->text(), cursor.anchor(), QTextDocument::FindCaseSensitively);
  }

  // if end of document is reached, restart search from beginning
  if (cursor.position() == -1) {
    cursor = mTextpage->getTextPage()->document()->find(
        findEdit->text(), cursor, QTextDocument::FindCaseSensitively);
  }

  // cursor should not stay at -1, otherwise text is not editable
  // todo: check how gedit handles this
  if (cursor.position() != -1) {
    mTextpage->getTextPage()->setTextCursor(cursor);
  }
  this->setBackground();
}

void FindWidget::slotFindPrevious() {
  QTextDocument::FindFlags flags;
  flags |= QTextDocument::FindBackward;
  flags |= QTextDocument::FindCaseSensitively;

  QTextCursor cursor = mTextpage->getTextPage()->textCursor();
  cursor = mTextpage->getTextPage()->document()->find(findEdit->text(), cursor,
                                                      flags);

  // if begin of document is reached, restart search from end
  if (cursor.position() == -1) {
    cursor = mTextpage->getTextPage()->document()->find(
        findEdit->text(), QTextCursor::End, flags);
  }

  // cursor should not stay at -1, otherwise text is not editable
  // todo: check how gedit handles this
  if (cursor.position() != -1) {
    mTextpage->getTextPage()->setTextCursor(cursor);
  }
  this->setBackground();
}

void FindWidget::keyPressEvent(QKeyEvent* e) {
  switch (e->key()) {
    case Qt::Key_Escape:
      this->slotClose();
      break;
    case Qt::Key_F3:
      if (e->modifiers() & Qt::ShiftModifier) {
        this->slotFindPrevious();
      } else {
        this->slotFindNext();
      }
      break;
  }
}

void FindWidget::slotClose() {
  QTextCursor cursor = mTextpage->getTextPage()->textCursor();

  if (cursor.position() == -1) {
    cursor.setPosition(0);
    mTextpage->getTextPage()->setTextCursor(cursor);
  }
  mTextpage->setFocus();
  close();
}

}  // namespace GpgFrontend::UI
