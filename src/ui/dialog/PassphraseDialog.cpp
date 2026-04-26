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
    detail_text += tr("\nThis passphrase will be used to set a new password.");
  }

  auto key = ctx_->GetKey();
  if (key != nullptr) {
    detail_text += tr("\nKey ID: %1").arg(key->ID());
    detail_text += tr("\nKey UID: %1").arg(key->UID());
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

  password_row_layout->addWidget(password_edit_, 1);
  password_row_layout->addWidget(show_password_checkbox_);

  form_layout->addRow(tr("Passphrase:"), password_row);

  if (ctx_->ShouldConfirm()) {
    form_layout->addRow(tr("Confirm:"), confirm_password_edit_);
  } else {
    confirm_password_edit_->hide();
  }

  auto* button_layout = new QHBoxLayout();
  button_layout->setContentsMargins(0, 6, 0, 0);
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

  password_edit_->setFocus();

  adjustSize();
  resize(std::max(width(), 520), height());
}

[[nodiscard]] auto PassphraseDialog::Passphrase() const -> QString {
  return password_edit_->text();
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

}  // namespace GpgFrontend::UI