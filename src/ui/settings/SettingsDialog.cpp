/**
 * This file is part of GPGFrontend.
 *
 * GPGFrontend is free software: you can redistribute it and/or modify
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

#include "ui/SettingsDialog.h"
#include "ui/WaitingDialog.h"

SettingsDialog::SettingsDialog(GpgFrontend::GpgContext *ctx, QWidget *parent)
        : QDialog(parent) {
    mCtx = ctx;
    tabWidget = new QTabWidget;
    generalTab = new GeneralTab(mCtx);
    appearanceTab = new AppearanceTab;
    sendMailTab = new SendMailTab;
    keyserverTab = new KeyserverTab;
    advancedTab = new AdvancedTab;
    gpgPathsTab = new GpgPathsTab;

    tabWidget->addTab(generalTab, tr("General"));
    tabWidget->addTab(appearanceTab, tr("Appearance"));
    tabWidget->addTab(sendMailTab, tr("Send Mail"));
    tabWidget->addTab(keyserverTab, tr("Key Server"));
    // tabWidget->addTab(gpgPathsTab, tr("Gpg paths"));
    tabWidget->addTab(advancedTab, tr("Advanced"));

    buttonBox =
            new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    connect(buttonBox, SIGNAL(accepted()), this, SLOT(slotAccept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    auto *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(tabWidget);
    mainLayout->stretch(0);
    mainLayout->addWidget(buttonBox);
    mainLayout->stretch(0);
    setLayout(mainLayout);

    setWindowTitle(tr("Settings"));

    // slots for handling the restartneeded member
    this->slotSetRestartNeeded(false);
    connect(generalTab, SIGNAL(signalRestartNeeded(bool)), this,
            SLOT(slotSetRestartNeeded(bool)));
    connect(appearanceTab, SIGNAL(signalRestartNeeded(bool)), this,
            SLOT(slotSetRestartNeeded(bool)));
    connect(sendMailTab, SIGNAL(signalRestartNeeded(bool)), this,
            SLOT(slotSetRestartNeeded(bool)));
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
    sendMailTab->applySettings();
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

    languages.insert("", tr("System Default"));

    QString appPath = qApp->applicationDirPath();
    QDir qmDir = QDir(RESOURCE_DIR(appPath) + "/ts/");
    QStringList fileNames = qmDir.entryList(QStringList("gpgfrontend_*.qm"));

    for (int i = 0; i < fileNames.size(); ++i) {
        QString locale = fileNames[i];
        locale.truncate(locale.lastIndexOf('.'));
        locale.remove(0, locale.indexOf('_') + 1);

        // this works in qt 4.8
        QLocale qloc(locale);
#if QT_VERSION < 0x040800
        QString language =
            QLocale::languageToString(qloc.language()) + " (" + locale +
            ")"; //+ " (" + QLocale::languageToString(qloc.language()) + ")";
#else
        QString language = qloc.nativeLanguageName() + " (" + locale + ")";
#endif
        languages.insert(locale, language);
    }
    return languages;
}

GpgPathsTab::GpgPathsTab(QWidget *parent)
        : QWidget(parent), appPath(qApp->applicationDirPath()),
          settings(RESOURCE_DIR(appPath) + "/conf/gpgfrontend.ini",
                   QSettings::IniFormat) {
    setSettings();

    /*****************************************
     * Keydb Box
     *****************************************/
    auto *keydbBox = new QGroupBox(tr("Relative path to keydb"));
    auto *keydbBoxLayout = new QGridLayout();

    // Label containing the current keydbpath relative to default keydb path
    keydbLabel = new QLabel(accKeydbPath, this);

    auto *keydbButton = new QPushButton("Change keydb path", this);
    connect(keydbButton, SIGNAL(clicked()), this, SLOT(chooseKeydbDir()));
    auto *keydbDefaultButton = new QPushButton("Set keydb to default path", this);
    connect(keydbDefaultButton, SIGNAL(clicked()), this,
            SLOT(setKeydbPathToDefault()));

    keydbBox->setLayout(keydbBoxLayout);
    keydbBoxLayout->addWidget(new QLabel(tr("Current keydb path: ")), 1, 1);
    keydbBoxLayout->addWidget(keydbLabel, 1, 2);
    keydbBoxLayout->addWidget(keydbButton, 1, 3);
    keydbBoxLayout->addWidget(keydbDefaultButton, 2, 3);
    keydbBoxLayout->addWidget(
            new QLabel(tr("<b>NOTE: </b> Gpg4usb will restart automatically if you "
                          "change the keydb path!")),
            3, 1, 1, 3);

    auto *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(keydbBox);
    mainLayout->addStretch(1);
    setLayout(mainLayout);
}

QString GpgPathsTab::getRelativePath(const QString &dir1, const QString &dir2) {
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
            this, tr("Choose keydb directory"), accKeydbPath,
            QFileDialog::ShowDirsOnly);

    accKeydbPath = getRelativePath(defKeydbPath, dir);
    keydbLabel->setText(accKeydbPath);
    return "";
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
