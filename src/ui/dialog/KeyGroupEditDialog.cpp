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

#include "KeyGroupEditDialog.h"

#include "ui/dialog/KeyGroupMetadataRules.h"

namespace GpgFrontend::UI {

KeyGroupEditDialog::KeyGroupEditDialog(const QString& name,
                                       const QString& email,
                                       const QString& comment, QWidget* parent)
    : GeneralDialog("KeyGroupEditDialog", parent),
      name_(new QLineEdit(name, this)),
      email_(new QLineEdit(email, this)),
      comment_(new QLineEdit(comment, this)),
      error_label_(new QLabel(this)),
      save_button_(nullptr) {
#ifdef Q_OS_MACOS
  setWindowFlag(Qt::WindowContextHelpButtonHint, false);
#endif

  setWindowTitle(tr("Edit Key Group"));
  setModal(true);
  setMinimumWidth(460);

  name_->setMinimumWidth(240);
  email_->setMinimumWidth(240);
  comment_->setMinimumWidth(240);

  auto* title_label = new QLabel(tr("Edit Key Group Details"), this);
  auto title_font = title_label->font();
  title_font.setBold(true);
  title_label->setFont(title_font);

  auto* desc_label = new QLabel(
      tr("These details only name the group. Changing them does not affect "
         "which keys belong to it."),
      this);
  desc_label->setWordWrap(true);

  auto tips_palette = desc_label->palette();
  tips_palette.setColor(
      QPalette::WindowText,
      palette().color(QPalette::Disabled, QPalette::WindowText));
  desc_label->setPalette(tips_palette);

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
      QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
  save_button_ = button_box->button(QDialogButtonBox::Save);
  save_button_->setText(tr("Save"));
  button_box->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));

  connect(button_box, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(button_box, &QDialogButtonBox::rejected, this, &QDialog::reject);

  connect(name_, &QLineEdit::textChanged, this,
          &KeyGroupEditDialog::update_validation_state);
  connect(email_, &QLineEdit::textChanged, this,
          &KeyGroupEditDialog::update_validation_state);

  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(10, 10, 10, 10);
  layout->setSpacing(8);
  layout->addWidget(title_label);
  layout->addWidget(desc_label);
  layout->addLayout(form_layout);
  layout->addWidget(error_label_);
  layout->addWidget(button_box);

  setAttribute(Qt::WA_DeleteOnClose, false);

  update_validation_state();
}

auto KeyGroupEditDialog::Name() const -> QString {
  return name_->text().trimmed();
}

auto KeyGroupEditDialog::Email() const -> QString {
  return email_->text().trimmed();
}

auto KeyGroupEditDialog::Comment() const -> QString {
  return comment_->text().trimmed();
}

void KeyGroupEditDialog::update_validation_state() {
  const auto problem = ValidateKeyGroupMetadata(name_->text(), email_->text());

  error_label_->setText(DescribeKeyGroupMetadataProblem(problem));
  if (save_button_ != nullptr) {
    save_button_->setEnabled(problem == KeyGroupMetadataProblem::kNone);
  }
}

void KeyGroupEditDialog::showEvent(QShowEvent* event) {
  GeneralDialog::showEvent(event);
  if (!isRectRestored()) movePosition2CenterOfParent();
}

}  // namespace GpgFrontend::UI
