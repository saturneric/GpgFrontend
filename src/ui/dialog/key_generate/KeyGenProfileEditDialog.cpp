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

#include "ui/dialog/key_generate/KeyGenProfileEditDialog.h"

#include "core/function/openpgp/support/KeyManagementOpSupport.h"
#include "core/utils/CommonUtils.h"
#include "ui/function/KeyGenerateHelper.h"
#include "ui/function/UIStyle.h"
#include "ui/widgets/KeyAlgoEditor.h"

namespace GpgFrontend::UI {

KeyGenProfileEditDialog::KeyGenProfileEditDialog(
    int channel, const ResolvedKeyGenProfile& resolved,
    QList<KeyGenProfile> others, QWidget* parent)
    : GeneralDialog(typeid(KeyGenProfileEditDialog).name(), parent),
      channel_(channel),
      others_(std::move(others)),
      base_(resolved.profile),
      key_info_(SecureCreateSharedObject<KeyGenerateInfo>()),
      subkey_algos_(KeyGenerateInfo::GetSupportedSubkeyAlgo(channel)),
      name_edit_(new QLineEdit(this)),
      description_edit_(new QLineEdit(this)),
      name_problem_(CreateDetailLabel({}, this)),
      primary_editor_(new KeyAlgoEditor(
          channel, KeyGenerateInfo::GetSupportedKeyAlgo(channel), this)),
      subkey_check_(new QCheckBox(tr("Add a subkey"), this)),
      subkey_editor_(new KeyAlgoEditor(channel, subkey_algos_, this)),
      ok_button_(nullptr) {
  // Run with exec() by its owner, which holds it on the stack and reads the
  // edited profile afterwards; GeneralDialog's delete-on-close would free it a
  // second time.
  setAttribute(Qt::WA_DeleteOnClose, false);
  setWindowTitle(base_.name.isEmpty() ? tr("New Profile") : tr("Edit Profile"));
  // Wide enough for a whole default name and a sentence of description.
  setMinimumWidth(540);

  ApplyKeyGenProfile(resolved, CurrentDateTime(), *key_info_, subkey_info_);

  name_edit_->setText(base_.name);
  name_edit_->setMaxLength(kKeyGenProfileNameMaxLen);
  name_edit_->setClearButtonEnabled(true);
  description_edit_->setText(base_.description);
  description_edit_->setMaxLength(kKeyGenProfileDescriptionMaxLen);
  description_edit_->setPlaceholderText(
      tr("optional: what the profile is for"));
  SetLabelTextColor(name_problem_, WarningColor(name_problem_->palette()));
  name_problem_->hide();

  auto* about = new QWidget(this);
  auto* about_form = new QFormLayout(about);
  about_form->setContentsMargins(0, 0, 0, 0);
  about_form->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
  auto* name_cell = new QWidget(about);
  auto* name_layout = new QVBoxLayout(name_cell);
  name_layout->setContentsMargins(0, 0, 0, 0);
  name_layout->setSpacing(2);
  name_layout->addWidget(name_edit_);
  name_layout->addWidget(name_problem_);
  about_form->addRow(tr("Name"), name_cell);
  about_form->addRow(tr("Description"), description_edit_);

  const auto expiry = IsOpSupported<SetExpireOpTag>(channel);
  primary_editor_->SetExpiryVisible(expiry);
  primary_editor_->SetInfo(key_info_);

  auto* subkey = new QWidget(this);
  auto* subkey_layout = new QVBoxLayout(subkey);
  subkey_layout->setContentsMargins(0, 0, 0, 0);
  subkey_layout->addWidget(subkey_check_);
  subkey_layout->addWidget(subkey_editor_);
  subkey_editor_->SetExpiryVisible(expiry);
  subkey_check_->setChecked(subkey_info_ != nullptr);
  subkey_editor_->SetInfo(subkey_info_);
  subkey_editor_->setVisible(subkey_info_ != nullptr);

  auto* buttons =
      new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
  ok_button_ = buttons->button(QDialogButtonBox::Ok);

  auto* layout = new QVBoxLayout(this);
  layout->addWidget(CreateCard(tr("Profile"), about, this));
  layout->addWidget(CreateCard(tr("Primary key"), primary_editor_, this));
  layout->addWidget(CreateCard(tr("Subkey"), subkey, this));
  layout->addStretch(1);
  layout->addWidget(buttons);

  connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
  connect(name_edit_, &QLineEdit::textChanged, this,
          &KeyGenProfileEditDialog::refresh_ok);
  connect(primary_editor_, &KeyAlgoEditor::SignalChanged, this,
          &KeyGenProfileEditDialog::refresh_ok);
  connect(subkey_editor_, &KeyAlgoEditor::SignalChanged, this,
          &KeyGenProfileEditDialog::refresh_ok);
  connect(subkey_check_, &QCheckBox::toggled, this, [this](bool on) {
    if (on && subkey_info_ == nullptr) {
      subkey_info_ = SecureCreateSharedObject<KeyGenerateInfo>(true);
      subkey_info_->SetAlgo(SuggestSubkeyAlgo(SubkeyPurpose::kEncrypt,
                                              key_info_->GetAlgo().Id(),
                                              subkey_algos_));
      subkey_info_->SetExpireTime(key_info_->GetExpireTime());
      subkey_info_->SetNonExpired(key_info_->IsNonExpired());
    } else if (!on) {
      subkey_info_ = nullptr;
    }
    subkey_editor_->SetInfo(subkey_info_);
    subkey_editor_->setVisible(on);
    refresh_ok();
  });

  refresh_ok();
  name_edit_->setFocus();
}

auto KeyGenProfileEditDialog::Profile() const -> KeyGenProfile {
  auto profile =
      KeyGenProfileFromInfo(*key_info_, subkey_info_, CurrentDateTime());
  profile.name = name_edit_->text().trimmed();
  profile.description = description_edit_->text().trimmed();
  profile.hidden = base_.hidden;
  return profile;
}

void KeyGenProfileEditDialog::refresh_ok() {
  const auto problem = CheckKeyGenProfileName(name_edit_->text(), others_);
  // Blank is only a problem to be fixed, not one worth a red line while the
  // name is still being typed.
  const auto shown = problem == KeyGenProfileNameProblem::kEmpty
                         ? QString()
                         : DescribeKeyGenProfileNameProblem(problem);
  name_problem_->setText(shown);
  name_problem_->setVisible(!shown.isEmpty());

  ok_button_->setEnabled(
      problem == KeyGenProfileNameProblem::kNone &&
      primary_editor_->IsComplete() &&
      (subkey_info_ == nullptr || subkey_editor_->IsComplete()));
}

}  // namespace GpgFrontend::UI
