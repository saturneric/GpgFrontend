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

#include "RecipientsPicker.h"

#include "ui/widgets/KeyList.h"

GpgFrontend::UI::RecipientsPicker::RecipientsPicker(
    const GpgFrontend::KeyIdArgsListPtr& current_key_ids, QWidget* parent)
    : QDialog(parent) {
  auto confirm_button = new QPushButton(_("Confirm"));
  connect(confirm_button, SIGNAL(clicked(bool)), this, SLOT(accept()));

  // Setup KeyList
  key_list_ = new KeyList(KeyMenuAbility::NONE, this);
  key_list_->addListGroupTab(
      _("Recipient(s)"), KeyListRow::SECRET_OR_PUBLIC_KEY,
      KeyListColumn::NAME | KeyListColumn::EmailAddress,
      [](const GpgKey& key) -> bool {
        return !key.is_private_key() && key.CanEncrActual();
      });
  key_list_->slotRefresh();

  auto key_ids = std::make_unique<GpgFrontend::KeyIdArgsList>();
  for (const auto& key_id : *current_key_ids) {
    key_ids->push_back(key_id);
  }
  key_list_->setChecked(std::move(key_ids));

  auto* vbox2 = new QVBoxLayout();
  vbox2->addWidget(new QLabel(QString(_("Select Recipient(s)")) + ": "));
  vbox2->addWidget(key_list_);
  vbox2->addWidget(new QLabel(
      QString(_("We use the public key provided by the recipient to encrypt "
                "the text.")) +
      "\n" +
      _("If you want to send to multiple recipients at the same time, you can "
        "select multiple keys.")));
  vbox2->addWidget(confirm_button);
  vbox2->addStretch(0);
  setLayout(vbox2);

  this->setWindowFlags(Qt::Window | Qt::WindowTitleHint |
                       Qt::CustomizeWindowHint);

  this->setModal(true);
  this->setWindowTitle("Recipient(s) Picker");
  this->setMinimumWidth(480);
  this->exec();
}

GpgFrontend::KeyIdArgsListPtr
GpgFrontend::UI::RecipientsPicker::getCheckedRecipients() {
  return key_list_->getChecked();
}
