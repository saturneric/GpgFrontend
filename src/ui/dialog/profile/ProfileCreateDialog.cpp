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

#include "ui/dialog/profile/ProfileCreateDialog.h"

#include "core/function/ProfileBootstrap.h"

namespace GpgFrontend::UI {

namespace {

/**
 * @brief A grey explanatory label that is allowed to be more than one line.
 *
 * A word-wrapped QLabel reports the height of a single line unless it is asked
 * to declare heightForWidth, so a dialog sized from its layout comes out too
 * short and the text is clipped into whatever sits below it.
 *
 * @param text the explanation
 * @param parent parent widget
 * @return the label
 */
auto MakeHintLabel(const QString& text, QWidget* parent) -> QLabel* {
  auto* label = new QLabel(text, parent);
  label->setWordWrap(true);
  label->setStyleSheet("color: gray;");

  auto policy = label->sizePolicy();
  policy.setHeightForWidth(true);
  policy.setVerticalPolicy(QSizePolicy::MinimumExpanding);
  label->setSizePolicy(policy);
  return label;
}

}  // namespace

ProfileCreateDialog::ProfileCreateDialog(QStringList taken_ids, QWidget* parent)
    : GeneralDialog("profile_create_dialog", parent),
      taken_ids_(std::move(taken_ids)) {
  init_ui();
  setWindowTitle(tr("New Profile"));
  setModal(true);

  // GeneralDialog deletes itself on close. This dialog is read *after*
  // exec() returns — it exists to hand back a name and a keyring choice —
  // so that would free it while the caller is still using it.
  setAttribute(Qt::WA_DeleteOnClose, false);

  movePosition2CenterOfParent();
}

void ProfileCreateDialog::init_ui() {
  auto* layout = new QVBoxLayout(this);

  // The explanations below are word-wrapped, and a word-wrapped label asks for
  // almost no width and however much height that implies. Left to itself the
  // dialog opens as a tall ribbon with the text running into the controls, so
  // the width is stated once here and everything wraps against it.
  setMinimumWidth(460);

  auto* form = new QFormLayout();
  name_edit_ = new QLineEdit(this);
  name_edit_->setPlaceholderText(tr("for example: Work"));
  form->addRow(tr("Name"), name_edit_);

  id_label_ = new QLabel(this);
  id_label_->setStyleSheet("color: gray;");
  form->addRow(tr("Folder"), id_label_);
  layout->addLayout(form);

  auto* keyring = new QGroupBox(tr("Keys"), this);
  auto* keyring_layout = new QVBoxLayout(keyring);

  own_keyring_ = new QRadioButton(tr("Start with an empty keyring"), keyring);
  own_keyring_->setChecked(true);
  auto* own_hint = MakeHintLabel(
      tr("Keys live inside this profile. It stays separate from everything "
         "else and can be exported as a single file. It opens with no keys, "
         "so you will import or create them here."),
      keyring);

  system_keyring_ =
      new QRadioButton(tr("Use the system GnuPG keyring"), keyring);
  auto* system_hint = MakeHintLabel(
      tr("The same keys you already have. They are shared with the rest of "
         "the system, so they are not really separate and cannot be carried "
         "inside a profile file."),
      keyring);

  keyring_layout->addWidget(own_keyring_);
  keyring_layout->addWidget(own_hint);
  keyring_layout->addSpacing(8);
  keyring_layout->addWidget(system_keyring_);
  keyring_layout->addWidget(system_hint);
  layout->addWidget(keyring);

  buttons_ = new QDialogButtonBox(
      QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
  layout->addWidget(buttons_);

  connect(name_edit_, &QLineEdit::textChanged, this,
          &ProfileCreateDialog::slot_name_changed);
  connect(buttons_, &QDialogButtonBox::accepted, this,
          &ProfileCreateDialog::slot_accept);
  connect(buttons_, &QDialogButtonBox::rejected, this,
          &ProfileCreateDialog::reject);

  slot_name_changed();
}

void ProfileCreateDialog::slot_name_changed() {
  id_ = MakeProfileId(name_edit_->text());

  // Say what will actually be created before it is created: the id becomes a
  // directory name, and a name that produces nothing usable is better caught
  // while typing than on accept.
  if (id_.isEmpty()) {
    id_label_->setText(tr("— enter a name —"));
  } else if (taken_ids_.contains(id_)) {
    id_label_->setText(tr("%1  (already in use)").arg(id_));
  } else {
    id_label_->setText(id_);
  }

  buttons_->button(QDialogButtonBox::Ok)
      ->setEnabled(!id_.isEmpty() && !taken_ids_.contains(id_));
}

void ProfileCreateDialog::slot_accept() {
  if (id_.isEmpty() || taken_ids_.contains(id_)) return;
  accept();
}

auto ProfileCreateDialog::DisplayName() const -> QString {
  const auto name = name_edit_->text().trimmed();
  return name.isEmpty() ? id_ : name;
}

auto ProfileCreateDialog::SelfContained() const -> bool {
  return own_keyring_->isChecked();
}

}  // namespace GpgFrontend::UI
