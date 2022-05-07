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

#include "SettingsDialog.h"

#include "core/function/GlobalSettingStation.h"
#include "ui/settings/SettingsAdvanced.h"
#include "ui/settings/SettingsAppearance.h"
#include "ui/settings/SettingsGeneral.h"
#include "ui/settings/SettingsKeyServer.h"
#include "ui/settings/SettingsNetwork.h"
#include "ui/main_window/MainWindow.h"

namespace GpgFrontend::UI {

SettingsDialog::SettingsDialog(QWidget* parent) : QDialog(parent) {
  tab_widget_ = new QTabWidget();
  general_tab_ = new GeneralTab();
  appearance_tab_ = new AppearanceTab();
  key_server_tab_ = new KeyserverTab();
  network_tab_ = new NetworkTab();

  tab_widget_->addTab(general_tab_, _("General"));
  tab_widget_->addTab(appearance_tab_, _("Appearance"));
  tab_widget_->addTab(key_server_tab_, _("Key Server"));
  tab_widget_->addTab(network_tab_, _("Network"));

  button_box_ =
      new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

  connect(button_box_, &QDialogButtonBox::accepted, this, &SettingsDialog::SlotAccept);
  connect(button_box_, &QDialogButtonBox::rejected, this, &SettingsDialog::reject);

  auto* mainLayout = new QVBoxLayout;
  mainLayout->addWidget(tab_widget_);
  mainLayout->stretch(0);
  mainLayout->addWidget(button_box_);
  mainLayout->stretch(0);
  setLayout(mainLayout);

  setWindowTitle(_("Settings"));

  // slots for handling the restart needed member
  this->slot_set_restart_needed(false);
  connect(general_tab_, &GeneralTab::SignalRestartNeeded, this,
          &SettingsDialog::slot_set_restart_needed);
  connect(this, &SettingsDialog::SignalRestartNeeded, qobject_cast<MainWindow *>(parent), &MainWindow::SlotSetRestartNeeded);

  this->setMinimumSize(480, 680);
  this->adjustSize();
  this->show();
}

bool SettingsDialog::get_restart_needed() const {
  return this->restart_needed_;
}

void SettingsDialog::slot_set_restart_needed(bool needed) {
  this->restart_needed_ = needed;
}

void SettingsDialog::SlotAccept() {
  LOG(INFO) << "Called";

  general_tab_->ApplySettings();
  appearance_tab_->ApplySettings();
  key_server_tab_->ApplySettings();
  network_tab_->ApplySettings();

  LOG(INFO) << "apply done";

  // write settings to filesystem
  GlobalSettingStation::GetInstance().SyncSettings();

  LOG(INFO) << "restart needed" << get_restart_needed();
  if (get_restart_needed()) {
    emit SignalRestartNeeded(true);
  }
  close();
}

QHash<QString, QString> SettingsDialog::ListLanguages() {
  QHash<QString, QString> languages;

  languages.insert(QString(), _("System Default"));

  auto locale_path = GlobalSettingStation::GetInstance().GetLocaleDir();

  auto locale_dir = QDir(QString::fromStdString(locale_path.string()));
  QStringList file_names = locale_dir.entryList(QStringList("*"));

  for (int i = 0; i < file_names.size(); ++i) {
    QString locale = file_names[i];
    LOG(INFO) << "locale" << locale.toStdString();
    if (locale == "." || locale == "..") continue;

    // this works in qt 4.8
    QLocale q_locale(locale);
    if (q_locale.nativeCountryName().isEmpty()) continue;
#if QT_VERSION < 0x040800
    QString language =
        QLocale::languageToString(q_locale.language()) + " (" + locale +
        ")";  //+ " (" + QLocale::languageToString(q_locale.language()) + ")";
#else
    auto language = q_locale.nativeLanguageName() + " (" + locale + ")";
#endif
    languages.insert(locale, language);
  }
  return languages;
}

}  // namespace GpgFrontend::UI
