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

#include "ModuleControllerDialog.h"

#include "core/function/GlobalSettingStation.h"
#include "core/model/SettingsObject.h"
#include "core/struct/settings_object/ModuleSO.h"
#include "ui_ModuleControllerDialog.h"

//
#include "core/module/ModuleManager.h"
#include "ui/widgets/ModuleListView.h"

namespace GpgFrontend::UI {

ModuleControllerDialog::ModuleControllerDialog(QWidget* parent)
    : QDialog(parent),
      ui_(std::make_shared<Ui_ModuleControllerDialog>()),
      module_manager_(&Module::ModuleManager::GetInstance()) {
  ui_->setupUi(this);
  ui_->actionsGroupBox->hide();

  connect(ui_->moduleListView, &ModuleListView::SignalSelectModule, this,
          &ModuleControllerDialog::slot_load_module_details);

  connect(ui_->activateOrDeactiveButton, &QPushButton::clicked, this, [=]() {
    auto module_id = ui_->moduleListView->GetCurrentModuleID();
    if (module_id.isEmpty()) return;

    if (!module_manager_->IsModuleActivated(module_id)) {
      module_manager_->ActiveModule(module_id);
    } else {
      module_manager_->DeactiveModule(module_id);
    }

    QTimer::singleShot(1000, [=]() { slot_load_module_details(module_id); });
  });

  connect(ui_->autoActivateButton, &QPushButton::clicked, this, [=]() {
    auto module_id = ui_->moduleListView->GetCurrentModuleID();
    SettingsObject so(QString("module.%1.so").arg(module_id));
    ModuleSO module_so(so);

    module_so.auto_activate =
        ui_->autoActivateButton->text() == tr("Enable Auto Activate");
    so.Store(module_so.ToJson());

    QTimer::singleShot(1000, [=]() { slot_load_module_details(module_id); });
  });

  connect(ui_->triggerEventButton, &QPushButton::clicked, this, [=]() {
    auto event_id =
        QInputDialog::getText(this, "Please provide an Event ID", "Event ID");
    Module::TriggerEvent(event_id);
  });

  connect(ui_->showModsDirButton, &QPushButton::clicked, this, [=]() {
    QDesktopServices::openUrl(QUrl::fromLocalFile(
        GlobalSettingStation::GetInstance().GetModulesDir()));
  });
}

void ModuleControllerDialog::slot_load_module_details(
    Module::ModuleIdentifier module_id) {
  GF_UI_LOG_DEBUG("loading module details, module id: {}", module_id);
  auto module = module_manager_->SearchModule(module_id);
  SettingsObject so(QString("module.%1.so").arg(module_id));
  ModuleSO module_so(so);

  if (module_id.isEmpty() || module == nullptr) {
    ui_->actionsGroupBox->hide();
    return;
  }

  ui_->actionsGroupBox->show();

  if (module_so.module_id != module_id ||
      module_so.module_hash != module->GetModuleHash()) {
    module_so.module_id = module_id;
    module_so.module_hash = module->GetModuleHash();
    module_so.auto_activate = false;
    GF_UI_LOG_DEBUG("reseting module settings object, module id: {}",
                    module_id);
    so.Store(module_so.ToJson());
  }

  QString buffer;
  QTextStream info(&buffer);

  info << "# BASIC INFO" << Qt::endl << Qt::endl;

  info << " - " << tr("ID") << ": " << module->GetModuleIdentifier()
       << Qt::endl;
  info << " - " << tr("Version") << ": " << module->GetModuleVersion()
       << Qt::endl;
  info << " - " << tr("SDK Version") << ": " << module->GetModuleSDKVersion()
       << Qt::endl;
  info << " - " << tr("Qt ENV Version") << ": "
       << module->GetModuleQtEnvVersion() << Qt::endl;
  info << " - " << tr("Hash") << ": " << module->GetModuleHash() << Qt::endl;
  info << " - " << tr("Path") << ": " << module->GetModulePath() << Qt::endl;

  auto if_activated = module_manager_->IsModuleActivated(module_id);

  info << " - " << tr("Auto Activate") << ": "
       << (module_so.auto_activate ? tr("True") : tr("False")) << Qt::endl;
  info << " - " << tr("Active") << ": "
       << (if_activated ? tr("True") : tr("False")) << Qt::endl;

  info << Qt::endl;

  info << "# METADATA" << Qt::endl << Qt::endl;

  for (const auto& metadata : module->GetModuleMetaData().asKeyValueRange()) {
    info << " - " << metadata.first << ": " << metadata.second << "\n";
  }

  info << Qt::endl;

  if (if_activated) {
    info << "# Listening Event" << Qt::endl << Qt::endl;

    auto listening_event_ids = module_manager_->GetModuleListening(module_id);
    for (const auto& event_id : listening_event_ids) {
      info << " - " << event_id << "\n";
    }
  }

  ui_->moduleInfoTextBrowser->setText(buffer);
  ui_->activateOrDeactiveButton->setText(if_activated ? tr("Deactivate")
                                                      : tr("Activate"));
  ui_->autoActivateButton->setText(module_so.auto_activate
                                       ? tr("Disable Auto Activate")
                                       : tr("Enable Auto Activate"));
}
}  // namespace GpgFrontend::UI
