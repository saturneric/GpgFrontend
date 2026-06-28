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

#include "PassphraseDialog.h"

namespace {

auto PassphraseStrengthDescription(int strength) -> QString {
  if (strength < 20) {
    return QObject::tr("Very weak");
  }
  if (strength < 40) {
    return QObject::tr("Weak");
  }
  if (strength < 60) {
    return QObject::tr("Fair");
  }
  if (strength < 80) {
    return QObject::tr("Good");
  }
  return QObject::tr("Strong");
}

auto PassphraseStrengthColor(int strength) -> QString {
  if (strength < 20) {
    return QStringLiteral("#e53935");
  }
  if (strength < 40) {
    return QStringLiteral("#fb8c00");
  }
  if (strength < 60) {
    return QStringLiteral("#fbc02d");
  }
  if (strength < 80) {
    return QStringLiteral("#7cb342");
  }
  return QStringLiteral("#43a047");
}

// Penalizes repeated characters (e.g. "aaaa") and sequential runs
// (e.g. "abcd", "4321") of three or more characters, which the
// per-character-class score would otherwise rate as strong.
auto CalculatePatternPenalty(const QString& text) -> int {
  const int length = static_cast<int>(text.size());
  if (length < 3) {
    return 0;
  }

  auto penalty = 0;
  auto repeat_run = 1;
  auto sequence_run = 1;

  for (int i = 1; i < length; ++i) {
    const auto prev = text[i - 1].unicode();
    const auto curr = text[i].unicode();

    repeat_run = curr == prev ? repeat_run + 1 : 1;
    if (repeat_run >= 3) {
      penalty += 6;
    }

    const auto step = curr - prev;
    if (i >= 2 && step == prev - text[i - 2].unicode() &&
        (step == 1 || step == -1)) {
      sequence_run += 1;
    } else {
      sequence_run = 1;
    }
    if (sequence_run >= 3) {
      penalty += 6;
    }
  }

  return penalty;
}

auto CalculatePassphraseStrength(const QString& text) -> int {
  if (text.isEmpty()) {
    return 0;
  }

  const int length = static_cast<int>(text.size());

  bool has_lower = false;
  bool has_upper = false;
  bool has_digit = false;
  bool has_symbol = false;

  for (const auto ch : text) {
    if (ch.isLower()) {
      has_lower = true;
    } else if (ch.isUpper()) {
      has_upper = true;
    } else if (ch.isDigit()) {
      has_digit = true;
    } else {
      has_symbol = true;
    }
  }

  auto score = 0;
  score += std::min(length * 5, 35);
  score += has_lower ? 10 : 0;
  score += has_upper ? 15 : 0;
  score += has_digit ? 15 : 0;
  score += has_symbol ? 20 : 0;

  if (length >= 12) {
    score += 5;
  }
  if (length >= 16) {
    score += 5;
  }

  score -= CalculatePatternPenalty(text);

  return std::clamp(score, 0, 100);
}

}  // namespace

namespace GpgFrontend::UI {

PassphraseDialog::PassphraseDialog(
    const QSharedPointer<GpgPassphraseContext>& ctx, QWidget* parent)
    : QDialog(parent), ctx_(ctx) {
  setWindowTitle(tr("Passphrase Required"));
  setModal(true);
  setMinimumWidth(480);
  resize(520, ctx_->ShouldConfirm() ? 360 : 310);

  auto* main_layout = new QVBoxLayout(this);
  main_layout->setContentsMargins(22, 22, 22, 18);
  main_layout->setSpacing(14);

  auto* title_label = new QLabel(tr("Enter Passphrase"), this);
  QFont title_font = title_label->font();
  title_font.setPointSize(title_font.pointSize() + 2);
  title_font.setBold(true);
  title_label->setFont(title_font);

  auto* info_label = new QLabel(
      tr("Please enter the passphrase required for the current operation."),
      this);
  info_label->setWordWrap(true);

  auto* details_frame = new QFrame(this);
  details_frame->setObjectName("detailsFrame");
  details_frame->setFrameShape(QFrame::StyledPanel);
  details_frame->setFrameShadow(QFrame::Plain);

  auto* details_layout = new QVBoxLayout(details_frame);
  details_layout->setContentsMargins(12, 10, 12, 10);
  details_layout->setSpacing(6);

  QString detail_text =
      tr("Passphrase info: %1").arg(ctx_->GetPassphraseInfo());

  if (ctx_->IsAskForNew()) {
    detail_text +=
        "\n" + tr("This passphrase will be used to set a new password.");
  }

  auto key = ctx_->GetKey();
  if (key != nullptr) {
    detail_text += "\n" + tr("Key ID: %1").arg(key->ID());
    detail_text += "\n" + tr("Key UID: %1").arg(key->UID());
  }

  auto* details_label = new QLabel(detail_text, details_frame);
  details_label->setTextInteractionFlags(Qt::TextSelectableByMouse);
  details_label->setWordWrap(true);
  details_layout->addWidget(details_label);

  auto* form_layout = new QFormLayout();
  form_layout->setContentsMargins(0, 2, 0, 0);
  form_layout->setHorizontalSpacing(14);
  form_layout->setVerticalSpacing(10);
  form_layout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
  form_layout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);

