/**
 * This file is part of GPGFrontend.
 *
 * GPGFrontend is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Foobar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Foobar.  If not, see <https://www.gnu.org/licenses/>.
 *
 * The initial version of the source code is inherited from gpg4usb-team.
 * Their source code version also complies with GNU General Public License.
 *
 * The source code version of this software was modified and released
 * by Saturneric<eric@bktus.com> starting on May 12, 2021.
 *
 */

#include "ui/widgets/SignersPicker.h"

namespace GpgFrontend::UI {

SignersPicker::SignersPicker(QWidget* parent) : QDialog(parent) {
  auto confirmButton = new QPushButton(tr("Confirm"));
  connect(confirmButton, SIGNAL(clicked(bool)), this, SLOT(accept()));

  /*Setup KeyList*/
  mKeyList = new KeyList(
      KeyListRow::ONLY_SECRET_KEY,
      KeyListColumn::NAME | KeyListColumn::EmailAddress | KeyListColumn::Usage);

  mKeyList->setFilter([](const GpgKey& key) -> bool {
    if (!key.CanSignActual())
      return false;
    else
      return true;
  });

  mKeyList->slotRefresh();

  auto* vbox2 = new QVBoxLayout();
  vbox2->addWidget(new QLabel("Select Signer(s): "));
  vbox2->addWidget(mKeyList);
  vbox2->addWidget(confirmButton);
  vbox2->addStretch(0);
  setLayout(vbox2);

  this->setModal(true);
  this->setWindowTitle("Signers Picker");
  this->setMinimumWidth(480);
  this->show();
}

GpgFrontend::KeyIdArgsListPtr SignersPicker::getCheckedSigners() {
  return mKeyList->getPrivateChecked();
}

}  // namespace GpgFrontend::UI
