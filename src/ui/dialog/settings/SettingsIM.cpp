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

#include "SettingsIM.h"

#include <QClipboard>

#include "core/function/InstantMessageOperator.h"
#include "core/thread/Task.h"
#include "core/thread/TaskRunnerGetter.h"

namespace GpgFrontend::UI {

namespace {

/// Typing pause before the (memory-hard) fingerprint derivation is worth doing.
constexpr int kFingerprintDebounceMs = 500;

/// A phrase pasted from a chat app may arrive wrapped across lines; it is one
/// unbroken word, so every kind of whitespace simply comes back out.
auto CleanPhrase(const QString& text) -> QString {
  QString out;
  out.reserve(text.size());
  for (const auto c : text) {
    if (!c.isSpace()) out.append(c);
  }
  return out;
}

auto MonospaceFont(const QWidget* widget) -> QFont {
  auto font = widget->font();
  font.setStyleHint(QFont::Monospace);
  font.setFamily("Monospace");
  return font;
}

}  // namespace

InstantMessagingTab::InstantMessagingTab(QWidget* parent) : QWidget(parent) {
  // ---- the phrase itself -------------------------------------------------
  // The Message Book phrase whitens every instant-messaging token: its bytes
  // derive the shuffle + XOR that hide the message, so without the same phrase
  // a token is indistinguishable from random text.
  auto* book_box = new QGroupBox(tr("Message Book Phrase"), this);
  auto* book_layout = new QVBoxLayout(book_box);

  auto* intro = new QLabel(
      tr("A long secret you share with one friend. It makes your messages look "
         "like random text, so nobody can tell they are PGP at all. You and "
         "your friend must use exactly the same phrase."),
      book_box);
  intro->setWordWrap(true);
  book_layout->addWidget(intro);

  // A generated phrase is 256 characters, far too long for a one-line field,
  // so it gets a wrapping box the user can see and select in full.
  phrase_edit_ = new QPlainTextEdit(book_box);
  phrase_edit_->setFont(MonospaceFont(this));
  phrase_edit_->setLineWrapMode(QPlainTextEdit::WidgetWidth);
  phrase_edit_->setPlaceholderText(
      tr("No phrase set. Messages use the built-in default book."));
  phrase_edit_->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  phrase_edit_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  phrase_edit_->setFixedHeight(phrase_edit_->fontMetrics().lineSpacing() * 5 +
                               12);
  connect(phrase_edit_, &QPlainTextEdit::textChanged, this, [this]() {
    if (syncing_ || !revealed_) return;
    set_phrase(CleanPhrase(phrase_edit_->toPlainText()));
  });
  book_layout->addWidget(phrase_edit_);

  phrase_state_label_ = new QLabel(book_box);
  phrase_state_label_->setWordWrap(true);

  // One click gives a phrase no one will ever guess; the user's only job is to
  // get it to their friend privately.
  auto* generate_button = new QPushButton(tr("Generate"), book_box);
  generate_button->setToolTip(
      tr("Create a new random phrase. Share it with your friend so you both "
         "use the same one."));
  connect(generate_button, &QPushButton::clicked, this, [this]() {
    const auto phrase = InstantMessageOperator::GeneratePhrase();
    if (phrase.isEmpty()) return;
    set_phrase(phrase);
    // A fresh phrase has to be seen to be shared.
    set_revealed(true);
  });

  reveal_button_ = new QPushButton(tr("Show"), book_box);
  reveal_button_->setToolTip(tr("Show or hide the phrase."));
  connect(reveal_button_, &QPushButton::clicked, this,
          [this]() { set_revealed(!revealed_); });

  auto* copy_button = new QPushButton(tr("Copy"), book_box);
  copy_button->setToolTip(tr("Copy the phrase to the clipboard."));
  connect(copy_button, &QPushButton::clicked, this,
          [this]() { QApplication::clipboard()->setText(phrase_); });

  auto* paste_button = new QPushButton(tr("Paste"), book_box);
  paste_button->setToolTip(
      tr("Replace the phrase with the one on the clipboard."));
  connect(paste_button, &QPushButton::clicked, this, [this]() {
    const auto text = CleanPhrase(QApplication::clipboard()->text());
    if (!text.isEmpty()) set_phrase(text);
  });

  auto* clear_button = new QPushButton(tr("Clear"), book_box);
  clear_button->setToolTip(
      tr("Remove the phrase and fall back to the default book."));
  connect(clear_button, &QPushButton::clicked, this,
          [this]() { set_phrase({}); });

  // The buttons keep their natural width on the right; the state line gets a
  // row of its own so it never squeezes them into a narrow column.
  auto* button_row = new QHBoxLayout();
  button_row->setContentsMargins(0, 0, 0, 0);
  button_row->addStretch(1);
  button_row->addWidget(generate_button);
  button_row->addWidget(reveal_button_);
  button_row->addWidget(copy_button);
  button_row->addWidget(paste_button);
  button_row->addWidget(clear_button);
  book_layout->addWidget(phrase_state_label_);
  book_layout->addLayout(button_row);

  // ---- checking both sides agree -----------------------------------------
  auto* check_box = new QGroupBox(tr("Book Fingerprint"), this);
  auto* check_layout = new QVBoxLayout(check_box);

  auto* check_note = new QLabel(
      tr("A short code made from your phrase. Read it out with your friend to "
         "be sure you both have the same one. Unlike the phrase, this code is "
         "safe to say out loud."),
      check_box);
  check_note->setWordWrap(true);
  check_layout->addWidget(check_note);

  fingerprint_label_ = new QLabel(check_box);
  auto fpr_font = MonospaceFont(this);
  fpr_font.setBold(true);
  fpr_font.setPointSize(fpr_font.pointSize() + 2);
  fingerprint_label_->setFont(fpr_font);
  fingerprint_label_->setTextInteractionFlags(Qt::TextSelectableByMouse);

  auto* copy_fpr_button = new QPushButton(tr("Copy"), check_box);
  copy_fpr_button->setToolTip(tr("Copy the fingerprint to the clipboard."));
  connect(copy_fpr_button, &QPushButton::clicked, this,
          [this]() { QApplication::clipboard()->setText(fingerprint_); });

  auto* fpr_row = new QHBoxLayout();
  fpr_row->setContentsMargins(0, 0, 0, 0);
  fpr_row->addWidget(fingerprint_label_);
  fpr_row->addStretch(1);
  fpr_row->addWidget(copy_fpr_button);
  check_layout->addLayout(fpr_row);

  auto* storage_note = new QLabel(
      tr("The phrase is stored in the encrypted cache, never in the settings "
         "file. Send it to your friend over a private channel."),
      this);
  storage_note->setWordWrap(true);

  auto* layout = new QVBoxLayout(this);
  layout->addWidget(book_box);
  layout->addWidget(check_box);
  layout->addWidget(storage_note);
  layout->addStretch(1);
  setLayout(layout);

  // Deriving a fingerprint costs one Argon2id (64 MiB), so wait for a pause in
  // typing rather than doing it per keystroke.
  fingerprint_timer_ = new QTimer(this);
  fingerprint_timer_->setSingleShot(true);
  fingerprint_timer_->setInterval(kFingerprintDebounceMs);
  connect(fingerprint_timer_, &QTimer::timeout, this,
          &InstantMessagingTab::start_fingerprint_update);

  SetSettings();
}

void InstantMessagingTab::SetSettings() {
  set_phrase(InstantMessageOperator::BookPhrase());
  // Nothing to hide when there is no phrase yet.
  set_revealed(phrase_.isEmpty());
}

void InstantMessagingTab::ApplySettings() {
  if (phrase_ == InstantMessageOperator::BookPhrase()) return;
  InstantMessageOperator::SetBookPhrase(phrase_);
}

void InstantMessagingTab::set_revealed(bool revealed) {
  revealed_ = revealed;
  reveal_button_->setText(revealed ? tr("Hide") : tr("Show"));

  const QScopedValueRollback<bool> guard(syncing_, true);
  // Masked, the box is a placeholder rather than an editor: editing dots
  // would only destroy the phrase behind them.
  phrase_edit_->setReadOnly(!revealed);
  phrase_edit_->setPlainText(revealed || phrase_.isEmpty()
                                 ? phrase_
                                 : QString(phrase_.size(), QChar(u'•')));
}

void InstantMessagingTab::set_phrase(const QString& phrase) {
  phrase_ = phrase;

  if (phrase_edit_->toPlainText() != phrase_) {
    const QScopedValueRollback<bool> guard(syncing_, true);
    phrase_edit_->setPlainText(phrase_);
  }

  phrase_state_label_->setText(
      phrase_.isEmpty() ? tr("No phrase set. Using the built-in default.")
                        : tr("Phrase set. %1 characters.").arg(phrase_.size()));

  schedule_fingerprint_update();
}

void InstantMessagingTab::schedule_fingerprint_update() {
  // Invalidate any derivation already in flight: its result is for an older
  // phrase and must not overwrite what we are about to compute.
  ++fingerprint_request_;
  set_fingerprint({});
  fingerprint_label_->setText(tr("Calculating…"));
  fingerprint_timer_->start();
}

void InstantMessagingTab::start_fingerprint_update() {
  const auto phrase = phrase_;
  const auto request = fingerprint_request_;
  auto result = std::make_shared<QString>();
  QPointer<InstantMessagingTab> self(this);

  auto* task = new Thread::Task(
      [phrase, result](const DataObjectPtr&) -> int {
        *result = InstantMessageOperator::BookFingerprintOf(phrase);
        return 0;
      },
      "im_book_fingerprint", nullptr,
      [self, result, request](int, const DataObjectPtr&) {
        if (self == nullptr || self->fingerprint_request_ != request) return;
        self->set_fingerprint(*result);
      });

  Thread::TaskRunnerGetter::GetInstance()
      .GetTaskRunner(Thread::TaskRunnerGetter::kTaskRunnerType_Default)
      ->PostTask(task);
}

void InstantMessagingTab::set_fingerprint(const QString& fingerprint) {
  fingerprint_ = fingerprint;
  fingerprint_label_->setText(fingerprint.isEmpty() ? QString("...")
                                                    : fingerprint);
}

}  // namespace GpgFrontend::UI
