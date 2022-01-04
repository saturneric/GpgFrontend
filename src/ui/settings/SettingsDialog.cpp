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

#include "SettingsDialog.h"

#include "GlobalSettingStation.h"
#include "SettingsAdvanced.h"
#include "SettingsAppearance.h"
#include "SettingsGeneral.h"
#include "SettingsKeyServer.h"
#include "SettingsNetwork.h"

#ifdef SMTP_SUPPORT
#include "SettingsSendMail.h"
#endif

namespace GpgFrontend::UI {

SettingsDialog::SettingsDialog(QWidget* parent) : QDialog(parent) {

  tabWidget = new QTabWidget();
  generalTab = new GeneralTab();
  appearanceTab = new AppearanceTab();
#ifdef SMTP_SUPPORT
  sendMailTab = new SendMailTab();
#endif
  keyserverTab = new KeyserverTab();
  networkTab = new NetworkTab();
#ifdef ADVANCED_SUPPORT
  advancedTab = new AdvancedTab;
#endif

  tabWidget->addTab(generalTab, _("General"));
  tabWidget->addTab(appearanceTab, _("Appearance"));
#ifdef SMTP_SUPPORT
  tabWidget->addTab(sendMailTab, _("Send Mail"));
#endif
  tabWidget->addTab(keyserverTab, _("Key Server"));
  // tabWidget->addTab(gpgPathsTab, _("Gpg paths"));
  tabWidget->addTab(networkTab, _("Network"));
#ifdef ADVANCED_SUPPORT
  tabWidget->addTab(advancedTab, _("Advanced"));
#endif

  buttonBox =
      new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

  connect(buttonBox, SIGNAL(accepted()), this, SLOT(slotAccept()));
  connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

  auto* mainLayout = new QVBoxLayout;
  mainLayout->addWidget(tabWidget);
  mainLayout->stretch(0);
  mainLayout->addWidget(buttonBox);
  mainLayout->stretch(0);
  setLayout(mainLayout);

  setWindowTitle(_("Settings"));

  // slots for handling the restartneeded member
  this->slotSetRestartNeeded(false);
  connect(generalTab, SIGNAL(signalRestartNeeded(bool)), this,
          SLOT(slotSetRestartNeeded(bool)));
  connect(appearanceTab, SIGNAL(signalRestartNeeded(bool)), this,
          SLOT(slotSetRestartNeeded(bool)));
#ifdef SMTP_SUPPORT
  connect(sendMailTab, SIGNAL(signalRestartNeeded(bool)), this,
          SLOT(slotSetRestartNeeded(bool)));
#endif
  connect(keyserverTab, SIGNAL(signalRestartNeeded(bool)), this,
          SLOT(slotSetRestartNeeded(bool)));
#ifdef ADVANCED_SUPPORT
  connect(advancedTab, SIGNAL(signalRestartNeeded(bool)), this,
          SLOT(slotSetRestartNeeded(bool)));
#endif

  connect(this, SIGNAL(signalRestartNeeded(bool)), parent,
          SLOT(slotSetRestartNeeded(bool)));

  this->setMinimumSize(480, 680);
  this->adjustSize();
  this->show();
}

bool SettingsDialog::getRestartNeeded() const { return this->restartNeeded; }

void SettingsDialog::slotSetRestartNeeded(bool needed) {
  this->restartNeeded = needed;
}

void SettingsDialog::slotAccept() {
  LOG(INFO) << "Called";

  generalTab->applySettings();
#ifdef SMTP_SUPPORT
  sendMailTab->applySettings();
#endif
  appearanceTab->applySettings();
  keyserverTab->applySettings();
  networkTab->applySettings();
#ifdef ADVANCED_SUPPORT
  advancedTab->applySettings();
#endif

  LOG(INFO) << "apply done";

  // write settings to filesystem
  GlobalSettingStation::GetInstance().Sync();

  LOG(INFO) << "restart needed" << getRestartNeeded();
  if (getRestartNeeded()) {
    emit signalRestartNeeded(true);
  }
  close();
}

QHash<QString, QString> SettingsDialog::listLanguages() {
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
