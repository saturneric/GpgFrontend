/**
 * Copyright (C) 2021 Saturneric <eric@bktus.com>
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

#include "KeyDetailsDialog.h"

#include "core/GpgModel.h"
#include "ui/dialog/keypair_details/KeyPairDetailTab.h"
#include "ui/dialog/keypair_details/KeyPairOperaTab.h"
#include "ui/dialog/keypair_details/KeyPairSubkeyTab.h"
#include "ui/dialog/keypair_details/KeyPairUIDTab.h"

namespace GpgFrontend::UI {
KeyDetailsDialog::KeyDetailsDialog(const GpgKey& key, QWidget* parent)
    : GeneralDialog(typeid(KeyDetailsDialog).name(), parent) {
  tab_widget_ = new QTabWidget();
  tab_widget_->addTab(new KeyPairDetailTab(key.GetId(), tab_widget_),
                      _("KeyPair"));
  tab_widget_->addTab(new KeyPairUIDTab(key.GetId(), tab_widget_), _("UIDs"));
  tab_widget_->addTab(new KeyPairSubkeyTab(key.GetId(), tab_widget_),
                      _("Subkeys"));
  tab_widget_->addTab(new KeyPairOperaTab(key.GetId(), tab_widget_),
                      _("Operations"));

  auto* main_layout = new QVBoxLayout;
  main_layout->addWidget(tab_widget_);

#ifdef MACOS
  setAttribute(Qt::WA_LayoutUsesWidgetRect);
#endif
  this->setAttribute(Qt::WA_DeleteOnClose, true);
  this->setLayout(main_layout);
  this->setWindowTitle(_("Key Details"));
  this->setModal(true);

  this->show();
}
}  // namespace GpgFrontend::UI
