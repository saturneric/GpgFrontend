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

#include "ui_ModuleControllerDialog.h"

//
#include "core/module/ModuleManager.h"
#include "ui/widgets/ModuleListView.h"

namespace GpgFrontend::UI {

ModuleControllerDialog::ModuleControllerDialog(QWidget* parent)
    : QDialog(parent),
      ui_(std::make_shared<Ui_ModuleControllerDialog>()),
      model_list_view_(new ModuleListView(this)),
      module_mamager_(&Module::ModuleManager::GetInstance()) {
  ui_->setupUi(this);

  model_list_view_->setMinimumWidth(250);
  model_list_view_->setViewMode(QListView::ListMode);
  model_list_view_->setMovement(QListView::Static);

  ui_->moduleListViewLayout->addWidget(model_list_view_);
  connect(model_list_view_, &ModuleListView::SignalSelectModule, this,
          &ModuleControllerDialog::slot_load_module_details);

  connect(ui_->activateOrDeactiveButton, &QPushButton::clicked, this, [=]() {
    auto module_id = model_list_view_->GetCurrentModuleID();
    if (module_id.isEmpty()) return;

    if (!module_mamager_->IsModuleActivated(module_id)) {
      module_mamager_->ActiveModule(module_id);
    } else {
      module_mamager_->DeactiveModule(module_id);
    }

    QTimer::singleShot(1000, [=]() { slot_load_module_details(module_id); });
  });

  connect(ui_->triggerEventButton, &QPushButton::clicked, this, [=]() {
    auto event_id =
        QInputDialog::getText(this, "Please provide an Event ID", "Event ID");
    Module::TriggerEvent(event_id);
  });
}

void ModuleControllerDialog::slot_load_module_details(
    Module::ModuleIdentifier module_id) {
  GF_UI_LOG_DEBUG("loading module detailes, module id: {}", module_id);

  auto module = module_mamager_->SearchModule(module_id);

  ui_->moduleIDLabel->setText(module->GetModuleIdentifier());

  QString buffer;
  QTextStream info(&buffer);

  info << "# BASIC INFO" << Qt::endl << Qt::endl;

  info << tr("Version") << ": " << module->GetModuleVersion() << Qt::endl;
  info << tr("Hash") << ": " << module->GetModuleHash() << Qt::endl;
  info << tr("Path") << ": " << module->GetModulePath() << Qt::endl;

  bool if_activated = module_mamager_->IsModuleActivated(module_id);

  info << tr("Active") << ": " << (if_activated ? tr("True") : tr("False"))
       << Qt::endl;

  info << Qt::endl;

  info << "# METADATA" << Qt::endl << Qt::endl;

  for (const auto& metadata : module->GetModuleMetaData()) {
    info << metadata.first << ": " << metadata.second << "\n";
  }

  info << Qt::endl;

  info << "# Listening Event" << Qt::endl << Qt::endl;

  auto listening_event_ids = module_mamager_->GetModuleListening(module_id);
  for (const auto& event_id : listening_event_ids) {
    info << " - " << event_id << "\n";
  }

  ui_->moduleInfoTextBrowser->setText(buffer);
  ui_->activateOrDeactiveButton->setText(if_activated ? tr("Deactivate")
                                                      : tr("Activate"));
}
}  // namespace GpgFrontend::UI
