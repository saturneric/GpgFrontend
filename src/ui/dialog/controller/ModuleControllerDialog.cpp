/**
 * Copyright (C) 2021-2024 Saturneric <eric@bktus.com>
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
#include "ui/widgets/GRTTreeView.h"
#include "ui/widgets/ModuleListView.h"

namespace GpgFrontend::UI {

namespace {

/// delay before the second refresh, module (de)activation is posted to the
/// module task runner and does not take effect synchronously
constexpr int kActivationSettleMs = 300;

/// number of hash characters shown before the ellipsis
constexpr int kHashDisplayLength = 24;

auto AccentColor(const QPalette& palette, bool positive) -> QColor {
  const auto dark = palette.color(QPalette::Base).lightness() < 128;
  if (!positive) {
    auto color = palette.color(QPalette::Text);
    color.setAlpha(150);
    return color;
  }
  return dark ? QColor(102, 187, 106) : QColor(46, 125, 50);
}

}  // namespace

ModuleControllerDialog::ModuleControllerDialog(QWidget* parent)
    : GeneralDialog("ModuleControllerDialog", parent),
      ui_(QSharedPointer<Ui_ModuleControllerDialog>::create()),
      module_manager_(&Module::ModuleManager::GetInstance()) {
  ui_->setupUi(this);

  init_texts();
  init_connections();
  update_policy_notice();

  ui_->moduleSplitter->setStretchFactor(0, 1);
  ui_->moduleSplitter->setStretchFactor(1, 2);

  slot_load_module_details({});

#ifdef RELEASE
  ui_->tabWidget->setTabVisible(2, false);
#endif

  const auto module_loading_policy =
      GetSettings()
          .value("basic/module_loading_policy", "only_integrated")
          .toString();

  if (module_loading_policy == "disable") {
    ui_->tabWidget->setTabEnabled(0, false);
  }
}

void ModuleControllerDialog::init_texts() {
  this->setWindowTitle(tr("Module Controller"));

  ui_->tabWidget->setTabText(0, tr("Registered Modules"));
  ui_->tabWidget->setTabText(1, tr("Global Register Table"));
  ui_->tabWidget->setTabText(2, tr("Debugger"));

  ui_->searchLineEdit->setPlaceholderText(tr("Search modules..."));
  ui_->filterComboBox->addItem(tr("All"),
                               static_cast<int>(ModuleCategory::kAll));
  ui_->filterComboBox->addItem(tr("Active"),
                               static_cast<int>(ModuleCategory::kActive));
  ui_->filterComboBox->addItem(tr("Inactive"),
                               static_cast<int>(ModuleCategory::kInactive));
  ui_->filterComboBox->addItem(tr("Integrated"),
                               static_cast<int>(ModuleCategory::kIntegrated));
  ui_->filterComboBox->addItem(tr("External"),
                               static_cast<int>(ModuleCategory::kExternal));

  ui_->detailPlaceholderLabel->setText(
      tr("Select a module to see its details."));

  ui_->idKeyLabel->setText(tr("ID"));
  ui_->sdkKeyLabel->setText(tr("SDK Version"));
  ui_->qtKeyLabel->setText(tr("Qt ENV Version"));
  ui_->hashKeyLabel->setText(tr("Hash"));
  ui_->pathKeyLabel->setText(tr("Path"));

  ui_->listeningEventsGroup->setTitle(tr("Listening Events"));

  ui_->autoActivateCheckBox->setText(tr("Activate on Start"));
  ui_->autoActivateCheckBox->setToolTip(
      tr("Activate this module automatically when GpgFrontend starts."));
  ui_->refreshButton->setText(tr("Refresh"));
  ui_->showModsDirButton->setText(tr("Show Mods Directory"));

  ui_->grtSearchLineEdit->setPlaceholderText(tr("Search keys and values..."));
  ui_->grtExpandAllButton->setText(tr("Expand All"));
  ui_->grtCollapseAllButton->setText(tr("Collapse All"));
  ui_->grtRefreshButton->setText(tr("Refresh"));

  ui_->triggerEventButton->setText(tr("Trigger Event"));
  ui_->upsertGrtValueButton->setText(tr("Upsert GRT Value"));
}

void ModuleControllerDialog::init_connections() {
  connect(ui_->moduleListView, &ModuleListView::SignalSelectModule, this,
          &ModuleControllerDialog::slot_load_module_details);

  connect(ui_->moduleListView, &ModuleListView::SignalCountsChanged, this,
          [=](int total, int active) {
            ui_->countLabel->setText(
                tr("%1 modules · %2 active").arg(total).arg(active));
          });

  connect(
      ui_->searchLineEdit, &QLineEdit::textChanged, this,
      [=](const QString& text) { ui_->moduleListView->SetSearchFilter(text); });

  connect(ui_->filterComboBox, &QComboBox::currentIndexChanged, this,
          [=](int index) {
            ui_->moduleListView->SetCategoryFilter(static_cast<ModuleCategory>(
                ui_->filterComboBox->itemData(index).toInt()));
          });

  connect(ui_->refreshButton, &QPushButton::clicked, this,
          [=]() { refresh_all(); });

  connect(ui_->activateOrDeactivateButton, &QPushButton::clicked, this, [=]() {
    auto module_id = ui_->moduleListView->GetCurrentModuleID();
    if (module_id.isEmpty()) return;

    if (!module_manager_->IsModuleActivated(module_id)) {
      module_manager_->ActiveModule(module_id);
    } else {
      module_manager_->DeactivateModule(module_id);
    }

    refresh_all();
    QTimer::singleShot(kActivationSettleMs, this, [=]() { refresh_all(); });
  });

  connect(ui_->autoActivateCheckBox, &QCheckBox::clicked, this,
          [=](bool checked) {
            auto module_id = ui_->moduleListView->GetCurrentModuleID();
            if (module_id.isEmpty()) return;

            SettingsObject so(QString("module.%1.so").arg(module_id));
            ModuleSO module_so(so);

            module_so.auto_activate = checked;
            module_so.set_by_user = true;
            so.Store(module_so.ToJson());

            refresh_all();
          });

  connect(ui_->pathValueLabel, &QLabel::linkActivated, this,
          [](const QString& link) {
            QDesktopServices::openUrl(
                QUrl::fromLocalFile(QFileInfo(link).absolutePath()));
          });

  connect(ui_->showModsDirButton, &QPushButton::clicked, this, [=]() {
    QDesktopServices::openUrl(QUrl::fromLocalFile(
        GlobalSettingStation::GetInstance().GetModulesDir()));
  });

  connect(ui_->grtSearchLineEdit, &QLineEdit::textChanged, this,
          [=](const QString& text) { ui_->treeView->SetFilter(text); });
  connect(ui_->grtExpandAllButton, &QPushButton::clicked, this,
          [=]() { ui_->treeView->ExpandAll(); });
  connect(ui_->grtCollapseAllButton, &QPushButton::clicked, this,
          [=]() { ui_->treeView->CollapseAll(); });
  connect(ui_->grtRefreshButton, &QPushButton::clicked, this,
          [=]() { ui_->treeView->Refresh(); });

  connect(ui_->triggerEventButton, &QPushButton::clicked, this, [=]() {
    auto event_id = QInputDialog::getText(this, tr("Trigger Event"),
                                          tr("Please provide an Event ID"));
    if (event_id.isEmpty()) return;
    Module::TriggerEvent(event_id);
  });

  connect(ui_->upsertGrtValueButton, &QPushButton::clicked, this, [=]() {
    auto ok = false;
    auto ns =
        QInputDialog::getText(this, tr("Upsert GRT Value"), tr("Namespace"),
                              QLineEdit::Normal, {}, &ok);
    if (!ok || ns.isEmpty()) return;

    auto key = QInputDialog::getText(this, tr("Upsert GRT Value"), tr("Key"),
                                     QLineEdit::Normal, {}, &ok);
    if (!ok || key.isEmpty()) return;

    auto value = QInputDialog::getText(this, tr("Upsert GRT Value"),
                                       tr("Value"), QLineEdit::Normal, {}, &ok);
    if (!ok) return;

    Module::UpsertRTValue(ns, key, value);
    ui_->treeView->Refresh();
  });
}

void ModuleControllerDialog::update_policy_notice() {
  const auto module_loading_policy =
      GetSettings()
          .value("basic/module_loading_policy", "only_integrated")
          .toString();

  if (module_loading_policy == "disable") {
    ui_->policyNoticeLabel->setText(
        tr("Module loading is disabled. Enable it in Settings > General to "
           "use modules."));
  } else if (module_loading_policy == "only_integrated") {
    ui_->policyNoticeLabel->setText(
        tr("Only integrated modules are loaded. To load external modules from "
           "the mods directory, change the module loading policy in "
           "Settings > General."));
  } else {
    ui_->policyNoticeLabel->hide();
  }
}

void ModuleControllerDialog::refresh_all() {
  ui_->moduleListView->Refresh();
  slot_load_module_details(ui_->moduleListView->GetCurrentModuleID());
}

void ModuleControllerDialog::set_chip(QLabel* label, const QString& text,
                                      const QColor& color) {
  label->setText(QString("<span style=\"color:%1;\">%2</span>")
                     .arg(color.name(QColor::HexRgb), text.toHtmlEscaped()));
}

void ModuleControllerDialog::slot_load_module_details(
    Module::ModuleIdentifier module_id) {
  auto module = module_manager_->SearchModule(module_id);

  if (module_id.isEmpty() || module == nullptr) {
    ui_->detailStackedWidget->setCurrentWidget(ui_->placeholderPage);
    ui_->activateOrDeactivateButton->setEnabled(false);
    ui_->autoActivateCheckBox->setEnabled(false);
    return;
  }

  SettingsObject so(QString("module.%1.so").arg(module_id));
  ModuleSO module_so(so);

  if (module_so.module_id != module_id ||
      module_so.module_hash != module->GetModuleHash()) {
    module_so.module_id = module_id;
    module_so.module_hash = module->GetModuleHash();
    module_so.auto_activate = false;
    so.Store(module_so.ToJson());
  }

  ui_->detailStackedWidget->setCurrentWidget(ui_->detailPage);
  ui_->activateOrDeactivateButton->setEnabled(true);
  ui_->autoActivateCheckBox->setEnabled(true);

  const auto meta_data = module->GetModuleMetaData();
  const auto if_activated = module_manager_->IsModuleActivated(module_id);
  const auto integrated = module_manager_->IsIntegratedModule(module_id);

  auto name_font = ui_->detailNameLabel->font();
  name_font.setBold(true);
  name_font.setPointSizeF(font().pointSizeF() + 2);
  ui_->detailNameLabel->setFont(name_font);
  ui_->detailNameLabel->setText(meta_data.value("Name", module_id));

  ui_->detailVersionLabel->setText(module->GetModuleVersion());

  set_chip(ui_->statusChipLabel,
           if_activated ? tr("● Active") : tr("○ Inactive"),
           AccentColor(palette(), if_activated));
  set_chip(ui_->typeChipLabel, integrated ? tr("Integrated") : tr("External"),
           palette().color(QPalette::Link));
  ui_->autoChipLabel->setVisible(module_so.auto_activate);
  if (module_so.auto_activate) {
    set_chip(ui_->autoChipLabel, tr("Auto Start"),
             AccentColor(palette(), true));
  }

  const auto author = meta_data.value("Author");
  ui_->authorLabel->setVisible(!author.isEmpty());
  ui_->authorLabel->setText(tr("by %1").arg(author));

  const auto description = meta_data.value("Description");
  ui_->descriptionLabel->setVisible(!description.isEmpty());
  ui_->descriptionLabel->setText(description);

  ui_->idValueLabel->setText(module->GetModuleIdentifier());
  ui_->sdkValueLabel->setText(module->GetModuleSDKVersion());
  ui_->qtValueLabel->setText(module->GetModuleQtEnvVersion());

  // hashes have no break opportunity, shorten them instead of widening the
  // whole panel
  const auto hash = module->GetModuleHash();
  auto hash_text = tr("N/A");
  if (!hash.isEmpty()) {
    hash_text = hash.size() > kHashDisplayLength
                    ? hash.left(kHashDisplayLength) + "..."
                    : hash;
  }
  ui_->hashValueLabel->setText(hash_text);
  ui_->hashValueLabel->setToolTip(hash);

  const auto path = module->GetModulePath();
  if (path.isEmpty()) {
    ui_->pathValueLabel->setText(tr("N/A (integrated)"));
  } else {
    ui_->pathValueLabel->setText(
        QString("<a href=\"%1\">%1</a>").arg(path.toHtmlEscaped()));
    ui_->pathValueLabel->setToolTip(tr("Click to open the containing folder"));
  }

  ui_->listeningEventsListWidget->clear();
  const auto listening_event_ids =
      if_activated ? module_manager_->GetModuleListening(module_id)
                   : QStringList{};
  ui_->listeningEventsListWidget->addItems(listening_event_ids);
  ui_->listeningEventsGroup->setVisible(!listening_event_ids.isEmpty());
  ui_->listeningEventsGroup->setTitle(
      tr("Listening Events (%1)").arg(listening_event_ids.size()));

  ui_->activateOrDeactivateButton->setText(if_activated ? tr("Deactivate")
                                                        : tr("Activate"));

  const QSignalBlocker blocker(ui_->autoActivateCheckBox);
  ui_->autoActivateCheckBox->setChecked(module_so.auto_activate);
}
}  // namespace GpgFrontend::UI
