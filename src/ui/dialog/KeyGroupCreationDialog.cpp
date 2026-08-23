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

#include "KeyGroupCreationDialog.h"

#include "core/function/openpgp/KeyGroupRepository.h"
#include "core/model/GpgKeyGroup.h"
#include "core/utils/CommonUtils.h"
#include "ui/UISignalStation.h"
#include "ui/dialog/KeyGroupMetadataRules.h"

namespace GpgFrontend::UI {
KeyGroupCreationDialog::KeyGroupCreationDialog(int channel, QStringList key_ids,
                                               QWidget* parent)
    : GeneralDialog("KeyGroupCreationDialog", parent),
      current_gpg_context_channel_(channel),
      key_ids_(std::move(key_ids)) {
  assert(!key_ids_.isEmpty());

#ifdef Q_OS_MACOS
  setWindowFlag(Qt::WindowContextHelpButtonHint, false);
#endif

  setWindowTitle(tr("New Key Group"));
  setModal(true);
  setMinimumWidth(460);

  name_ = new QLineEdit(this);
  name_->setMinimumWidth(240);
  name_->setPlaceholderText(tr("Engineering Team"));
  email_ = new QLineEdit(this);
  email_->setMinimumWidth(240);
  email_->setPlaceholderText(tr("Optional"));
  comment_ = new QLineEdit(this);
  comment_->setMinimumWidth(240);
  comment_->setPlaceholderText(tr("Optional"));

  auto* title_label = new QLabel(tr("Create a Key Group"), this);
  auto title_font = title_label->font();
  title_font.setBold(true);
  title_label->setFont(title_font);

  auto* desc_label = new QLabel(
      tr("Encrypting to a key group encrypts to every key it contains."), this);
  desc_label->setWordWrap(true);

  auto hint_palette = desc_label->palette();
  hint_palette.setColor(
      QPalette::WindowText,
      palette().color(QPalette::Disabled, QPalette::WindowText));
  desc_label->setPalette(hint_palette);

  auto* members_label = new QLabel(
      DescribeKeyGroupCreation(static_cast<int>(key_ids_.size())), this);
  members_label->setWordWrap(true);

  error_label_ = new QLabel(this);
  error_label_->setWordWrap(true);
  auto error_palette = error_label_->palette();
  error_palette.setColor(QPalette::WindowText, QColor("#d33"));
  error_label_->setPalette(error_palette);

  auto* form_layout = new QGridLayout();
  form_layout->addWidget(new QLabel(tr("Name"), this), 0, 0);
  form_layout->addWidget(name_, 0, 1);
  form_layout->addWidget(new QLabel(tr("Email"), this), 1, 0);
  form_layout->addWidget(email_, 1, 1);
  form_layout->addWidget(new QLabel(tr("Comment"), this), 2, 0);
  form_layout->addWidget(comment_, 2, 1);
  form_layout->setColumnStretch(1, 1);

  auto* button_box = new QDialogButtonBox(
      QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
  create_button_ = button_box->button(QDialogButtonBox::Ok);
  create_button_->setText(tr("Create"));
  button_box->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));

  connect(button_box, &QDialogButtonBox::accepted, this,
          &KeyGroupCreationDialog::slot_create_new_uid);
  connect(button_box, &QDialogButtonBox::rejected, this, &QDialog::reject);

  connect(name_, &QLineEdit::textChanged, this,
          &KeyGroupCreationDialog::update_validation_state);
  connect(email_, &QLineEdit::textChanged, this,
          &KeyGroupCreationDialog::update_validation_state);

  connect(this, &KeyGroupCreationDialog::SignalCreated,
          UISignalStation::GetInstance(),
          &UISignalStation::SignalKeyDatabaseRefresh);

  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(10, 10, 10, 10);
  layout->setSpacing(8);
  layout->addWidget(title_label);
  layout->addWidget(desc_label);
  layout->addLayout(form_layout);
  layout->addWidget(members_label);
  layout->addWidget(error_label_);
  layout->addWidget(button_box);

  this->setAttribute(Qt::WA_DeleteOnClose, true);

  update_validation_state();
  name_->setFocus();
}

void KeyGroupCreationDialog::update_validation_state() {
  const auto problem = ValidateKeyGroupMetadata(name_->text(), email_->text());

  // Stay quiet until there is something to type into: an error shown before
  // the first keystroke reads as a complaint about opening the dialog.
  error_label_->setText(name_->text().isEmpty()
                            ? QString{}
                            : DescribeKeyGroupMetadataProblem(problem));

  if (create_button_ != nullptr) {
    create_button_->setEnabled(problem == KeyGroupMetadataProblem::kNone);
  }
}

void KeyGroupCreationDialog::showEvent(QShowEvent* event) {
  GeneralDialog::showEvent(event);
  if (!isRectRestored()) movePosition2CenterOfParent();
}

void KeyGroupCreationDialog::slot_create_new_uid() {
  // Shared with KeyGroupEditDialog so the two forms cannot disagree about what
  // a valid key group is.
  const auto problem = ValidateKeyGroupMetadata(name_->text(), email_->text());
  if (problem != KeyGroupMetadataProblem::kNone) {
    error_label_->setText(DescribeKeyGroupMetadataProblem(problem));
    return;
  }

  auto p_kg = GpgKeyGroup{name_->text().trimmed(), email_->text().trimmed(),
                          comment_->text().trimmed(), key_ids_};
  KeyGroupRepository::GetInstance(current_gpg_context_channel_)
      .AddKeyGroup(p_kg);

  emit SignalCreated();
  this->close();
}

}  // namespace GpgFrontend::UI
