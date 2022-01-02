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

#include "SenderPicker.h"

#include "ui/widgets/KeyList.h"

GpgFrontend::UI::SenderPicker::SenderPicker(const KeyId& current_key_id,
                                            QWidget* parent)
    : QDialog(parent) {
  auto confirm_button = new QPushButton(_("Confirm"));
  connect(confirm_button, SIGNAL(clicked(bool)), this, SLOT(accept()));

  // Setup KeyList
  key_list_ = new KeyList(false, this);
  key_list_->addListGroupTab(
      _("Sender"), KeyListRow::ONLY_SECRET_KEY,
      KeyListColumn::NAME | KeyListColumn::EmailAddress,
      [](const GpgKey& key) -> bool { return key.CanSignActual(); });
  key_list_->slotRefresh();

  auto key_ids = std::make_unique<GpgFrontend::KeyIdArgsList>();
  key_ids->push_back(current_key_id);
  key_list_->setChecked(std::move(key_ids));

  auto* vbox2 = new QVBoxLayout();
  vbox2->addWidget(new QLabel(QString(_("Select Sender")) + ": "));
  vbox2->addWidget(key_list_);
  vbox2->addWidget(new QLabel(
      QString(
          _("As the sender of the mail, the private key is generally used.")) +
      "\n" +
      _(" The private key is generally used as a signature for the content of "
        "the mail.")));
  vbox2->addWidget(confirm_button);
  vbox2->addStretch(0);
  setLayout(vbox2);

  this->setWindowFlags(Qt::Window | Qt::WindowTitleHint |
                       Qt::CustomizeWindowHint);

  this->setModal(true);
  this->setWindowTitle("Sender Picker");
  this->setMinimumWidth(480);
  this->exec();
}

GpgFrontend::KeyId GpgFrontend::UI::SenderPicker::getCheckedSender() {
  auto checked_keys = key_list_->getChecked();
  if (!checked_keys->empty()) {
    return key_list_->getChecked()->front();
  } else {
    return {};
  }
}
