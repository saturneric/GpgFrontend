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

#include "SignersPicker.h"

#include "ui/widgets/KeyList.h"

namespace GpgFrontend::UI {

SignersPicker::SignersPicker(int channel, QWidget* parent)
    : GeneralDialog(typeid(SignersPicker).name(), parent) {
#ifdef Q_OS_MACOS
  setWindowFlag(Qt::WindowContextHelpButtonHint, false);
#endif

  setModal(true);
  setWindowTitle(tr("Signers Picker"));
  setMinimumSize(560, 380);
  resize(680, 460);

  auto* title_label = new QLabel(tr("Select Signer(s)"));
  auto title_font = title_label->font();
  title_font.setBold(true);
  title_label->setFont(title_font);

  auto* tips_label = new QLabel(
      tr("Please select one or more private keys that should be used for "
         "signing."));
  tips_label->setWordWrap(true);

  auto tips_palette = tips_label->palette();
  tips_palette.setColor(
      QPalette::WindowText,
      palette().color(QPalette::Disabled, QPalette::WindowText));
  tips_label->setPalette(tips_palette);

  key_list_ = new KeyList(
      channel, KeyMenuAbility::kCOLUMN_FILTER | KeyMenuAbility::kSEARCH_BAR,
      GpgKeyTableColumn::kNAME | GpgKeyTableColumn::kEMAIL_ADDRESS |
          GpgKeyTableColumn::kKEY_ID | GpgKeyTableColumn::kUSAGE,
      this);

  key_list_->AddListGroupTab(tr("Signers"), "signers",
                             GpgKeyTableDisplayMode::kPRIVATE_KEY,
                             [](const GpgAbstractKey* key) -> bool {
                               return key != nullptr && key->IsHasSignCap();
                             });

  key_list_->SlotRefresh();

  auto* button_box = new QDialogButtonBox(
      QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);

  confirm_button_ = button_box->button(QDialogButtonBox::Ok);
  confirm_button_->setText(tr("Confirm"));
  confirm_button_->setEnabled(false);

  auto* cancel_button = button_box->button(QDialogButtonBox::Cancel);
  cancel_button->setText(tr("Cancel"));

  connect(button_box, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(button_box, &QDialogButtonBox::rejected, this, &QDialog::reject);

  connect(key_list_, &KeyList::SignalKeyChecked, this,
          &SignersPicker::update_confirm_button_state);

  auto* main_layout = new QVBoxLayout(this);
  main_layout->setContentsMargins(10, 10, 10, 10);
  main_layout->setSpacing(8);

  main_layout->addWidget(title_label);
  main_layout->addWidget(tips_label);
  main_layout->addWidget(key_list_, 1);
  main_layout->addWidget(button_box);

  setAttribute(Qt::WA_DeleteOnClose, false);

  update_confirm_button_state();

  show();
  raise();
  activateWindow();
}

auto SignersPicker::GetCheckedSigners() -> GpgAbstractKeyPtrList {
  if (key_list_ == nullptr) return {};
  return key_list_->GetCheckedPrivateKey();
}

void SignersPicker::update_confirm_button_state() {
  if (confirm_button_ == nullptr) return;
  confirm_button_->setEnabled(!GetCheckedSigners().isEmpty());
}

auto SignersPicker::showEvent(QShowEvent* event) -> void {
  GeneralDialog::showEvent(event);

  if (!isRectRestored()) {
    movePosition2CenterOfParent();
  }
}

}  // namespace GpgFrontend::UI