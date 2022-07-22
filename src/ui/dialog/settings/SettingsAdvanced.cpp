/**
 * Copyright (C) 2021 Saturneric
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
 * Saturneric<eric@bktus.com> starting on May 12, 2021.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include "SettingsAdvanced.h"

#include "core/function/GlobalSettingStation.h"

namespace GpgFrontend::UI {

AdvancedTab::AdvancedTab(QWidget* parent) : QWidget(parent) {
  auto* stegano_box = new QGroupBox(_("Show Steganography Options"));
  auto* stegano_box_layout = new QHBoxLayout();
  stegano_check_box_ = new QCheckBox(_("Show Steganography Options."), this);
  stegano_box_layout->addWidget(stegano_check_box_);
  stegano_box->setLayout(stegano_box_layout);

  auto* pubkey_exchange_box = new QGroupBox(_("Pubkey Exchange"));
  auto* pubkey_exchange_box_layout = new QHBoxLayout();
  auto_pubkey_exchange_check_box_ =
      new QCheckBox(_("Auto Pubkey Exchange"), this);
  pubkey_exchange_box_layout->addWidget(auto_pubkey_exchange_check_box_);
  pubkey_exchange_box->setLayout(pubkey_exchange_box_layout);

  auto* main_layout = new QVBoxLayout;
  main_layout->addWidget(stegano_box);
  main_layout->addWidget(pubkey_exchange_box);
  SetSettings();
  main_layout->addStretch(1);
  setLayout(main_layout);
}

void AdvancedTab::SetSettings() {
  auto& settings = GlobalSettingStation::GetInstance().GetUISettings();
  try {
    bool stegano_checked = settings.lookup("advanced.stegano_checked");
    if (stegano_checked) stegano_check_box_->setCheckState(Qt::Checked);
  } catch (...) {
    LOG(ERROR) << _("Setting Operation Error") << _("stegano_checked");
  }

  try {
    bool auto_pubkey_exchange_checked =
        settings.lookup("advanced.auto_pubkey_exchange_checked");
    if (auto_pubkey_exchange_checked)
      auto_pubkey_exchange_check_box_->setCheckState(Qt::Checked);
  } catch (...) {
    LOG(ERROR) << _("Setting Operation Error")
               << _("auto_pubkey_exchange_checked");
  }
}

void AdvancedTab::ApplySettings() {
  auto& settings =
      GpgFrontend::GlobalSettingStation::GetInstance().GetUISettings();

  if (!settings.exists("advanced") ||
      settings.lookup("advanced").getType() != libconfig::Setting::TypeGroup)
    settings.add("advanced", libconfig::Setting::TypeGroup);

  auto& advanced = settings["advanced"];

  if (!advanced.exists("stegano_checked"))
    advanced.add("stegano_checked", libconfig::Setting::TypeBoolean) =
        stegano_check_box_->isChecked();
  else {
    advanced["stegano_checked"] = stegano_check_box_->isChecked();
  }

  if (!advanced.exists("auto_pubkey_exchange_checked"))
    advanced.add("auto_pubkey_exchange_checked",
                 libconfig::Setting::TypeBoolean) =
        auto_pubkey_exchange_check_box_->isChecked();
  else {
    advanced["auto_pubkey_exchange_checked"] =
        auto_pubkey_exchange_check_box_->isChecked();
  }
}

}  // namespace GpgFrontend::UI