  password_edit_ = new QLineEdit(this);
  password_edit_->setEchoMode(QLineEdit::Password);
  password_edit_->setPlaceholderText(tr("Enter your passphrase here"));
#ifndef Q_OS_MACOS
  password_edit_->setMinimumHeight(30);
#endif

  confirm_password_edit_ = new QLineEdit(this);
  confirm_password_edit_->setEchoMode(QLineEdit::Password);
  confirm_password_edit_->setPlaceholderText(tr("Enter your passphrase again"));
#ifndef Q_OS_MACOS
  confirm_password_edit_->setMinimumHeight(30);
#endif

  auto* password_row = new QWidget(this);
  auto* password_row_layout = new QHBoxLayout(password_row);
  password_row_layout->setContentsMargins(0, 0, 0, 0);
  password_row_layout->setSpacing(8);

  show_password_checkbox_ = new QCheckBox(tr("Show"), this);

  connect(show_password_checkbox_, &QCheckBox::toggled, this,
          [this](bool checked) {
            const auto echo_mode =
                checked ? QLineEdit::Normal : QLineEdit::Password;

            password_edit_->setEchoMode(echo_mode);

            if (confirm_password_edit_ != nullptr) {
              confirm_password_edit_->setEchoMode(echo_mode);
            }
          });

  connect(password_edit_, &QLineEdit::textChanged, this,
          [this](const QString& text) { update_passphrase_strength(text); });

  password_row_layout->addWidget(password_edit_, 1);
  password_row_layout->addWidget(show_password_checkbox_);

  form_layout->addRow(tr("Passphrase:"), password_row);

  auto* strength_widget = new QWidget(this);
  auto* strength_layout = new QHBoxLayout(strength_widget);
  strength_layout->setContentsMargins(0, 0, 0, 0);
  strength_layout->setSpacing(8);

  passphrase_strength_bar_ = new QProgressBar(strength_widget);
  passphrase_strength_bar_->setRange(0, 100);
  passphrase_strength_bar_->setTextVisible(false);
  passphrase_strength_bar_->setFixedHeight(8);

  passphrase_strength_label_ = new QLabel(strength_widget);
  passphrase_strength_label_->setMinimumWidth(80);
  passphrase_strength_label_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

  strength_layout->addWidget(passphrase_strength_bar_, 1);
  strength_layout->addWidget(passphrase_strength_label_);

  if (ctx_->IsAskForNew()) {
    form_layout->addRow(tr("Strength:"), strength_widget);
  } else {
    passphrase_strength_bar_->setVisible(false);
    passphrase_strength_label_->setVisible(false);
    strength_widget->hide();
  }

  if (ctx_->ShouldConfirm()) {
    form_layout->addRow(tr("Confirm:"), confirm_password_edit_);
  } else {
    confirm_password_edit_->hide();
  }

  auto* button_layout = new QHBoxLayout();
  button_layout->setContentsMargins(0, 6, 0, 0);

  timeout_label_ = new QLabel(this);
  button_layout->addWidget(timeout_label_);
  button_layout->addStretch();

