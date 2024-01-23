/* pinlineedit.cpp - Modified QLineEdit widget.
 * Copyright (C) 2018 Damien Goutte-Gattat
 * Copyright (C) 2021 g10 Code GmbH
 *
 * Software engineering by Ingo Klöcker <dev@ingo-kloecker.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses/>.
 * SPDX-License-Identifier: GPL-2.0+
 */

#include "pinlineedit.h"

#include <QClipboard>
#include <QGuiApplication>
#include <QKeyEvent>

static const int FormattedPassphraseGroupSize = 5;
static const QChar FormattedPassphraseSeparator = QChar::Nbsp;

namespace {
struct Selection {
  bool empty() const { return start < 0 || start >= end; }
  int length() const { return empty() ? 0 : end - start; }

  int start;
  int end;
};
}  // namespace

class PinLineEdit::Private {
  PinLineEdit *const q;

 public:
  Private(PinLineEdit *q) : q{q} {}

  QString formatted(QString text) const {
    const int dashCount = text.size() / FormattedPassphraseGroupSize;
    text.reserve(text.size() + dashCount);
    for (int i = FormattedPassphraseGroupSize; i < text.size();
         i += FormattedPassphraseGroupSize + 1) {
      text.insert(i, FormattedPassphraseSeparator);
    }
    return text;
  }

  Selection formattedSelection(Selection selection) const {
    if (selection.empty()) {
      return selection;
    }
    return {selection.start + selection.start / FormattedPassphraseGroupSize,
            selection.end + (selection.end - 1) / FormattedPassphraseGroupSize};
  }

  QString unformatted(QString text) const {
    for (int i = FormattedPassphraseGroupSize; i < text.size();
         i += FormattedPassphraseGroupSize) {
      text.remove(i, 1);
    }
    return text;
  }

  Selection unformattedSelection(Selection selection) const {
    if (selection.empty()) {
      return selection;
    }
    return {
        selection.start - selection.start / (FormattedPassphraseGroupSize + 1),
        selection.end - selection.end / (FormattedPassphraseGroupSize + 1)};
  }

  void copyToClipboard() {
    if (q->echoMode() != QLineEdit::Normal) {
      return;
    }

    QString text = q->selectedText();
    if (mFormattedPassphrase) {
      text.remove(FormattedPassphraseSeparator);
    }
    if (!text.isEmpty()) {
      QGuiApplication::clipboard()->setText(text);
    }
  }

  int selectionEnd() {
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
    return q->selectionEnd();
#else
    return q->selectionStart() + q->selectedText().size();
#endif
  }

 public:
  bool mFormattedPassphrase = false;
};

PinLineEdit::PinLineEdit(QWidget *parent)
    : QLineEdit(parent), d{new Private{this}} {
  connect(this, SIGNAL(textEdited(QString)), this, SLOT(textEdited()));
}

PinLineEdit::~PinLineEdit() = default;

void PinLineEdit::setFormattedPassphrase(bool on) {
  if (on == d->mFormattedPassphrase) {
    return;
  }
  d->mFormattedPassphrase = on;
  Selection selection{selectionStart(), d->selectionEnd()};
  if (d->mFormattedPassphrase) {
    setText(d->formatted(text()));
    selection = d->formattedSelection(selection);
  } else {
    setText(d->unformatted(text()));
    selection = d->unformattedSelection(selection);
  }
  if (!selection.empty()) {
    setSelection(selection.start, selection.length());
  }
}

void PinLineEdit::copy() const { d->copyToClipboard(); }

void PinLineEdit::cut() {
  if (hasSelectedText()) {
    copy();
    del();
  }
}

void PinLineEdit::setPin(const QString &pin) {
  setText(d->mFormattedPassphrase ? d->formatted(pin) : pin);
}

QString PinLineEdit::pin() const {
  if (d->mFormattedPassphrase) {
    return d->unformatted(text());
  } else {
    return text();
  }
}

void PinLineEdit::keyPressEvent(QKeyEvent *e) {
  if (e == QKeySequence::Copy) {
    copy();
    return;
  } else if (e == QKeySequence::Cut) {
    if (!isReadOnly() && hasSelectedText()) {
      copy();
      del();
    }
    return;
  } else if (e == QKeySequence::DeleteEndOfLine) {
    if (!isReadOnly()) {
      setSelection(cursorPosition(), text().size());
      copy();
      del();
    }
    return;
  } else if (e == QKeySequence::DeleteCompleteLine) {
    if (!isReadOnly()) {
      setSelection(0, text().size());
      copy();
      del();
    }
    return;
  }

  QLineEdit::keyPressEvent(e);

  if (e->key() == Qt::Key::Key_Backspace) {
    emit backspacePressed();
  }
}

void PinLineEdit::textEdited() {
  if (!d->mFormattedPassphrase) {
    return;
  }
  auto currentText = text();
  // first calculate the cursor position in the reformatted text; the cursor
  // is put left of the separators, so that backspace works as expected
  auto cursorPos = cursorPosition();
  cursorPos -= QStringView{currentText}.left(cursorPos).count(
      FormattedPassphraseSeparator);
  cursorPos += std::max(cursorPos - 1, 0) / FormattedPassphraseGroupSize;
  // then reformat the text
  currentText.remove(FormattedPassphraseSeparator);
  currentText = d->formatted(currentText);
  // finally, set reformatted text and updated cursor position
  setText(currentText);
  setCursorPosition(cursorPos);
}
