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

#include "SettingsGeneral.h"

#include "SettingsDialog.h"
#include "core/function/GlobalSettingStation.h"
#include "core/utils/CommonUtils.h"
#include "core/utils/RustUtils.h"
#include "ui_GeneralSettings.h"

namespace GpgFrontend::UI {

GeneralTab::GeneralTab(QWidget* parent)
    : QWidget(parent),
      ui_(GpgFrontend::SecureCreateSharedObject<Ui_GeneralSettings>()) {
  ui_->setupUi(this);

  ui_->baseBox->setTitle(tr("Base"));

  ui_->clearGpgPasswordCacheCheckBox->setText(
      tr("Clear gpg password cache when closing GpgFrontend."));

  // Hide the "Clear gpg password cache" option if GnuPG is not supported, since
  // this option is not useful without GnuPG and may cause confusion to users.
  auto if_gnupg_supported = GetGSS().IsEngineSupported(OpenPGPEngine::kGNUPG);
  if (!if_gnupg_supported) {
    ui_->clearGpgPasswordCacheCheckBox->setHidden(true);
  }

  ui_->defaultEngineLabel->setText(tr("Default Engine:"));
  for (const auto& engine : GetGSS().AllSupportedEngines()) {
    ui_->defaultEngineComboBox->addItem(engine, engine.toUpper());
  }

  // rPGP passphrase cache timeouts, presented in minutes (stored in seconds).
  ui_->rpgpCacheTtlLabel->setText(tr("rPGP Password Cache TTL (minutes):"));
  ui_->rpgpCacheTtlSpinBox->setRange(1, 1440);  // 1 minute .. 24 hours
  ui_->rpgpCacheTtlSpinBox->setSuffix(tr(" min"));
  ui_->rpgpCacheTtlSpinBox->setToolTip(
      tr("Idle time the rPGP engine keeps an entered passphrase cached. The "
         "window is renewed each time the passphrase is used."));

  ui_->rpgpCacheMaxTtlLabel->setText(
      tr("rPGP Password Cache Max TTL (minutes):"));
  ui_->rpgpCacheMaxTtlSpinBox->setRange(1, 10080);  // 1 minute .. 7 days
  ui_->rpgpCacheMaxTtlSpinBox->setSuffix(tr(" min"));
  ui_->rpgpCacheMaxTtlSpinBox->setToolTip(
      tr("Absolute lifetime of a cached passphrase, measured from when it was "
         "first entered, regardless of use. Never shorter than the TTL."));

  // Only relevant to the rPGP engine; hide entirely when it is unavailable.
  auto if_rpgp_supported = GetGSS().IsEngineSupported(OpenPGPEngine::kRPGP);
  if (!if_rpgp_supported) {
    ui_->rpgpCacheTtlLabel->setHidden(true);
    ui_->rpgpCacheTtlSpinBox->setHidden(true);
    ui_->rpgpCacheMaxTtlLabel->setHidden(true);
    ui_->rpgpCacheMaxTtlSpinBox->setHidden(true);
  }

  ui_->modulePolicyLabel->setText(tr("Module Loading Policy:"));
  ui_->modulePolicyComboBox->addItem(tr("Only Integrated Modules"),
                                     "only_integrated");
  ui_->modulePolicyComboBox->addItem(tr("All Modules"), "all");
  ui_->modulePolicyComboBox->addItem(tr("Disable"), "disable");

  ui_->importConfirmationBox->setTitle(tr("Operation"));

  ui_->defaultWorkspaceAsLabel->setText(tr("Default Workspace As:"));
  ui_->filePanelRadioButton->setText(tr("File Panel"));
  ui_->textEditorRadioButton->setText(tr("Text Editor"));

  auto* workspace_button_group = new QButtonGroup(this);
  workspace_button_group->addButton(ui_->filePanelRadioButton);
  workspace_button_group->addButton(ui_->textEditorRadioButton);

  ui_->homePathAsDefaultPathcheckBox->setText(
      tr("Use home path as the default path for FilePanel"));

  ui_->restoreTextEditorPageCheckBox->setText(
      tr("Cache text editor contents."));

  ui_->importConfirmationCheckBox->setText(
      tr("Import files dropped on the Key List without confirmation."));

  ui_->asciiModeCheckBox->setText(tr("Use Binary Mode for File Operations"));

  ui_->langBox->setTitle(tr("Language"));
  ui_->langNoteLabel->setText(
      "<b>" + tr("NOTE") + tr(": ") + "</b>" +
      tr("GpgFrontend will restart automatically if you change the language!"));

  ui_->dataBox->setTitle(tr("Data"));
  ui_->clearAllDataObjectsButton->setText(
      tr("Clear All Data Objects (Total Size: %1)")
          .arg(GlobalSettingStation::GetInstance().GetDataObjectsFilesSize()));

  // Hide some options if running in sandbox, since they are not useful in
  // sandbox and may cause confusion to users.
  if (IsRunningInSandBox()) {
    ui_->dataBox->setHidden(true);
    ui_->modulePolicyComboBox->setHidden(true);
    ui_->modulePolicyLabel->setHidden(true);
    ui_->defaultWorkspaceAsLabel->setHidden(true);
    ui_->filePanelRadioButton->setHidden(true);
    ui_->textEditorRadioButton->setHidden(true);
    ui_->homePathAsDefaultPathcheckBox->setHidden(true);
    ui_->clearGpgPasswordCacheCheckBox->setHidden(true);
  }

  ui_->revealInFileExplorerButton->setText(tr("Reveal in File Explorer"));

  lang_ = SettingsDialog::ListLanguages();
  for (const auto& l : lang_) {
    ui_->langSelectBox->addItem(l);
  }
  connect(ui_->langSelectBox, qOverload<int>(&QComboBox::currentIndexChanged),
          this, &GeneralTab::SignalRestartNeeded);

  // Changing the default engine re-initializes the core, so a deep restart
  // (core + UI) is required for the new engine to take effect.
  connect(ui_->defaultEngineComboBox,
          qOverload<int>(&QComboBox::currentIndexChanged), this,
          &GeneralTab::SignalDeepRestartNeeded);

  connect(ui_->clearAllDataObjectsButton, &QPushButton::clicked, this, [=]() {
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(
        this, tr("Confirm"),
        tr("Are you sure you want to clear all data objects?\nThis will result "
           "in loss of all cached form positions, statuses, key servers, etc."),
        QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) {
      GlobalSettingStation::GetInstance().ClearAllDataObjects();
      ui_->clearAllDataObjectsButton->setText(
          tr("Clear All Data Objects (Total Size: %1)")
              .arg(GlobalSettingStation::GetInstance()
                       .GetDataObjectsFilesSize()));
    }
  });

  connect(ui_->revealInFileExplorerButton, &QPushButton::clicked, this, [=]() {
    QDesktopServices::openUrl(QUrl::fromLocalFile(
        GlobalSettingStation::GetInstance().GetAppDataPath()));
  });

  SetSettings();
}

void GeneralTab::SetSettings() {
  // Populating the controls below must not be mistaken for user edits that
  // would announce a restart.
  QSignalBlocker engine_blocker(ui_->defaultEngineComboBox);

  auto settings = GetSettings();

  auto default_workspace_as =
      settings.value("basic/default_workspace_as", "file_panel").toString();
  if (default_workspace_as == "file_panel") {
    ui_->filePanelRadioButton->setChecked(true);
  } else {
    ui_->textEditorRadioButton->setChecked(true);
  }
  auto home_path_as_file_panel_default_path =
      settings.value("basic/home_path_as_file_panel_default_path", true)
          .toBool();
  ui_->homePathAsDefaultPathcheckBox->setCheckState(
      home_path_as_file_panel_default_path ? Qt::Checked : Qt::Unchecked);

  auto clear_gpg_password_cache =
      settings.value("basic/clear_gpg_password_cache", true).toBool();
  ui_->clearGpgPasswordCacheCheckBox->setCheckState(
      clear_gpg_password_cache ? Qt::Checked : Qt::Unchecked);

  auto restore_text_editor_page =
      settings.value("basic/restore_text_editor_page", true).toBool();
  ui_->restoreTextEditorPageCheckBox->setCheckState(
      restore_text_editor_page ? Qt::Checked : Qt::Unchecked);

  auto confirm_import_keys =
      settings.value("basic/confirm_import_keys", false).toBool();
  ui_->importConfirmationCheckBox->setCheckState(
      confirm_import_keys ? Qt::Checked : Qt::Unchecked);

  auto module_loading_policy =
      settings.value("basic/module_loading_policy", "only_integrated")
          .toString();
  if (module_loading_policy == "all") {
    ui_->modulePolicyComboBox->setCurrentIndex(
        ui_->modulePolicyComboBox->findData("all"));
  } else if (module_loading_policy == "only_integrated") {
    ui_->modulePolicyComboBox->setCurrentIndex(
        ui_->modulePolicyComboBox->findData("only_integrated"));
  } else if (module_loading_policy == "disable") {
    ui_->modulePolicyComboBox->setCurrentIndex(
        ui_->modulePolicyComboBox->findData("disable"));
  } else {
    ui_->modulePolicyComboBox->findData("only_integrated");
  }

  auto default_engine =
      settings.value("basic/default_engine", "GNUPG").toString().toUpper();
  int engine_index = ui_->defaultEngineComboBox->findData(default_engine);
  if (engine_index != -1) {
    ui_->defaultEngineComboBox->setCurrentIndex(engine_index);
  } else {
    ui_->defaultEngineComboBox->setCurrentIndex(0);
  }

  // Stored in seconds; presented in minutes (rounded up so a sub-minute value
  // never collapses to 0, which would disable the cache).
  auto cache_ttl_secs =
      settings.value("engine/password_cache_ttl", 600).toLongLong();
  auto cache_max_ttl_secs =
      settings.value("engine/password_cache_max_ttl", 7200).toLongLong();
  ui_->rpgpCacheTtlSpinBox->setValue(
      static_cast<int>(qMax<qint64>(1, (cache_ttl_secs + 59) / 60)));
  ui_->rpgpCacheMaxTtlSpinBox->setValue(
      static_cast<int>(qMax<qint64>(1, (cache_max_ttl_secs + 59) / 60)));

  auto non_ascii_at_file_operation =
      settings.value("gnupg/non_ascii_at_file_operation", true).toBool();
  if (non_ascii_at_file_operation) {
    ui_->asciiModeCheckBox->setCheckState(Qt::Checked);
  }

  auto lang_key = settings.value("basic/lang").toString();
  auto lang_value = lang_.value(lang_key);
  if (!lang_.empty()) {
    ui_->langSelectBox->setCurrentIndex(
        ui_->langSelectBox->findText(lang_value));
  } else {
    ui_->langSelectBox->setCurrentIndex(0);
  }
}

void GeneralTab::ApplySettings() {
  auto settings = GpgFrontend::GetSettings();

  settings.setValue(
      "basic/default_workspace_as",
      ui_->filePanelRadioButton->isChecked() ? "file_panel" : "text_editor");

  settings.setValue("basic/home_path_as_file_panel_default_path",
                    ui_->homePathAsDefaultPathcheckBox->isChecked());
  settings.setValue("basic/clear_gpg_password_cache",
                    ui_->clearGpgPasswordCacheCheckBox->isChecked());
  settings.setValue("basic/restore_text_editor_page",
                    ui_->restoreTextEditorPageCheckBox->isChecked());
  settings.setValue("basic/confirm_import_keys",
                    ui_->importConfirmationCheckBox->isChecked());
  settings.setValue("basic/lang", lang_.key(ui_->langSelectBox->currentText()));
  settings.setValue("basic/module_loading_policy",
                    ui_->modulePolicyComboBox->currentData().toString());
  settings.setValue("gnupg/non_ascii_at_file_operation",
                    ui_->asciiModeCheckBox->isChecked());
  settings.setValue(
      "basic/default_engine",
      ui_->defaultEngineComboBox->currentData().toString().toUpper());

  // Convert the minute-based UI values back to seconds. The max cap can never
  // be shorter than the sliding window; clamp here so the stored values are
  // sane (the engine clamps again defensively).
  qint64 cache_ttl_secs =
      static_cast<qint64>(ui_->rpgpCacheTtlSpinBox->value()) * 60;
  qint64 cache_max_ttl_secs = qMax<qint64>(
      cache_ttl_secs,
      static_cast<qint64>(ui_->rpgpCacheMaxTtlSpinBox->value()) * 60);
  settings.setValue("engine/password_cache_ttl",
                    static_cast<qulonglong>(cache_ttl_secs));
  settings.setValue("engine/password_cache_max_ttl",
                    static_cast<qulonglong>(cache_max_ttl_secs));

  // Apply immediately so the new timeouts take effect without a restart.
  SetRpgpPasswordCacheTtl(static_cast<uint64_t>(cache_ttl_secs),
                          static_cast<uint64_t>(cache_max_ttl_secs));
}

}  // namespace GpgFrontend::UI
