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

#include "ui/keypair_details/KeyDetailsDialog.h"

namespace GpgFrontend::UI {
KeyDetailsDialog::KeyDetailsDialog(const GpgKey& key, QWidget* parent)
    : QDialog(parent) {
  tabWidget = new QTabWidget();
  tabWidget->addTab(new KeyPairDetailTab(key.id(), tabWidget), tr("KeyPair"));
  tabWidget->addTab(new KeyPairUIDTab(key.id(), tabWidget), tr("UIDs"));
  tabWidget->addTab(new KeyPairSubkeyTab(key.id(), tabWidget), tr("Subkeys"));

  auto* mainLayout = new QVBoxLayout;
  mainLayout->addWidget(tabWidget);

  this->setAttribute(Qt::WA_DeleteOnClose, true);
  this->setLayout(mainLayout);
  this->setWindowTitle(tr("Key Details"));
  this->setModal(true);
  this->setMinimumSize(380, 620);
  this->show();
}
}  // namespace GpgFrontend::UI
