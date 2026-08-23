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
#include "ui/UserInterfaceUtils.h"
#include "ui/dialog/PassphraseStrength.h"

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
  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(10);

  // A grid rather than a QFormLayout, because the generator button needs a
  // column of its own. Putting it in a nested row beside one field makes that
  // field narrower than the one under it, and two password fields whose right
  // edges do not line up look like a mistake every time the dialog opens. With
  // the button in column two, every field ends exactly where that column
  // starts, whether or not the row beside it has a button.
  auto* form = new QGridLayout();
  form->setHorizontalSpacing(12);
  form->setVerticalSpacing(10);
  form->setColumnStretch(1, 1);

  int row = 0;
  const auto add_field = [this, form, &row](const QString& label,
                                            QLineEdit* edit) {
    form->addWidget(new QLabel(label, this), row, 0);
    form->addWidget(edit, row, 1);
    ++row;
  };

  if (config_.ask_current) {
    current_edit_ = make_field();
    add_field(config_.texts.current_label, current_edit_);
  }

  if (config_.ask_new) {
    new_edit_ = make_field();
    const auto new_row = row;
    add_field(config_.texts.new_label, new_edit_);

    // The generator sits on the row it fills rather than off in a corner, so it
    // reads as an alternative to typing there rather than as a separate
    // feature.
    if (config_.offer_generation) {
      generate_button_ = new QPushButton(tr("Generate"), this);
      generate_button_->setIcon(QIcon(":/icons/password-generate.svg"));
      generate_button_->setToolTip(tr(
          "Invent a strong passphrase and show it, so you can write it down"));
      form->addWidget(generate_button_, new_row, 2);

      connect(generate_button_, &QPushButton::clicked, this,
              &SecretEntryPanel::generate);
    }

    confirm_edit_ = make_field();
    add_field(config_.texts.confirm_label, confirm_edit_);
  }

  layout->addLayout(form);

  // Set apart as a quiet control rather than another form row competing with
  // the field labels.
  reveal_box_ = new QCheckBox(config_.texts.reveal_label, this);
  auto* reveal_row = new QHBoxLayout();
  reveal_row->addStretch(1);
  reveal_row->addWidget(reveal_box_);
  layout->addLayout(reveal_row);

  connect(reveal_box_, &QCheckBox::toggled, this, [this](bool shown) {
    const auto echo = shown ? QLineEdit::Normal : QLineEdit::Password;
    for (auto* edit : {current_edit_, new_edit_, confirm_edit_}) {
      if (edit != nullptr) edit->setEchoMode(echo);
    }
  });

  if (config_.ask_new) {
    auto* caption = new QLabel(config_.texts.strength_caption, this);
    SetLabelTextColor(caption, MutedTextColor(caption->palette()));

    strength_bar_ = new QProgressBar(this);
    strength_bar_->setRange(0, 100);
    strength_bar_->setTextVisible(false);
    strength_bar_->setFixedHeight(8);
    strength_label_ = new QLabel(this);

    auto* strength_row = new QHBoxLayout();
    strength_row->setSpacing(10);
    strength_row->addWidget(caption);
    strength_row->addWidget(strength_bar_, 1);
    strength_row->addWidget(strength_label_);
    layout->addLayout(strength_row);
  }

  message_label_ = new QLabel(this);
  message_label_->setWordWrap(true);
  message_label_->setAlignment(Qt::AlignTop);
  // The message row does double duty: a dimmed hint when all is well, the retry
  // message in danger red on a failure. Keeping it always visible means the
  // reserved space carries guidance instead of reading as empty padding, and
  // the two-line floor keeps everything below it from moving when a longer
  // message replaces a shorter one.
  message_label_->setMinimumHeight(message_label_->fontMetrics().lineSpacing() *
                                   2);
  layout->addWidget(message_label_);

  for (auto* edit : {current_edit_, new_edit_, confirm_edit_}) {
    if (edit == nullptr) continue;
    connect(edit, &QLineEdit::textChanged, this, &SecretEntryPanel::refresh);
  }

  show_default_hint();
  refresh_strength();
  refresh();
}

SecretEntryPanel::~SecretEntryPanel() { Clear(); }

auto SecretEntryPanel::make_field() -> QLineEdit* {
  auto* edit = new QLineEdit(this);
  edit->setEchoMode(QLineEdit::Password);
  edit->setMinimumHeight(kSecretFieldHeight);
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

void SecretEntryPanel::Clear() {
  for (auto* edit : {current_edit_, new_edit_, confirm_edit_}) {
    if (edit == nullptr) continue;
    // Overwrite before clearing; see the note on Clear() in the header for what
    // this does and does not achieve.
    auto scratch = edit->text();
    if (!scratch.isEmpty()) {
      scratch.fill('X');
      edit->setText(scratch);
    }
    edit->clear();
  }
}

void SecretEntryPanel::FocusFirstField() {
  if (current_edit_ != nullptr) {
    current_edit_->setFocus();
  } else if (new_edit_ != nullptr) {
    new_edit_->setFocus();
  }
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

  const auto strength = CalculatePassphraseStrength(new_edit_->text());
  const auto color = PassphraseStrengthColor(strength);

  strength_bar_->setValue(strength);
  strength_bar_->setStyleSheet(
      QStringLiteral(
          "QProgressBar { border: none; border-radius: 4px; "
          "background-color: rgba(128, 128, 128, 60); }"
          "QProgressBar::chunk { border-radius: 4px; background-color: %1; }")
          .arg(color));

  strength_label_->setStyleSheet(
      QStringLiteral("color: %1; font-weight: 600;").arg(color));
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
      show_default_hint();
      break;
  }

  emit SignalStateChanged();
}

}  // namespace GpgFrontend::UI
