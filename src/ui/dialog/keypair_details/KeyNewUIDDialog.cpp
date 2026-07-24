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

#include "KeyNewUIDDialog.h"

#include "core/function/openpgp/UserIdOperation.h"
#include "core/utils/CommonUtils.h"
#include "core/utils/GpgUtils.h"
#include "ui/UISignalStation.h"
#include "ui/UserInterfaceUtils.h"
#include "ui/function/GpgOperaHelper.h"

namespace GpgFrontend::UI {
KeyNewUIDDialog::KeyNewUIDDialog(int channel, GpgKeyPtr key, QWidget* parent)
    : GeneralDialog(typeid(KeyNewUIDDialog).name(), parent),
      current_gpg_context_channel_(channel),
      m_key_(std::move(key)) {
  assert(m_key_ != nullptr);

  auto* title_label = new QLabel(tr("Add a new User ID"));
  QFont title_font = title_label->font();
  title_font.setBold(true);
  title_font.setPointSize(title_font.pointSize() + 1);
  title_label->setFont(title_font);

  auto* hint_label = new QLabel(
      tr("A User ID pairs a name with an optional email and comment. The name "
         "is required."));
  hint_label->setWordWrap(true);

  name_ = new QLineEdit();
  name_->setClearButtonEnabled(true);
  name_->setPlaceholderText(tr("Full name"));
  email_ = new QLineEdit();
  email_->setClearButtonEnabled(true);
  email_->setPlaceholderText(tr("name@example.com"));
  comment_ = new QLineEdit();
  comment_->setClearButtonEnabled(true);
  comment_->setPlaceholderText(tr("Optional comment"));

  auto* form_layout = new QFormLayout();
  form_layout->setContentsMargins(0, 0, 0, 0);
  form_layout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
  form_layout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
  form_layout->addRow(tr("Name"), name_);
  form_layout->addRow(tr("Email"), email_);
  form_layout->addRow(tr("Comment"), comment_);

  summary_label_ = new QLabel();
  summary_label_->setWordWrap(true);

  button_box_ =
      new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  button_box_->button(QDialogButtonBox::Ok)->setText(tr("Create"));

  auto* header_layout = new QVBoxLayout();
  header_layout->setSpacing(4);
  header_layout->addWidget(title_label);
  header_layout->addWidget(hint_label);

  auto engine =
      OpenPGPContext::GetInstance(current_gpg_context_channel_).Engine();
  if (engine == OpenPGPEngine::kGNUPG) {
    auto* primary_notice =
        new QLabel(tr("The new User ID will be set as the primary User ID."));
    primary_notice->setWordWrap(true);
    header_layout->addWidget(primary_notice);
  }

  auto* layout = new QVBoxLayout();
  layout->setContentsMargins(18, 18, 18, 18);
  layout->setSpacing(12);
  layout->addLayout(header_layout);
  layout->addLayout(form_layout);
  layout->addWidget(summary_label_);
  layout->addWidget(button_box_);

  connect(name_, &QLineEdit::textChanged, this,
          &KeyNewUIDDialog::refresh_widgets_state);
  connect(email_, &QLineEdit::textChanged, this,
          &KeyNewUIDDialog::refresh_widgets_state);
  connect(comment_, &QLineEdit::textChanged, this,
          &KeyNewUIDDialog::refresh_widgets_state);
  connect(button_box_, &QDialogButtonBox::accepted, this,
          &KeyNewUIDDialog::slot_create_new_uid);
  connect(button_box_, &QDialogButtonBox::rejected, this,
          &KeyNewUIDDialog::reject);

  this->setLayout(layout);
  this->setMinimumWidth(460);
  this->setWindowTitle(tr("Create New UID"));
  this->setAttribute(Qt::WA_DeleteOnClose, true);
  this->setModal(true);

  connect(this, &KeyNewUIDDialog::SignalUIDCreated,
          UISignalStation::GetInstance(),
          &UISignalStation::SignalKeyDatabaseRefresh);

  refresh_widgets_state();
  name_->setFocus();
}

void KeyNewUIDDialog::refresh_widgets_state() {
  const auto name = name_->text().trimmed();
  const auto email = email_->text().trimmed();
  const auto comment = comment_->text().trimmed();

  QString message;
  bool valid = true;

  // A user id needs a name; its length is only advisory (handled at submit).
  if (name.isEmpty()) {
    valid = false;
    message = tr("Enter a name for the User ID.");
    // The name and comment become part of an RFC 2822 mail name-addr
    // ("Name (Comment) <email>"); reject the structural delimiters '(', ')',
    // '<', '>' and control characters or the resulting UID would be malformed.
  } else if (!IsValidUserIdComponent(name) ||
             !IsValidUserIdComponent(comment)) {
    valid = false;
    message = tr(
        "Name and comment must not contain the characters '(', ')', '<', '>' "
        "or control characters.");
    // The email address is optional, but if one is given it must be usable.
  } else if (!email.isEmpty() && !IsEmailAddress(email)) {
    valid = false;
    message = tr("Please give a valid email address.");
  } else {
    message = tr("Ready to create the User ID.");
  }

  auto palette = summary_label_->palette();
  palette.setColor(summary_label_->foregroundRole(),
                   valid ? Qt::darkGreen : Qt::red);
  summary_label_->setPalette(palette);
  summary_label_->setText(message);

  button_box_->button(QDialogButtonBox::Ok)->setEnabled(valid);
}

void KeyNewUIDDialog::slot_create_new_uid() {
  const auto name = name_->text().trimmed();
  const auto email = email_->text().trimmed();
  const auto comment = comment_->text().trimmed();

  // Guard again: the Create button is only enabled when input is valid, but a
  // programmatic accept() could still land here on invalid input.
  refresh_widgets_state();
  if (!button_box_->button(QDialogButtonBox::Ok)->isEnabled()) return;

  if (!ConfirmShortUserIdName(this, name)) return;

  auto f = [this, name, comment, email](const OperaWaitingHd& hd) {
    UserIdOperation::GetInstance(current_gpg_context_channel_)
        .AddUID(m_key_, name, comment, email,
                [this, hd](GpgError err, const DataObjectPtr&) {
                  // stop showing the waiting dialog
                  hd();

                  if (CheckGpgError(err) == GPG_ERR_NO_ERROR) {
                    emit SignalUIDCreated();

                    // Present the notice over the parent window and close the
                    // form first, so it never stacks above a dialog that is
                    // about to vanish.
                    auto* msg_box =
                        new QMessageBox(qobject_cast<QWidget*>(this->parent()));
                    msg_box->setAttribute(Qt::WA_DeleteOnClose);
                    msg_box->setStandardButtons(QMessageBox::Ok);
                    msg_box->setIcon(QMessageBox::Information);
                    msg_box->setWindowTitle(tr("Successful Operation"));
                    msg_box->setText(tr("Successfully added a new UID."));
                    msg_box->setModal(true);
                    msg_box->open();

                    this->close();
                  } else {
                    // Keep the dialog open so the user can retry.
                    QMessageBox::critical(
                        this, tr("Operation Failed"),
                        tr("An error occurred during the operation."));
                  }
                });
  };
  GpgOperaHelper::WaitForOpera(this, tr("Creating UID"), f);
}

}  // namespace GpgFrontend::UI
