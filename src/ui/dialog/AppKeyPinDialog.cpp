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

#include "AppKeyPinDialog.h"

#include "ui/dialog/PassphraseStrength.h"

namespace GpgFrontend::UI {

namespace {

/// Shortest PIN accepted when setting one. The strength meter stays advice, but
/// a floor keeps a two-character PIN from being offered as real protection.
constexpr int kMinPinLength = 8;

}  // namespace

AppKeyPinDialog::AppKeyPinDialog(Mode mode, QWidget* parent)
    : QDialog(parent), mode_(mode) {
  const bool asks_for_new = mode_ != Mode::kUNLOCK;

  setWindowTitle(asks_for_new ? tr("Set Application PIN")
                              : tr("Application PIN Required"));
  setModal(true);
  setMinimumWidth(460);

  auto* main_layout = new QVBoxLayout(this);

  auto* info_label = new QLabel(
      asks_for_new
          ? tr("This PIN encrypts the application key on disk. You will be "
               "asked for it every time the application starts.")
          : tr("The application key is protected by a PIN. Enter it to "
               "continue."),
      this);
  info_label->setWordWrap(true);
  main_layout->addWidget(info_label);

  auto* form_layout = new QFormLayout();
  form_layout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

  if (mode_ == Mode::kUNLOCK || mode_ == Mode::kCHANGE) {
    current_edit_ = new QLineEdit(this);
    current_edit_->setEchoMode(QLineEdit::Password);
    form_layout->addRow(
        mode_ == Mode::kCHANGE ? tr("Current PIN:") : tr("PIN:"),
        current_edit_);
  }

  if (asks_for_new) {
    new_edit_ = new QLineEdit(this);
    new_edit_->setEchoMode(QLineEdit::Password);
    form_layout->addRow(mode_ == Mode::kCHANGE ? tr("New PIN:") : tr("PIN:"),
                        new_edit_);

    confirm_edit_ = new QLineEdit(this);
    confirm_edit_->setEchoMode(QLineEdit::Password);
    form_layout->addRow(tr("Confirm:"), confirm_edit_);
  }

  auto* show_box = new QCheckBox(tr("Show"), this);
  form_layout->addRow(QString(), show_box);
  connect(show_box, &QCheckBox::toggled, this, [this](bool shown) {
    const auto echo = shown ? QLineEdit::Normal : QLineEdit::Password;
    for (auto* edit : {current_edit_, new_edit_, confirm_edit_}) {
      if (edit != nullptr) edit->setEchoMode(echo);
    }
  });

  main_layout->addLayout(form_layout);

  if (asks_for_new) {
    auto* strength_layout = new QHBoxLayout();
    strength_bar_ = new QProgressBar(this);
    strength_bar_->setRange(0, 100);
    strength_bar_->setTextVisible(false);
    strength_label_ = new QLabel(this);
    strength_layout->addWidget(new QLabel(tr("Strength:"), this));
    strength_layout->addWidget(strength_bar_, 1);
    strength_layout->addWidget(strength_label_);
    main_layout->addLayout(strength_layout);

    // The one thing a user must understand before choosing a PIN: there is no
    // recovery path, because the key is encrypted with it and nothing else.
    auto* warning_label = new QLabel(
        tr("If you forget this PIN, everything the application has encrypted "
           "becomes permanently unreadable. There is no recovery."),
        this);
    warning_label->setWordWrap(true);
    warning_label->setStyleSheet(QStringLiteral("color: #e53935;"));
    main_layout->addWidget(warning_label);
  }

  error_label_ = new QLabel(this);
  error_label_->setWordWrap(true);
  error_label_->setStyleSheet(QStringLiteral("color: #e53935;"));
  error_label_->setVisible(false);
  main_layout->addWidget(error_label_);

  auto* buttons = new QDialogButtonBox(
      QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
  accept_button_ = buttons->button(QDialogButtonBox::Ok);
  main_layout->addWidget(buttons);

  connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

  for (auto* edit : {current_edit_, new_edit_, confirm_edit_}) {
    if (edit == nullptr) continue;
    connect(edit, &QLineEdit::textChanged, this, [this]() {
      // Any edit means the previous failure no longer describes what is typed.
      error_label_->setVisible(false);
      refresh_strength();
      refresh_validity();
    });
  }

  refresh_strength();
  refresh_validity();

  if (current_edit_ != nullptr) {
    current_edit_->setFocus();
  } else if (new_edit_ != nullptr) {
    new_edit_->setFocus();
  }

  adjustSize();
}

auto AppKeyPinDialog::Pin() const -> GFBuffer {
  auto* source = mode_ == Mode::kUNLOCK ? current_edit_ : new_edit_;
  return source != nullptr ? GFBuffer(source->text()) : GFBuffer();
}

auto AppKeyPinDialog::CurrentPin() const -> GFBuffer {
  if (mode_ != Mode::kCHANGE || current_edit_ == nullptr) return {};
  return GFBuffer(current_edit_->text());
}

void AppKeyPinDialog::SetErrorText(const QString& text) {
  error_label_->setText(text);
  error_label_->setVisible(!text.isEmpty());
  adjustSize();
}

void AppKeyPinDialog::Clear() {
  for (auto* edit : {current_edit_, new_edit_, confirm_edit_}) {
    if (edit != nullptr) edit->clear();
  }
}

void AppKeyPinDialog::refresh_validity() {
  if (accept_button_ == nullptr) return;

  if (mode_ == Mode::kUNLOCK) {
    accept_button_->setEnabled(!current_edit_->text().isEmpty());
    return;
  }

  const auto pin = new_edit_->text();
  const bool long_enough = pin.size() >= kMinPinLength;
  const bool confirmed = pin == confirm_edit_->text();
  const bool has_current =
      mode_ != Mode::kCHANGE || !current_edit_->text().isEmpty();

  accept_button_->setEnabled(long_enough && confirmed && has_current);

  // Say which of the two conditions is unmet, but only once something has been
  // typed — an empty form is not yet a mistake.
  if (!pin.isEmpty() && !long_enough) {
    error_label_->setText(
        tr("The PIN must be at least %1 characters.").arg(kMinPinLength));
    error_label_->setVisible(true);
  } else if (long_enough && !confirm_edit_->text().isEmpty() && !confirmed) {
    error_label_->setText(tr("The two PINs do not match."));
    error_label_->setVisible(true);
  }
}

void AppKeyPinDialog::refresh_strength() {
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

}  // namespace GpgFrontend::UI
