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

#include "ui/widgets/SecretEntryPanel.h"

#include "core/function/PassphraseGenerator.h"
#include "core/utils/MemoryUtils.h"
#include "ui/dialog/PassphraseStrength.h"
#include "ui/function/UIStyle.h"

namespace GpgFrontend::UI {

namespace {

/// How long a generated passphrase is. Twenty-four characters over the
/// generator's 62-character alphabet is about 142 bits, which is far past
/// anything the Argon2id stretching has to make up for — and short enough to
/// still be copied by hand off a screen if that is what someone does with it.
constexpr int kGeneratedLength = 24;

}  // namespace

SecretEntryPanel::SecretEntryPanel(Config config, QWidget* parent)
    : QWidget(parent), config_(std::move(config)) {
  if (config_.ask_current) current_edit_ = make_field();
  if (config_.ask_new) {
    new_edit_ = make_field();
    confirm_edit_ = make_field();
  }

  reveal_box_ = new QCheckBox(config_.texts.reveal_label, this);
  connect(reveal_box_, &QCheckBox::toggled, this, [this](bool shown) {
    const auto echo = shown ? QLineEdit::Normal : QLineEdit::Password;
    for (auto* edit : {current_edit_, new_edit_, confirm_edit_}) {
      if (edit != nullptr) edit->setEchoMode(echo);
    }
  });

  if (config_.offer_generation) {
    generate_button_ = new QPushButton(tr("Generate"), this);
    generate_button_->setIcon(QIcon(":/icons/password-generate.svg"));
    generate_button_->setToolTip(
        tr("Invent a strong passphrase and show it, so you can write it down"));
    connect(generate_button_, &QPushButton::clicked, this,
            &SecretEntryPanel::generate);
  }

  if (config_.ask_new) {
    strength_bar_ = new QProgressBar(this);
    strength_bar_->setRange(0, 100);
    strength_bar_->setTextVisible(false);
    strength_label_ = new QLabel(this);
  }

  message_label_ = new QLabel(this);
  message_label_->setWordWrap(true);
  message_label_->setAlignment(Qt::AlignTop);
  // The message row does double duty: a dimmed hint when all is well, the retry
  // message in danger red on a failure. Keeping it always visible means the
  // reserved space carries guidance instead of reading as empty padding, and
  // the two-line floor keeps everything below it from moving when a longer
  // message replaces a shorter one.
  //
  // A prompt with no hint has nothing to put in the second line, and reserving
  // it anyway would leave a blank band above the buttons that reads as a
  // missing control. One line still absorbs the messages such a prompt shows.
  message_label_->setMinimumHeight(message_label_->fontMetrics().lineSpacing() *
                                   (config_.texts.hint.isEmpty() ? 1 : 2));

  if (config_.compact) {
    lay_out_compact();
  } else {
    lay_out_standard();
  }

  for (auto* edit : {current_edit_, new_edit_, confirm_edit_}) {
    if (edit == nullptr) continue;
    connect(edit, &QLineEdit::textChanged, this, &SecretEntryPanel::refresh);
  }

  if (config_.defer_validation) wire_deferred_validation();

  show_default_hint();
  refresh_strength();
  refresh();
}

void SecretEntryPanel::lay_out_standard() {
  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(10);

  // A grid rather than a QFormLayout so every field is laid out identically.
  // The generator deliberately does not sit in a column beside the field it
  // fills: giving it one either makes that field narrower than the one below it
  // — two password fields whose right edges disagree look like a mistake — or
  // leaves an empty column under the button, which looks like a missing
  // control. It lives on the row of controls below instead, beside the reveal
  // toggle, which is the other thing that acts on the fields above.
  auto* form = new QGridLayout();
  form->setHorizontalSpacing(12);
  form->setVerticalSpacing(10);
  form->setColumnStretch(1, 1);

  int row = 0;
  const auto add_field = [this, form, &row](const QString& label,
                                            QLineEdit* edit) {
    if (edit == nullptr) return;
    auto* caption = new QLabel(label, this);
    caption->setBuddy(edit);
    form->addWidget(caption, row, 0);
    form->addWidget(edit, row, 1);
    ++row;
  };

  add_field(config_.texts.current_label, current_edit_);
  add_field(config_.texts.new_label, new_edit_);
  add_field(config_.texts.confirm_label, confirm_edit_);

  layout->addLayout(form);

  // The controls that act on the fields above, set apart as quiet controls
  // rather than as more form rows competing with the field labels.
  auto* reveal_row = new QHBoxLayout();
  if (generate_button_ != nullptr) reveal_row->addWidget(generate_button_);
  reveal_row->addStretch(1);
  reveal_row->addWidget(reveal_box_);
  layout->addLayout(reveal_row);

  if (strength_bar_ != nullptr) {
    auto* caption = new QLabel(config_.texts.strength_caption, this);
    SetLabelTextColor(caption, MutedTextColor(caption->palette()));

    strength_bar_->setFixedHeight(8);

    auto* strength_row = new QHBoxLayout();
    strength_row->setSpacing(10);
    strength_row->addWidget(caption);
    strength_row->addWidget(strength_bar_, 1);
    strength_row->addWidget(strength_label_);
    layout->addLayout(strength_row);
  }

  layout->addWidget(message_label_);
}

void SecretEntryPanel::lay_out_compact() {
  // One column of labels and one of everything else. The strength sits right
  // under the field it grades, and the controls and the message line up with
  // the fields rather than with the labels, so the panel reads as a single
  // block instead of rows that each start somewhere different.
  auto* form = new QGridLayout(this);
  form->setContentsMargins(0, 0, 0, 0);
  form->setHorizontalSpacing(12);
  form->setVerticalSpacing(6);
  form->setColumnStretch(1, 1);

  int row = 0;
  const auto add_field = [this, form, &row](const QString& label,
                                            QLineEdit* edit) {
    if (edit == nullptr) return;
    auto* caption = new QLabel(label, this);
    caption->setBuddy(edit);
    form->addWidget(caption, row, 0);
    form->addWidget(edit, row, 1);
    ++row;
  };

  add_field(config_.texts.current_label, current_edit_);
  add_field(config_.texts.new_label, new_edit_);

  if (strength_bar_ != nullptr) {
    strength_bar_->setFixedHeight(4);
    strength_bar_->setAccessibleName(config_.texts.strength_caption);

    auto font = strength_label_->font();
    font.setPointSizeF(font.pointSizeF() * 0.92);
    strength_label_->setFont(font);

    auto* strength_row = new QHBoxLayout();
    strength_row->setSpacing(8);
    strength_row->addWidget(strength_bar_, 1, Qt::AlignVCenter);
    strength_row->addWidget(strength_label_);
    form->addLayout(strength_row, row++, 1);
  }

  add_field(config_.texts.confirm_label, confirm_edit_);

  // Quiet controls: the generator is a flat button, read as an offer rather
  // than as a second primary action competing with Continue.
  auto* controls = new QHBoxLayout();
  controls->setSpacing(12);
  if (generate_button_ != nullptr) {
    generate_button_->setText(config_.texts.generate_label.isEmpty()
                                  ? tr("Generate a strong passphrase")
                                  : config_.texts.generate_label);
    generate_button_->setFlat(true);
    generate_button_->setCursor(Qt::PointingHandCursor);

    // Drawn in the link colour so a flat button still reads as something to
    // click. The icon is dropped: its SVG paints with currentColor, which Qt
    // renders as nothing, and an empty gap before the text looks like a typo.
    generate_button_->setIcon({});
    auto palette = generate_button_->palette();
    palette.setColor(QPalette::ButtonText,
                     generate_button_->palette().color(QPalette::Link));
    generate_button_->setPalette(palette);
    controls->addWidget(generate_button_);
  }
  controls->addStretch(1);
  controls->addWidget(reveal_box_);
  form->addLayout(controls, row++, 1);

  // Left to right, as they are drawn; creation order would put the reveal
  // toggle first.
  if (generate_button_ != nullptr) {
    setTabOrder(generate_button_, reveal_box_);
  }

  form->addWidget(message_label_, row, 1);
}

void SecretEntryPanel::wire_deferred_validation() {
  // Clicking either must not take the caret out of a field: under deferred
  // validation, leaving a field is what names its problem, and showing the
  // passphrase is not a verdict on it. Both stay reachable with Tab.
  reveal_box_->setFocusPolicy(Qt::TabFocus);
  if (generate_button_ != nullptr) {
    generate_button_->setFocusPolicy(Qt::TabFocus);
  }

  for (auto* edit : {new_edit_, confirm_edit_}) {
    if (edit == nullptr) continue;
    edit->installEventFilter(this);
    // Return is a commit too. The dialog's default button runs after this, and
    // finds the field already judged.
    connect(edit, &QLineEdit::returnPressed, this, [this, edit]() {
      (edit == new_edit_ ? secret_committed_ : confirm_committed_) = true;
      refresh();
    });
  }
}

SecretEntryPanel::~SecretEntryPanel() {
  // scrub(), never Clear(). By the time a child widget is destroyed, the parent
  // dialog's own destructor body has already run and its vtable has been
  // downgraded to the base class, so announcing SignalStateChanged here would
  // deliver a slot call to an object that is no longer of the receiving type —
  // which Qt catches with "Called object is not of the correct type (class
  // destructor may have already run)" and aborts on. The wipe still has to
  // happen; only the announcement must not.
  scrub();
}

auto SecretEntryPanel::make_field() -> QLineEdit* {
  auto* edit = new QLineEdit(this);
  edit->setEchoMode(QLineEdit::Password);
  if (!config_.compact) edit->setMinimumHeight(kSecretFieldHeight);
  edit->setClearButtonEnabled(true);
  return edit;
}

auto SecretEntryPanel::Secret() const -> GFBuffer {
  auto* source = config_.ask_new ? new_edit_ : current_edit_;
  return source != nullptr ? GFBuffer(source->text()) : GFBuffer();
}

auto SecretEntryPanel::CurrentSecret() const -> GFBuffer {
  return current_edit_ != nullptr ? GFBuffer(current_edit_->text())
                                  : GFBuffer();
}

auto SecretEntryPanel::Acceptable() const -> bool { return state_.acceptable; }

void SecretEntryPanel::SetErrorText(const QString& text) {
  // An empty message is not "no message" but "back to guidance": the row is
  // always visible, so it falls back to the dimmed hint rather than a blank
  // gap.
  if (text.isEmpty()) {
    show_default_hint();
    return;
  }
  SetLabelTextColor(message_label_, DangerColor(palette()));
  message_label_->setText(text);
  // No adjustSize(): the row's height is reserved up front, so showing an error
  // must leave everything around it exactly where it was.
}

void SecretEntryPanel::show_default_hint() {
  SetLabelTextColor(message_label_, MutedTextColor(palette()));
  message_label_->setText(config_.texts.hint);
}

void SecretEntryPanel::scrub() {
  for (auto* edit : {current_edit_, new_edit_, confirm_edit_}) {
    if (edit == nullptr) continue;

    // Writing to a field emits textChanged, which would run refresh() once per
    // field and announce a state nobody asked about. Blocked here rather than
    // at the two call sites, so that scrubbing is silent by construction and a
    // future caller cannot reintroduce the destruction-time crash by forgetting
    // to block.
    const QSignalBlocker block(edit);

    // Overwrite before clearing; see the note on Clear() in the header for what
    // this does and does not achieve.
    auto scratch = edit->text();
    if (!scratch.isEmpty()) {
      WipeString(scratch);
      edit->setText(scratch);
    }
    edit->clear();
  }
}

void SecretEntryPanel::Clear() {
  scrub();
  // Announced explicitly, because emptying the fields genuinely does change the
  // state: a retry loop clears and expects its accept button to go back to
  // disabled. This also restores the idle hint, which is why callers setting an
  // error message must do so after clearing, not before.
  refresh();
}

void SecretEntryPanel::FocusFirstField() {
  if (current_edit_ != nullptr) {
    current_edit_->setFocus();
  } else if (new_edit_ != nullptr) {
    new_edit_->setFocus();
  }
}

auto SecretEntryPanel::eventFilter(QObject* watched, QEvent* event) -> bool {
  if (event->type() != QEvent::FocusOut) {
    return QWidget::eventFilter(watched, event);
  }

  // Another window, a context menu, or the dialog going away: the user has not
  // moved on from this field, so it is not finished.
  const auto reason = static_cast<QFocusEvent*>(event)->reason();
  if (reason == Qt::ActiveWindowFocusReason || reason == Qt::PopupFocusReason) {
    return QWidget::eventFilter(watched, event);
  }

  // Qt has already moved focus by the time the old widget hears FocusOut, so
  // this is where it went. The reveal toggle and the generator act on the
  // field rather than leave it.
  auto* next = QApplication::focusWidget();
  if (next == nullptr || next == reveal_box_ || next == generate_button_) {
    return QWidget::eventFilter(watched, event);
  }

  if (watched == new_edit_) secret_committed_ = true;
  if (watched == confirm_edit_) confirm_committed_ = true;
  refresh();

  return QWidget::eventFilter(watched, event);
}

void SecretEntryPanel::generate() {
  auto generated =
      PassphraseGenerator::GetInstance().Generate(kGeneratedLength);
  if (!generated.has_value() || generated->Empty()) {
    SetErrorText(tr("A passphrase could not be generated."));
    return;
  }

  const auto text = generated->ConvertToQString();
  new_edit_->setText(text);
  confirm_edit_->setText(text);

  // Force the reveal on. A passphrase nobody has seen is a passphrase nobody
  // can write down, and this one exists nowhere else the moment the dialog
  // closes — so hiding it behind dots would make the button useless.
  reveal_box_->setChecked(true);

  QGuiApplication::clipboard()->setText(text);

  refresh();
  SetErrorText({});
  SetLabelTextColor(message_label_, WarningColor(palette()));
  message_label_->setText(
      tr("Copied to the clipboard. Save it somewhere safe now — it is not "
         "stored anywhere, and it cannot be recovered later."));
}

void SecretEntryPanel::refresh_strength() {
  if (strength_bar_ == nullptr || new_edit_ == nullptr) return;

  // Fully rounded at whatever height the layout gave the bar.
  const auto radius = QString::number(strength_bar_->maximumHeight() / 2);

  // An empty field has no strength to grade. Painting it as a red "Very weak"
  // scolds the user before they have typed anything.
  if (config_.defer_validation && new_edit_->text().isEmpty()) {
    strength_bar_->setValue(0);
    strength_bar_->setStyleSheet(
        u"QProgressBar { border: none; border-radius: %1px; "
        "background-color: rgba(128, 128, 128, 60); }"_s.arg(radius));
    strength_label_->clear();
    return;
  }

  const auto strength = CalculatePassphraseStrength(new_edit_->text());
  const auto color = PassphraseStrengthColor(strength);

  strength_bar_->setValue(strength);
  strength_bar_->setStyleSheet(
      u"QProgressBar { border: none; border-radius: %2px; "
      "background-color: rgba(128, 128, 128, 60); }"
      "QProgressBar::chunk { border-radius: %2px; background-color: %1; }"_s
          .arg(color, radius));

  strength_label_->setStyleSheet(u"color: %1; font-weight: 600;"_s.arg(color));
  strength_label_->setText(PassphraseStrengthDescription(strength));
}

void SecretEntryPanel::refresh() {
  SecretEntryInput input;
  input.ask_current = config_.ask_current;
  input.ask_new = config_.ask_new;
  input.min_length = config_.texts.min_length;
  input.current_length = current_edit_ != nullptr
                             ? static_cast<int>(current_edit_->text().size())
                             : 0;
  input.secret_length =
      new_edit_ != nullptr ? static_cast<int>(new_edit_->text().size()) : 0;
  input.confirm_length = confirm_edit_ != nullptr
                             ? static_cast<int>(confirm_edit_->text().size())
                             : 0;
  input.confirm_matches = new_edit_ != nullptr && confirm_edit_ != nullptr &&
                          new_edit_->text() == confirm_edit_->text();

  // An emptied field starts over: whatever it was committed with is gone.
  if (input.secret_length == 0) secret_committed_ = false;
  if (input.confirm_length == 0) confirm_committed_ = false;

  input.defer_until_committed = config_.defer_validation;
  input.secret_committed = secret_committed_;
  input.confirm_committed = confirm_committed_;

  state_ = EvaluateSecretEntry(input);

  refresh_strength();

  switch (state_.problem) {
    case SecretEntryProblem::kTooShort:
      SetErrorText(
          config_.texts.too_short_message.arg(config_.texts.min_length));
      break;
    case SecretEntryProblem::kMismatch:
      SetErrorText(config_.texts.mismatch_message);
      break;
    case SecretEntryProblem::kCurrentMissing:
      SetErrorText(config_.texts.current_missing_message);
      break;
    case SecretEntryProblem::kNone:
    case SecretEntryProblem::kEmpty:
    case SecretEntryProblem::kPending:
      show_default_hint();
      break;
  }

  emit SignalStateChanged();
}

}  // namespace GpgFrontend::UI
