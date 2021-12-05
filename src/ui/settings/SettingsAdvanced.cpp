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

#include "SettingsAdvanced.h"

namespace GpgFrontend::UI {

AdvancedTab::AdvancedTab(QWidget* parent)
    : QWidget(parent),
      appPath(qApp->applicationDirPath()),
      settings(RESOURCE_DIR(appPath) + "/conf/gpgfrontend.ini",
               QSettings::IniFormat) {
  /*****************************************
   * Steganography Box
   *****************************************/
  auto* steganoBox = new QGroupBox(_("Show Steganography Options"));
  auto* steganoBoxLayout = new QHBoxLayout();
  steganoCheckBox = new QCheckBox(_("Show Steganographic Options."), this);
  steganoBoxLayout->addWidget(steganoCheckBox);
  steganoBox->setLayout(steganoBoxLayout);

  auto* pubkeyExchangeBox = new QGroupBox(_("Pubkey Exchange"));
  auto* pubkeyExchangeBoxLayout = new QHBoxLayout();
  autoPubkeyExchangeCheckBox = new QCheckBox(_("Auto Pubkey Exchange"), this);
  pubkeyExchangeBoxLayout->addWidget(autoPubkeyExchangeCheckBox);
  pubkeyExchangeBox->setLayout(pubkeyExchangeBoxLayout);

  auto* mainLayout = new QVBoxLayout;
  mainLayout->addWidget(steganoBox);
  mainLayout->addWidget(pubkeyExchangeBox);
  setSettings();
  mainLayout->addStretch(1);
  setLayout(mainLayout);
}

void AdvancedTab::setSettings() {
  if (settings.value("advanced/steganography").toBool()) {
    steganoCheckBox->setCheckState(Qt::Checked);
  }
  if (settings.value("advanced/autoPubkeyExchange").toBool()) {
    autoPubkeyExchangeCheckBox->setCheckState(Qt::Checked);
  }
}

void AdvancedTab::applySettings() {
  settings.setValue("advanced/steganography", steganoCheckBox->isChecked());
  settings.setValue("advanced/autoPubkeyExchange",
                    autoPubkeyExchangeCheckBox->isChecked());
}

}  // namespace GpgFrontend::UI
