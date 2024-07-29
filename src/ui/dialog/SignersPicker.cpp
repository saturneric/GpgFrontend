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

#include "core/GpgModel.h"
#include "ui/widgets/KeyList.h"

namespace GpgFrontend::UI {

SignersPicker::SignersPicker(QWidget* parent)
    : GeneralDialog(typeid(SignersPicker).name(), parent) {
  auto* confirm_button = new QPushButton(tr("Confirm"));
  auto* cancel_button = new QPushButton(tr("Cancel"));

  connect(confirm_button, &QPushButton::clicked,
          [=]() { this->accepted_ = true; });
  connect(confirm_button, &QPushButton::clicked, this, &QDialog::accept);
  connect(cancel_button, &QPushButton::clicked, this, &QDialog::reject);

  /*Setup KeyList*/
  key_list_ =
      new KeyList(KeyMenuAbility::kCOLUMN_FILTER | KeyMenuAbility::kSEARCH_BAR,
                  GpgKeyTableColumn::kNAME | GpgKeyTableColumn::kEMAIL_ADDRESS |
                      GpgKeyTableColumn::kKEY_ID | GpgKeyTableColumn::kUSAGE,
                  this);
  key_list_->AddListGroupTab(tr("Signers"), "signers",
                             GpgKeyTableDisplayMode::kPRIVATE_KEY,
                             [](const GpgKey& key) -> bool {
                               return key.IsHasActualSigningCapability();
                             });
  key_list_->SlotRefresh();

  auto* vbox2 = new QVBoxLayout();
  vbox2->addWidget(new QLabel(tr("Select Signer(s)") + ": "));
  vbox2->addWidget(key_list_);
  vbox2->addWidget(new QLabel(
      tr("Please select one or more private keys you use for signing.") + "\n" +
      tr("If no key is selected, the default key will be used for signing.")));
  vbox2->addWidget(confirm_button);
  vbox2->addWidget(cancel_button);
  vbox2->addStretch(0);
  setLayout(vbox2);

  this->setWindowFlags(Qt::Window | Qt::WindowTitleHint |
                       Qt::CustomizeWindowHint);

  this->setModal(true);
  this->setWindowTitle(tr("Signers Picker"));
  this->setMinimumWidth(480);

  movePosition2CenterOfParent();
  this->show();
}

auto SignersPicker::GetCheckedSigners() -> GpgFrontend::KeyIdArgsListPtr {
  return key_list_->GetCheckedPrivateKey();
}

auto SignersPicker::GetStatus() const -> bool { return this->accepted_; }

}  // namespace GpgFrontend::UI
