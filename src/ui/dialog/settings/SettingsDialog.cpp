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

#include "SettingsDialog.h"

#include "core/GFConstants.h"
#include "core/function/GlobalSettingStation.h"
#include "core/utils/CommonUtils.h"
#include "ui/dialog/settings/SettingsAppearance.h"
#include "ui/dialog/settings/SettingsGeneral.h"
#include "ui/dialog/settings/SettingsGnuPG.h"
#include "ui/dialog/settings/SettingsKeyDatabases.h"
#include "ui/dialog/settings/SettingsNetwork.h"
#include "ui/dialog/settings/SettingsRpgp.h"
#include "ui/main_window/MainWindow.h"

namespace GpgFrontend::UI {

SettingsDialog::SettingsDialog(QWidget* parent)
    : GeneralDialog(typeid(SettingsDialog).name(), parent) {
  tab_widget_ = new QTabWidget();
  general_tab_ = new GeneralTab();
  appearance_tab_ = new AppearanceTab();
  network_tab_ = new NetworkTab();
  key_dbs_tab_ = new KeyDatabasesTab();
  gnupg_tab_ = new GnuPGTab();
  rpgp_tab_ = new RpgpTab();

  auto* main_layout = new QVBoxLayout();
  main_layout->addWidget(tab_widget_);
  main_layout->stretch(0);

  tab_widget_->addTab(general_tab_, tr("General"));
  tab_widget_->addTab(appearance_tab_, tr("Appearance"));

  // network settings is not available in sandbox environment, so only add the
  // tab when not running in sandbox
  if (!IsRunningInSandBox()) {
    tab_widget_->addTab(network_tab_, tr("Network"));
  }

  tab_widget_->addTab(key_dbs_tab_, tr("Key Databases"));

  if (GetGSS().IsEngineSupported(OpenPGPEngine::kGNUPG)) {
    tab_widget_->addTab(gnupg_tab_, tr("GnuPG"));
  }

  if (GetGSS().IsEngineSupported(OpenPGPEngine::kRPGP)) {
    tab_widget_->addTab(rpgp_tab_, tr("rPGP"));
  }

#ifdef Q_OS_MACOS
  connect(this, &QDialog::finished, this, &SettingsDialog::SlotAccept);
  setWindowTitle(tr("Preference"));
#else
  button_box_ =
      new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  connect(button_box_, &QDialogButtonBox::accepted, this,
          &SettingsDialog::SlotAccept);
  connect(button_box_, &QDialogButtonBox::rejected, this,
          &SettingsDialog::reject);
  main_layout->addWidget(button_box_);
  main_layout->stretch(0);
  setWindowTitle(tr("Settings"));
#endif

  setLayout(main_layout);

  // restart ui
  connect(general_tab_, &GeneralTab::SignalRestartNeeded, this,
          &SettingsDialog::slot_declare_a_restart);

  // restart core and ui
  connect(general_tab_, &GeneralTab::SignalDeepRestartNeeded, this,
          &SettingsDialog::slot_declare_a_deep_restart);

  // restart core and ui
  connect(appearance_tab_, &AppearanceTab::SignalRestartNeeded, this,
          &SettingsDialog::slot_declare_a_restart);

  connect(key_dbs_tab_, &KeyDatabasesTab::SignalDeepRestartNeeded, this,
          &SettingsDialog::slot_declare_a_deep_restart);

  connect(gnupg_tab_, &GnuPGTab::SignalDeepRestartNeeded, this,
          &SettingsDialog::slot_declare_a_deep_restart);

  // announce main window
  connect(this, &SettingsDialog::SignalRestartNeeded,
          qobject_cast<MainWindow*>(parent), &MainWindow::SlotSetRestartNeeded);

  this->show();
  this->raise();
  this->activateWindow();
}

void SettingsDialog::showEvent(QShowEvent* event) {
  GeneralDialog::showEvent(event);

  // If the window state has not been restored, move the dialog to the center of
  // the parent window (if has parent) or the screen.
  if (!isRectRestored()) {
    movePosition2CenterOfParent();
  }
}

void SettingsDialog::slot_declare_a_restart() {
  if (restart_mode_ < kRestartCode) {
    this->restart_mode_ = kRestartCode;
  }
}

void SettingsDialog::slot_declare_a_deep_restart() {
  if (restart_mode_ < kDeepRestartCode) {
    this->restart_mode_ = kDeepRestartCode;
  }
}

void SettingsDialog::SlotAccept() {
  general_tab_->ApplySettings();
  appearance_tab_->ApplySettings();
  network_tab_->ApplySettings();
  key_dbs_tab_->ApplySettings();

  if (GetGSS().IsEngineSupported(OpenPGPEngine::kRPGP)) {
    rpgp_tab_->ApplySettings();
  }

  emit SignalAppearanceChanged();

  LOG_D() << "flushing qt event loop to ensure all pending events are "
             "processed before applying the settings";
  QCoreApplication::sendPostedEvents(nullptr, 0);
  QCoreApplication::processEvents(QEventLoop::AllEvents);

  if (restart_mode_ != kNonRestartCode) {
    emit SignalRestartNeeded(restart_mode_);
  }

  close();
}

auto SettingsDialog::ListLanguages() -> QHash<QString, QString> {
  QHash<QString, QString> languages;
  languages.insert(QString(), tr("System Default"));

  auto filenames = QDir(QLatin1String(":/i18n")).entryList();
  for (const auto& file : filenames) {
    auto start = file.indexOf('.') + 1;
    auto end = file.lastIndexOf('.');
    if (start < 0 || end < 0 || start >= end) continue;

    auto locale = file.mid(start, end - start);
    QLocale const q_locale(locale);

#if QT_VERSION >= QT_VERSION_CHECK(6, 2, 0)
    if (q_locale.nativeTerritoryName().isEmpty()) continue;
#else
    if (q_locale.nativeCountryName().isEmpty()) continue;
#endif

    auto language = q_locale.nativeLanguageName() + " (" + locale + ")";
    languages.insert(q_locale.name(), language);
  }
  return languages;
}

}  // namespace GpgFrontend::UI