  auto* cancel_button = new QPushButton(tr("Cancel"), this);
  auto* ok_button = new QPushButton(tr("OK"), this);

  ok_button->setDefault(true);
  ok_button->setAutoDefault(true);

  connect(ok_button, &QPushButton::clicked, this, [this]() {
    if (!validate_passphrase_input()) return;
    accept();
  });
  connect(cancel_button, &QPushButton::clicked, this, [this]() { reject(); });

  button_layout->addWidget(cancel_button);
  button_layout->addWidget(ok_button);

  main_layout->addWidget(title_label);
  main_layout->addWidget(info_label);
  main_layout->addWidget(details_frame);
  main_layout->addLayout(form_layout);
  main_layout->addLayout(button_layout);

  update_passphrase_strength(password_edit_->text());
  password_edit_->setFocus();

  // Abort the operation if the user does not respond within the timeout window.
  // A rejected dialog yields an empty passphrase, which the caller treats as
  // cancellation.
  update_timeout_label();
  timeout_timer_ = new QTimer(this);
  timeout_timer_->setInterval(1000);
  connect(timeout_timer_, &QTimer::timeout, this, [this]() {
    if (--remaining_seconds_ <= 0) {
      timeout_timer_->stop();
      reject();
      return;
    }
    update_timeout_label();
  });
  timeout_timer_->start();

  adjustSize();
  resize(std::max(width(), 520), height());
}

[[nodiscard]] auto PassphraseDialog::Passphrase() const -> GFBuffer {
  return password_edit_ != nullptr ? GFBuffer(password_edit_->text())
                                   : GFBuffer();
}

[[nodiscard]] auto PassphraseDialog::validate_passphrase_input() -> bool {
  if (password_edit_ == nullptr) {
    return false;
  }

  if (!ctx_->ShouldConfirm()) {
    return true;
  }

  if (confirm_password_edit_ == nullptr) {
    return false;
  }

  const auto passphrase = password_edit_->text();
  const auto confirmation = confirm_password_edit_->text();

  if (passphrase.isEmpty()) {
    QMessageBox::warning(this, tr("Empty Passphrase"),
                         tr("Passphrase cannot be empty. Please enter a valid "
                            "passphrase."));

    password_edit_->setFocus();
    password_edit_->selectAll();
    return false;
  }

  if (passphrase != confirmation) {
    QMessageBox::warning(this, tr("Passphrase Mismatch"),
                         tr("The two passphrases do not match. "
                            "Please enter them again."));

    confirm_password_edit_->clear();
    confirm_password_edit_->setFocus();
    confirm_password_edit_->selectAll();
    return false;
  }

  return true;
}

void PassphraseDialog::Clear() {
  if (password_edit_ != nullptr) {
    password_edit_->clear();
  }

  if (confirm_password_edit_ != nullptr) {
    confirm_password_edit_->clear();
  }
}

void PassphraseDialog::update_timeout_label() {
  if (timeout_label_ == nullptr) {
    return;
  }

  const auto minutes = remaining_seconds_ / 60;
  const auto seconds = remaining_seconds_ % 60;
  timeout_label_->setText(tr("Closing in %1:%2")
                              .arg(minutes)
                              .arg(seconds, 2, 10, QLatin1Char('0')));
}

void PassphraseDialog::update_passphrase_strength(const QString& text) {
  if (passphrase_strength_bar_ == nullptr ||
      passphrase_strength_label_ == nullptr) {
    return;
  }

  const auto strength = CalculatePassphraseStrength(text);
  const auto color = PassphraseStrengthColor(strength);

  passphrase_strength_bar_->setValue(strength);
  passphrase_strength_bar_->setStyleSheet(
      QStringLiteral(
          "QProgressBar { border: none; border-radius: 4px; "
          "background-color: rgba(128, 128, 128, 60); }"
          "QProgressBar::chunk { border-radius: 4px; background-color: %1; }")
          .arg(color));

  passphrase_strength_label_->setStyleSheet(
      QStringLiteral("color: %1; font-weight: 600;").arg(color));
  passphrase_strength_label_->setText(PassphraseStrengthDescription(strength));
}

}  // namespace GpgFrontend::UI