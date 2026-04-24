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
  resize(460, 220);
  setMinimumWidth(420);

  auto* main_layout = new QVBoxLayout(this);
  main_layout->setContentsMargins(20, 20, 20, 20);
  main_layout->setSpacing(12);

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
  auto* details_layout = new QVBoxLayout(details_frame);
  details_layout->setContentsMargins(12, 10, 12, 10);
  details_layout->setSpacing(6);

  QString detail_text =
      tr("Passphrase info: %1").arg(ctx_->GetPassphraseInfo());

  if (ctx->IsAskForNew()) {
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

  auto* pwd_label = new QLabel(tr("Passphrase"), this);

  auto* pwd_layout = new QHBoxLayout();
  pwd_layout->setSpacing(8);

  password_edit_ = new QLineEdit(this);
  password_edit_->setEchoMode(QLineEdit::Password);
  password_edit_->setPlaceholderText(tr("Enter your passphrase here"));

  show_password_checkbox_ = new QCheckBox(tr("Show"), this);
  connect(show_password_checkbox_, &QCheckBox::toggled, this,
          [this](bool checked) {
            password_edit_->setEchoMode(checked ? QLineEdit::Normal
                                                : QLineEdit::Password);
          });

  pwd_layout->addWidget(password_edit_, 1);
  pwd_layout->addWidget(show_password_checkbox_);

  auto* button_layout = new QHBoxLayout();
  button_layout->addStretch();

  auto* ok_button = new QPushButton(tr("OK"), this);
  auto* cancel_button = new QPushButton(tr("Cancel"), this);

  ok_button->setDefault(true);
  ok_button->setAutoDefault(true);

  connect(ok_button, &QPushButton::clicked, this, [this]() { accept(); });
  connect(cancel_button, &QPushButton::clicked, this, [this]() { reject(); });

  button_layout->addWidget(cancel_button);
  button_layout->addWidget(ok_button);

  main_layout->addWidget(title_label);
  main_layout->addWidget(info_label);
  main_layout->addWidget(details_frame);
  main_layout->addWidget(pwd_label);
  main_layout->addLayout(pwd_layout);
  main_layout->addSpacing(6);
  main_layout->addLayout(button_layout);

  password_edit_->setFocus();
}

[[nodiscard]] auto PassphraseDialog::Passphrase() const -> QString {
  return password_edit_->text();
}

}  // namespace GpgFrontend::UI