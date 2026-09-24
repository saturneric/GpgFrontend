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
#include "core/module/ModuleInit.h"
#include "core/struct/settings_object/ModuleSO.h"
#include "ui/dialog/controller/ModuleMeta.h"
#include "ui_ModuleControllerDialog.h"

//
#include "core/module/ModuleExternalTrust.h"
#include "core/module/ModuleManager.h"
#include "ui/function/UIStyle.h"
#include "ui/widgets/GRTTreeView.h"
#include "ui/widgets/ModuleListView.h"

namespace GpgFrontend::UI {

namespace {

/// The stored policy, as the one enum that names it. Parsing is the core's
/// job, so the dialog cannot come to disagree with the loader about what a
/// stored string means.
auto CurrentLoadingPolicy() -> Module::ModuleLoadingPolicy {
  return Module::ParseModuleLoadingPolicy(
             GetSettings()
                 .value("basic/module_loading_policy",
                        Module::ModuleLoadingPolicyKey(
                            Module::ModuleLoadingPolicy::kONLY_INTEGRATED))
                 .toString())
      .policy;
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

  if (CurrentLoadingPolicy() == Module::ModuleLoadingPolicy::kDISABLE) {
    ui_->tabWidget->setTabEnabled(0, false);
  }
}

void ModuleControllerDialog::init_texts() {
  this->setWindowTitle(tr("Module Controller"));

  ui_->tabWidget->setTabText(0, tr("Registered Modules"));
  ui_->tabWidget->setTabText(1, tr("Global Register Table"));
  ui_->tabWidget->setTabText(2, tr("Developer"));

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
  ui_->filterComboBox->addItem(tr("Not Loaded"),
                               static_cast<int>(ModuleCategory::kPending));

  ui_->detailPlaceholderLabel->setText(
      tr("Select a module to see its details."));

  ui_->listeningEventsGroup->setTitle(tr("Listening Events"));

  ui_->autoActivateCheckBox->setText(tr("Activate on Start"));
  ui_->autoActivateCheckBox->setToolTip(
      tr("Activate this module automatically when GpgFrontend starts."));
  ui_->refreshButton->setText(tr("Refresh"));
  ui_->showModsDirButton->setText(tr("Show Modules Folder"));

  ui_->grtSearchLineEdit->setPlaceholderText(tr("Search keys and values..."));
  ui_->grtExpandAllButton->setText(tr("Expand All"));
  ui_->grtCollapseAllButton->setText(tr("Collapse All"));
  ui_->grtRefreshButton->setText(tr("Refresh"));
}

void ModuleControllerDialog::init_connections() {
  connect(ui_->moduleListView, &ModuleListView::SignalSelectModule, this,
          &ModuleControllerDialog::slot_load_module_details);

  connect(ui_->moduleListView, &ModuleListView::SignalCountsChanged, this,
          [=](int total, int active) {
            ui_->countLabel->setText(tr("%n module(s)", nullptr, total) +
                                     QStringLiteral(" · ") +
                                     tr("%n active", nullptr, active));
          });

  connect(
      ui_->searchLineEdit, &QLineEdit::textChanged, this,
      [=](const QString& text) { ui_->moduleListView->SetSearchFilter(text); });

  connect(ui_->filterComboBox, qOverload<int>(&QComboBox::currentIndexChanged),
          this, [=](int index) {
            ui_->moduleListView->SetCategoryFilter(static_cast<ModuleCategory>(
                ui_->filterComboBox->itemData(index).toInt()));
          });

  connect(ui_->refreshButton, &QPushButton::clicked, this,
          [=]() { refresh_all(); });

  connect(ui_->activateOrDeactivateButton, &QPushButton::clicked, this, [=]() {
    auto module_id = ui_->moduleListView->GetCurrentModuleID();
    if (module_id.isEmpty()) return;

    // One transition at a time, and the dialog hears when it has run rather
    // than guessing how long it takes: two quick clicks used to read the same
    // stale state and post the same transition twice.
    transition_pending_ = true;
    ui_->activateOrDeactivateButton->setEnabled(false);
    QPointer<ModuleControllerDialog> self(this);
    auto done = [self](bool) {
      if (self == nullptr) return;
      self->transition_pending_ = false;
      self->refresh_all();
    };
    if (!module_manager_->IsModuleActivated(module_id)) {
      module_manager_->ActiveModule(module_id, done);
    } else {
      module_manager_->DeactivateModule(module_id, done);
    }
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

  init_authorization_actions();

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
}

void ModuleControllerDialog::update_policy_notice() {
  switch (CurrentLoadingPolicy()) {
    case Module::ModuleLoadingPolicy::kDISABLE:
      ui_->policyNoticeLabel->setText(
          tr("Modules are disabled. Change Module Discovery in Settings > "
             "General to use them."));
      break;
    case Module::ModuleLoadingPolicy::kONLY_INTEGRATED:
      ui_->policyNoticeLabel->setText(
          tr("Only integrated modules are loaded. To also load modules you "
             "have added, change Module Discovery in Settings > General."));
      break;
    case Module::ModuleLoadingPolicy::kALL:
      ui_->policyNoticeLabel->hide();
      break;
  }
}

void ModuleControllerDialog::refresh_all() {
  ui_->moduleListView->Refresh();
  slot_load_module_details(ui_->moduleListView->GetCurrentModuleID());
}

void ModuleControllerDialog::init_authorization_actions() {
  // Built here rather than in the .ui file because they are only ever shown
  // for one kind of selection, and two permanently hidden buttons in the
  // designer invite somebody to wire them to something else.
  //
  // TWO buttons, deliberately, for two decisions that must not collapse into
  // one. Trusting a publisher key says "signatures by this key are worth
  // considering"; enabling says "run this one". A single "Allow" would make
  // one click do both, and the first of them applies to every module that key
  // ever signs.
  trust_key_button_ = new QPushButton(tr("Trust This Publisher Key..."), this);
  enable_module_button_ = new QPushButton(tr("Enable This Module"), this);
  trust_key_button_->hide();
  enable_module_button_->hide();

  ui_->actionsLayout->insertWidget(0, trust_key_button_);
  ui_->actionsLayout->insertWidget(1, enable_module_button_);

  connect(trust_key_button_, &QPushButton::clicked, this, [=]() {
    const auto refusal = ui_->moduleListView->GetCurrentRefusal();
    if (!refusal.valid || refusal.publisher_key.isEmpty()) return;

    const auto fingerprint =
        Module::ModulePublisherKeyFingerprint(refusal.publisher_key);

    // The fingerprint is the decision. It is shown in full, and the question
    // is phrased around the KEY rather than the module, because that is what
    // is actually being accepted -- and because this key will admit anything
    // else it signs that the user later enables.
    const auto answer = QMessageBox::question(
        this, tr("Trust This Publisher Key?"),
        tr("<p>Modules signed by this publisher key will be offered for you "
           "to enable, one at a time. Trusting it does not enable anything by "
           "itself.</p>"
           "<p><b>Publisher key fingerprint</b><br/>"
           "<code>%1</code></p>"
           "<p>The key is the publisher's identity. Any name or website a "
           "module shows is only its own claim. A module signed with a "
           "different key will ask you again.</p>"
           "<p>Only continue if you obtained this fingerprint from the "
           "module's "
           "author through a channel you trust.</p>")
                .arg(fingerprint.toHtmlEscaped()) +
            // A separate string, so the translations of the one above stay.
            tr("<p>A module you enable runs inside GpgFrontend and can do "
               "anything GpgFrontend can. The capabilities it lists are what "
               "it asked for, not a limit.</p>"),
        QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);
    if (answer != QMessageBox::Yes) return;

    if (!Module::TrustModulePublisherKey(refusal.publisher_key, {})) {
      QMessageBox::warning(
          this, tr("Not Saved"),
          tr("That decision could not be saved, so nothing has changed."));
      return;
    }
    refresh_all();
  });

  connect(enable_module_button_, &QPushButton::clicked, this, [=]() {
    const auto refusal = ui_->moduleListView->GetCurrentRefusal();
    if (!refusal.valid || refusal.module_id.isEmpty() ||
        refusal.publisher_key.isEmpty()) {
      return;
    }

    if (!Module::SetExternalModuleEnabled(refusal.module_id,
                                          refusal.publisher_key, true)) {
      QMessageBox::warning(
          this, tr("Not Saved"),
          tr("That decision could not be saved, so nothing has changed."));
      return;
    }

    // Loading happens on the next start: phase one runs once, during startup,
    // and mapping module code into a running process on demand is a different
    // and much larger change than this.
    QMessageBox::information(
        this, tr("Enabled"),
        tr("This module will be loaded the next time GpgFrontend starts."));
    refresh_all();
  });
}

auto ModuleControllerDialog::show_refused_module() -> bool {
  const auto refusal = ui_->moduleListView->GetCurrentRefusal();
  if (!refusal.valid) {
    trust_key_button_->hide();
    enable_module_button_->hide();
    return false;
  }

  ui_->detailStackedWidget->setCurrentWidget(ui_->detailPage);

  // Nothing registered, so there is nothing to activate and nothing to start
  // automatically. Left visible and disabled rather than hidden, so the
  // controls do not move about as the selection changes.
  ui_->activateOrDeactivateButton->setEnabled(false);
  ui_->autoActivateCheckBox->setEnabled(false);

  const auto display_id = refusal.module_id.isEmpty()
                              ? QFileInfo(refusal.descriptor_path).fileName()
                              : refusal.module_id;

  auto name_font = ui_->detailNameLabel->font();
  name_font.setBold(true);
  name_font.setPointSizeF(font().pointSizeF() + 2);
  ui_->detailNameLabel->setFont(name_font);
  ui_->detailNameLabel->setText(display_id);

  // Through the same helper the loaded case uses, so a refused module does
  // not read as a differently styled kind of thing in the same pane.
  SetChip(ui_->statusChipLabel,
          refusal.pending_user_action ? tr("Needs Approval") : tr("Refused"),
          AccentColor(palette(), false));
  ui_->autoChipLabel->setVisible(false);
  ui_->authorLabel->setVisible(false);
  ui_->descriptionLabel->setVisible(true);
  ui_->descriptionLabel->setText(refusal.reason);

  QVector<MetaListRow> rows;
  rows.append({.caption = tr("Status"),
               .value = refusal.pending_user_action ? tr("Waiting for you")
                                                    : tr("Not loaded"),
               .detail = refusal.reason});
  if (!refusal.module_id.isEmpty()) {
    rows.append({.caption = tr("Identifier"), .value = refusal.module_id});
  }
  rows.append({.caption = tr("Descriptor"), .value = refusal.descriptor_path});

  if (!refusal.publisher_key.isEmpty()) {
    const auto fingerprint =
        Module::ModulePublisherKeyFingerprint(refusal.publisher_key);
    const auto trusted =
        Module::IsModulePublisherKeyTrusted(refusal.publisher_key);
    rows.append(
        {.caption = tr("Publisher key"),
         .value = fingerprint,
         .detail = trusted ? tr("You have trusted this publisher key.")
                           : tr("You have not trusted this publisher key. "
                                "Before trusting it, compare its fingerprint "
                                "with the one the publisher published."),
         .degraded = !trusted});
  }

  ui_->detailMetaPanel->SetRows(rows);
  ui_->listeningEventsGroup->setVisible(false);
  ui_->listeningEventsListWidget->clear();

  // Only an external module has anything to decide. An integrated one that
  // was refused is broken, not pending, and offering an approval control for
  // it would suggest a remedy that does not exist.
  const auto decidable = !refusal.publisher_key.isEmpty();
  const auto trusted =
      decidable && Module::IsModulePublisherKeyTrusted(refusal.publisher_key);

  trust_key_button_->setVisible(decidable);
  trust_key_button_->setEnabled(decidable && !trusted);
  trust_key_button_->setText(trusted ? tr("Publisher Key Trusted")
                                     : tr("Trust This Publisher Key..."));

  enable_module_button_->setVisible(decidable);
  // Enabling stays unavailable until the key is trusted. The order is the
  // point: a module cannot be admitted by a decision about the module alone.
  enable_module_button_->setEnabled(decidable && trusted &&
                                    !refusal.module_id.isEmpty());

  return true;
}

void ModuleControllerDialog::slot_load_module_details(
    Module::ModuleIdentifier module_id) {
  // Refusals first: several of them have no identifier at all, so asking the
  // manager about one would answer "no such module" and show the placeholder.
  if (show_refused_module()) return;

  auto module = module_manager_->SearchModule(module_id);

  if (module_id.isEmpty() || module == nullptr) {
    ui_->detailStackedWidget->setCurrentWidget(ui_->placeholderPage);
    ui_->activateOrDeactivateButton->setEnabled(false);
    ui_->autoActivateCheckBox->setEnabled(false);
    return;
  }

  // The one settings policy, the loader's own: this used to reset the same
  // record by a rule of its own, which disagreed with the loader's about
  // integrated modules.
  const auto module_so = Module::ReconcileModuleSettings(
      module_id, module->GetModuleHash(),
      module_manager_->IsIntegratedModule(module_id));

  ui_->detailStackedWidget->setCurrentWidget(ui_->detailPage);
  ui_->activateOrDeactivateButton->setEnabled(!transition_pending_);
  ui_->autoActivateCheckBox->setEnabled(true);
  trust_key_button_->hide();
  enable_module_button_->hide();

  // One set of facts, assembled by the manager. The header labels below and
  // the metadata panel further down used to read them from two different
  // sources, which agreed only because one setter call bridged them.
  const auto provenance = module_manager_->GetModuleProvenance(module_id);
  const auto& meta_data = provenance.metadata;
  const auto if_activated = provenance.activated;
  const auto integrated = provenance.integrated;

  auto name_font = ui_->detailNameLabel->font();
  name_font.setBold(true);
  name_font.setPointSizeF(font().pointSizeF() + 2);
  ui_->detailNameLabel->setFont(name_font);
  ui_->detailNameLabel->setText(meta_data.value("Name", module_id));

  SetChip(ui_->statusChipLabel,
          if_activated ? tr("● Active") : tr("○ Inactive"),
          AccentColor(palette(), if_activated));
  ui_->autoChipLabel->setVisible(module_so.auto_activate);
  if (module_so.auto_activate) {
    SetChip(ui_->autoChipLabel, tr("Auto Start"), AccentColor(palette(), true));
  }

  const auto author = meta_data.value("Author");
  ui_->authorLabel->setVisible(!author.isEmpty());
  ui_->authorLabel->setText(tr("by %1").arg(author));

  const auto description = meta_data.value("Description");
  ui_->descriptionLabel->setVisible(!description.isEmpty());
  ui_->descriptionLabel->setText(description);

  // One description of a module, built by a pure function and rendered by the
  // shared panel -- so what it says can be asserted by a test, and so a fact
  // and a claim are never shown as one undifferentiated list.
  ui_->detailMetaPanel->SetRows(BuildModuleRows(provenance));

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
