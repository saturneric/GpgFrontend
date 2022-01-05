/**
 * This file is part of GpgFrontend.
 *
 * GpgFrontend is free software: you can redistribute it and/or modify
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

#include "ui/widgets/KeyList.h"

namespace GpgFrontend::UI {

SignersPicker::SignersPicker(QWidget* parent) : QDialog(parent) {
  auto confirm_button = new QPushButton(_("Confirm"));
  connect(confirm_button, SIGNAL(clicked(bool)), this, SLOT(accept()));

  /*Setup KeyList*/
  key_list_ = new KeyList(false, this);
  key_list_->addListGroupTab(
      _("Signers"), KeyListRow::ONLY_SECRET_KEY,
      KeyListColumn::NAME | KeyListColumn::EmailAddress | KeyListColumn::Usage,
      [](const GpgKey& key) -> bool { return key.CanSignActual(); });
  key_list_->slotRefresh();

  auto* vbox2 = new QVBoxLayout();
  vbox2->addWidget(new QLabel(QString(_("Select Signer(s)")) + ": "));
  vbox2->addWidget(key_list_);
  vbox2->addWidget(new QLabel(
      QString(
          _("Please select one or more private keys you use for signing.")) +
      "\n" +
      _("If no key is selected, the default key will be used for signing.")));
  vbox2->addWidget(confirm_button);
  vbox2->addStretch(0);
  setLayout(vbox2);

  this->setWindowFlags(Qt::Window | Qt::WindowTitleHint |
                       Qt::CustomizeWindowHint);

  this->setModal(true);
  this->setWindowTitle("Signers Picker");
  this->setMinimumWidth(480);
  this->show();
}

GpgFrontend::KeyIdArgsListPtr SignersPicker::getCheckedSigners() {
  return key_list_->getPrivateChecked();
}

}  // namespace GpgFrontend::UI
