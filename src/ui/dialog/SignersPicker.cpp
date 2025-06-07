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
  auto* confirm_button = new QPushButton(tr("Confirm"));
  auto* cancel_button = new QPushButton(tr("Cancel"));

  confirm_button->setDisabled(true);

  connect(confirm_button, &QPushButton::clicked, this, &QDialog::accept);
  connect(cancel_button, &QPushButton::clicked, this, &QDialog::reject);

  /*Setup KeyList*/
  key_list_ = new KeyList(
      channel, KeyMenuAbility::kCOLUMN_FILTER | KeyMenuAbility::kSEARCH_BAR,
      GpgKeyTableColumn::kNAME | GpgKeyTableColumn::kEMAIL_ADDRESS |
          GpgKeyTableColumn::kKEY_ID | GpgKeyTableColumn::kUSAGE,
      this);
  key_list_->AddListGroupTab(
      tr("Signers"), "signers", GpgKeyTableDisplayMode::kPRIVATE_KEY,
      [](const GpgAbstractKey* key) -> bool { return key->IsHasSignCap(); });
  key_list_->SlotRefresh();

  connect(key_list_, &KeyList::SignalKeyChecked, this, [=]() {
    confirm_button->setDisabled(GetCheckedSigners().isEmpty());
  });

  auto* vbox2 = new QVBoxLayout();
  vbox2->addWidget(new QLabel(tr("Select Signer(s)") + ": "));
  vbox2->addWidget(key_list_, 1);

  auto* tips_label = new QLabel(
      tr("Please select one or more private keys you use for signing."));
  vbox2->addWidget(tips_label);
  tips_label->setStyleSheet("color: #666; margin-bottom: 6px;");

  auto* btn_layout = new QHBoxLayout();
  btn_layout->addStretch();
  btn_layout->addWidget(confirm_button);
  btn_layout->addWidget(cancel_button);
  vbox2->addLayout(btn_layout);
  vbox2->addStretch(0);
  setLayout(vbox2);

  this->setWindowFlags(Qt::Window | Qt::WindowTitleHint |
                       Qt::CustomizeWindowHint);

  this->setModal(true);
  this->setWindowTitle(tr("Signers Picker"));
  this->setMinimumWidth(480);

  movePosition2CenterOfParent();

  this->show();
  this->raise();
  this->activateWindow();

  // should not delete itself at closing by default
  setAttribute(Qt::WA_DeleteOnClose, false);
}

auto SignersPicker::GetCheckedSigners() -> GpgAbstractKeyPtrList {
  return key_list_->GetCheckedPrivateKey();
}

}  // namespace GpgFrontend::UI
