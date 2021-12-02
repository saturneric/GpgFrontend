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

#ifdef SMTP_SUPPORT
#include "SettingsSendMail.h"
#endif

namespace GpgFrontend::UI {

SettingsDialog::SettingsDialog(QWidget* parent) : QDialog(parent) {
  tabWidget = new QTabWidget;
  generalTab = new GeneralTab();
  appearanceTab = new AppearanceTab;
#ifdef SMTP_SUPPORT
  sendMailTab = new SendMailTab;
#endif
  keyserverTab = new KeyserverTab;
  advancedTab = new AdvancedTab;
  gpgPathsTab = new GpgPathsTab;

  tabWidget->addTab(generalTab, _("General"));
  tabWidget->addTab(appearanceTab, _("Appearance"));
#ifdef SMTP_SUPPORT
  tabWidget->addTab(sendMailTab, _("Send Mail"));
#endif
  tabWidget->addTab(keyserverTab, _("Key Server"));
  // tabWidget->addTab(gpgPathsTab, _("Gpg paths"));
  tabWidget->addTab(advancedTab, _("Advanced"));

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
  connect(advancedTab, SIGNAL(signalRestartNeeded(bool)), this,
          SLOT(slotSetRestartNeeded(bool)));

  connect(this, SIGNAL(signalRestartNeeded(bool)), parent,
          SLOT(slotSetRestartNeeded(bool)));

  this->resize(480, 640);
  this->show();
}

bool SettingsDialog::getRestartNeeded() const { return this->restartNeeded; }

void SettingsDialog::slotSetRestartNeeded(bool needed) {
  this->restartNeeded = needed;
}

void SettingsDialog::slotAccept() {
  generalTab->applySettings();
#ifdef SMTP_SUPPORT
  sendMailTab->applySettings();
#endif
  appearanceTab->applySettings();
  keyserverTab->applySettings();
  advancedTab->applySettings();
  gpgPathsTab->applySettings();
  if (getRestartNeeded()) {
    emit signalRestartNeeded(true);
  }
  close();
}

// http://www.informit.com/articles/article.aspx?p=1405555&seqNum=3
// http://developer.qt.nokia.com/wiki/How_to_create_a_multi_language_application
QHash<QString, QString> SettingsDialog::listLanguages() {
  QHash<QString, QString> languages;

  languages.insert(QString(), _("System Default"));

  auto locale_path = GlobalSettingStation::GetInstance().GetLocaleDir();

  auto locale_dir = QDir(locale_path.c_str());
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

GpgPathsTab::GpgPathsTab(QWidget* parent)
    : QWidget(parent),
      appPath(qApp->applicationDirPath()),
      settings(RESOURCE_DIR(appPath) + "/conf/gpgfrontend.ini",
               QSettings::IniFormat) {
  setSettings();

  /*****************************************
   * Keydb Box
   *****************************************/
  auto* keydbBox = new QGroupBox(_("Relative path to Key Database"));
  auto* keydbBoxLayout = new QGridLayout();

  // Label containing the current keydbpath relative to default keydb path
  keydbLabel = new QLabel(accKeydbPath, this);

  auto* keydbButton = new QPushButton(_("Change Key Database path"), this);
  connect(keydbButton, SIGNAL(clicked()), this, SLOT(chooseKeydbDir()));
  auto* keydbDefaultButton =
      new QPushButton(_("Set Key Database to default path"), this);
  connect(keydbDefaultButton, SIGNAL(clicked()), this,
          SLOT(setKeydbPathToDefault()));

  keydbBox->setLayout(keydbBoxLayout);
  keydbBoxLayout->addWidget(
      new QLabel(QString(_("Current Key Database path")) + ": "), 1, 1);
  keydbBoxLayout->addWidget(keydbLabel, 1, 2);
  keydbBoxLayout->addWidget(keydbButton, 1, 3);
  keydbBoxLayout->addWidget(keydbDefaultButton, 2, 3);
  keydbBoxLayout->addWidget(
      new QLabel(QString("<b>") + _("NOTE") + ": </b> " +
                 _("GpgFrontend will restart automatically if you change the "
                   "Key Database path!")),
      3, 1, 1, 3);

  auto* mainLayout = new QVBoxLayout;
  mainLayout->addWidget(keydbBox);
  mainLayout->addStretch(1);
  setLayout(mainLayout);
}

QString GpgPathsTab::getRelativePath(const QString& dir1, const QString& dir2) {
  QDir dir(dir1);
  QString s;

  s = dir.relativeFilePath(dir2);
  qDebug() << "relative path: " << s;
  if (s.isEmpty()) {
    s = ".";
  }
  return s;
}

void GpgPathsTab::setKeydbPathToDefault() {
  accKeydbPath = ".";
  keydbLabel->setText(".");
}

QString GpgPathsTab::chooseKeydbDir() {
  QString dir = QFileDialog::getExistingDirectory(
      this, _("Choose keydb directory"), accKeydbPath,
      QFileDialog::ShowDirsOnly);

  accKeydbPath = getRelativePath(defKeydbPath, dir);
  keydbLabel->setText(accKeydbPath);
  return {};
}

void GpgPathsTab::setSettings() {
  defKeydbPath = qApp->applicationDirPath() + "/keydb";

  accKeydbPath = settings.value("gpgpaths/keydbpath").toString();
  if (accKeydbPath.isEmpty()) {
    accKeydbPath = ".";
  }
}

void GpgPathsTab::applySettings() {
  settings.setValue("gpgpaths/keydbpath", accKeydbPath);
}

}  // namespace GpgFrontend::UI
