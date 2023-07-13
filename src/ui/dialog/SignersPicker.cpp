/*
 * Copyright (c) 2022. Saturneric
 *
 *  This file is part of GpgFrontend.
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
 * Saturneric<eric@bktus.com> starting on May 12, 2021.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "SignersPicker.h"

#include "ui/widgets/KeyList.h"

namespace GpgFrontend::UI {

SignersPicker::SignersPicker(QWidget* parent)
    : GeneralDialog(typeid(SignersPicker).name(), parent) {
  auto confirm_button = new QPushButton(_("Confirm"));
  auto cancel_button = new QPushButton(_("Cancel"));

  connect(confirm_button, &QPushButton::clicked,
          [=]() { this->accepted_ = true; });
  connect(confirm_button, &QPushButton::clicked, this, &QDialog::accept);
  connect(cancel_button, &QPushButton::clicked, this, &QDialog::reject);

  /*Setup KeyList*/
  key_list_ = new KeyList(false, this);
  key_list_->AddListGroupTab(
      _("Signers"), "signers", KeyListRow::ONLY_SECRET_KEY,
      KeyListColumn::NAME | KeyListColumn::EmailAddress | KeyListColumn::Usage,
      [](const GpgKey& key, const KeyTable&) -> bool {
        return key.IsHasActualSigningCapability();
      });
  key_list_->SlotRefresh();

  auto* vbox2 = new QVBoxLayout();
  vbox2->addWidget(new QLabel(QString(_("Select Signer(s)")) + ": "));
  vbox2->addWidget(key_list_);
  vbox2->addWidget(new QLabel(
      QString(
          _("Please select one or more private keys you use for signing.")) +
      "\n" +
      _("If no key is selected, the default key will be used for signing.")));
  vbox2->addWidget(confirm_button);
  vbox2->addWidget(cancel_button);
  vbox2->addStretch(0);
  setLayout(vbox2);

  this->setWindowFlags(Qt::Window | Qt::WindowTitleHint |
                       Qt::CustomizeWindowHint);

  this->setModal(true);
  this->setWindowTitle("Signers Picker");
  this->setMinimumWidth(480);
  this->show();
}

GpgFrontend::KeyIdArgsListPtr SignersPicker::GetCheckedSigners() {
  return key_list_->GetPrivateChecked();
}

bool SignersPicker::GetStatus() const { return this->accepted_; }

}  // namespace GpgFrontend::UI
