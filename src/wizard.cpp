/* *      wizard.cpp
 *
 *      Copyright 2008 gpg4usb-team <gpg4usb@cpunk.de>
 *
 *      This file is part of gpg4usb.
 *
 *      Gpg4usb is free software: you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation, either version 3 of the License, or
 *      (at your option) any later version.
 *
 *      Gpg4usb is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with gpg4usb.  If not, see <http://www.gnu.org/licenses/>
 */

#include "wizard.h"

#ifdef Q_OS_WIN
#include "windows.h"
#endif

Wizard::Wizard(GpgME::GpgContext *ctx, KeyMgmt *keyMgmt, QWidget *parent)
        : QWizard(parent) {
    mCtx = ctx;
    mKeyMgmt = keyMgmt;

    setPage(Page_Intro, new IntroPage(this));
    setPage(Page_Choose, new ChoosePage(this));
    setPage(Page_ImportFromGpg4usb, new ImportFromGpg4usbPage(mCtx, mKeyMgmt, this));
    setPage(Page_ImportFromGnupg, new ImportFromGnupgPage(mCtx, mKeyMgmt, this));
    setPage(Page_GenKey, new KeyGenPage(mCtx, this));
    setPage(Page_Conclusion, new ConclusionPage(this));
#ifndef Q_WS_MAC
    setWizardStyle(ModernStyle);
#endif
    setWindowTitle(tr("First Start Wizard"));

    // http://www.flickr.com/photos/laureenp/6141822934/
    setPixmap(QWizard::WatermarkPixmap, QPixmap(":/keys2.jpg"));
    setPixmap(QWizard::LogoPixmap, QPixmap(":/logo_small.png"));
    setPixmap(QWizard::BannerPixmap, QPixmap(":/banner.png"));

    QSettings settings;
    setStartId(settings.value("wizard/nextPage", -1).toInt());
    settings.remove("wizard/nextPage");

    connect(this, SIGNAL(accepted()), this, SLOT(slotWizardAccepted()));
    connect(this, SIGNAL(signalOpenHelp(QString)), parentWidget(), SLOT(signalOpenHelp(QString)));

}

void Wizard::slotWizardAccepted() {
    QSettings settings;
    // Don't show is mapped to show -> negation
    settings.setValue("wizard/showWizard", !field("showWizard").toBool());

    if (field("openHelp").toBool()) {
        emit signalOpenHelp("docu.html#content");
    }
}

bool Wizard::importPubAndSecKeysFromDir(const QString &dir, KeyMgmt *keyMgmt) {
    QFile secRingFile(dir + "/secring.gpg");
    QFile pubRingFile(dir + "/pubring.gpg");

    // Return, if no keyrings are found in subdir of chosen dir
    if (!(pubRingFile.exists() or secRingFile.exists())) {
        QMessageBox::critical(0, tr("Import Error"), tr("Couldn't locate any keyring file in %1").arg(dir));
        return false;
    }

    QByteArray inBuffer;
    if (secRingFile.exists()) {
        // write content of secringfile to inBuffer
        if (!secRingFile.open(QIODevice::ReadOnly)) {
            QMessageBox::critical(nullptr, tr("Import error"),
                                  tr("Couldn't open private keyringfile: %1").arg(secRingFile.fileName()));
            return false;
        }
        inBuffer = secRingFile.readAll();
        secRingFile.close();
    }

    if (pubRingFile.exists()) {
        // try to import public keys
        if (!pubRingFile.open(QIODevice::ReadOnly)) {
            QMessageBox::critical(nullptr, tr("Import error"),
                                  tr("Couldn't open public keyringfile: %1").arg(pubRingFile.fileName()));
            return false;
        }
        inBuffer.append(pubRingFile.readAll());
        pubRingFile.close();
    }
    keyMgmt->slotImportKeys(inBuffer);
    inBuffer.clear();

    return true;
}

IntroPage::IntroPage(QWidget *parent)
        : QWizardPage(parent) {
    setTitle(tr("Getting started..."));
    setSubTitle(tr("... with gpg4usb"));

    auto *topLabel = new QLabel(tr("To use gpg4usb for decrypting and signing messages, you need a "
                                   "private key. The next page will help you with "
                                   "key generation or import.<br><br>"
                                   "For more information have a look at the <a href='docu_concepts.html'>concepts</a> "
                                   "(by clicking the link, the page will open in the main window). <br>"));
    topLabel->setWordWrap(true);
    connect(topLabel, SIGNAL(linkActivated(QString)), parentWidget()->parentWidget(), SLOT(openHelp(QString)));

    // QComboBox for language selection
    auto *langLabel = new QLabel(tr("Choose a Language"));
    langLabel->setWordWrap(true);

    languages = SettingsDialog::listLanguages();
    auto *langSelectBox = new QComboBox();

            foreach(QString l, languages) {
            langSelectBox->addItem(l);
        }
    // selected entry from config
    QSettings settings;
    QString langKey = settings.value("int/lang").toString();
    QString langValue = languages.value(langKey);
    if (langKey != "") {
        langSelectBox->setCurrentIndex(langSelectBox->findText(langValue));
    }

    connect(langSelectBox, SIGNAL(currentIndexChanged(QString)), this, SLOT(slotLangChange(QString)));

    // set layout and add widgets
    auto *layout = new QVBoxLayout;
    layout->addWidget(topLabel);
    layout->addWidget(langLabel);
    layout->addWidget(langSelectBox);
    setLayout(layout);
}

void IntroPage::slotLangChange(const QString &lang) {
    QSettings settings;
    settings.setValue("int/lang", languages.key(lang));
    settings.setValue("wizard/nextPage", this->wizard()->currentId());
    qApp->exit(RESTART_CODE);
}

int IntroPage::nextId() const {
    return Wizard::Page_Choose;
}

ChoosePage::ChoosePage(QWidget *parent)
        : QWizardPage(parent) {
    setTitle(tr("Choose your action..."));
    setSubTitle(tr("...by clicking on the apropriate link."));

    auto *keygenLabel = new QLabel(tr("If you have never used gpg4usb before and also don't own a gpg key yet you "
                                      "may possibly want to ") + "<a href=""Wizard::Page_GenKey"">"
                                   + tr("create a new keypair") + "</a><hr>");
    keygenLabel->setWordWrap(true);
    connect(keygenLabel, SIGNAL(linkActivated(QString)), this, SLOT(slotJumpPage(Qtring)));

    auto *importGpg4usbLabel = new QLabel(tr("If you upgrade from an older version of gpg4usb you may want to ")
                                          + "<a href=""Wizard::Page_ImportFromGpg4usb"">"
                                          + tr("import settings and/or keys from gpg4usb") + "</a>");
    importGpg4usbLabel->setWordWrap(true);
    connect(importGpg4usbLabel, SIGNAL(linkActivated(QString)), this, SLOT(slotJumpPage(QString)));

    auto *importGnupgLabel = new QLabel(tr("If you are already using GnuPG you may want to ")
                                        + "<a href=""Wizard::Page_ImportFromGnupg"">"
                                        + tr("import keys from GnuPG") + "</a><hr>");
    importGnupgLabel->setWordWrap(true);
    connect(importGnupgLabel, SIGNAL(linkActivated(QString)), this, SLOT(slotJumpPage(QString)));

    auto *layout = new QVBoxLayout();
    layout->addWidget(keygenLabel);
    layout->addWidget(importGnupgLabel);
    layout->addWidget(importGpg4usbLabel);
    setLayout(layout);
    nextPage = Wizard::Page_Conclusion;
}

int ChoosePage::nextId() const {
    return nextPage;
}

void ChoosePage::slotJumpPage(const QString &page) {
    QMetaObject qmo = Wizard::staticMetaObject;
    int index = qmo.indexOfEnumerator("WizardPages");
    QMetaEnum m = qmo.enumerator(index);

    nextPage = m.keyToValue(page.toUtf8().data());
    wizard()->next();
}

ImportFromGpg4usbPage::ImportFromGpg4usbPage(GpgME::GpgContext *ctx, KeyMgmt *keyMgmt, QWidget *parent)
        : QWizardPage(parent) {
    mCtx = ctx;
    mKeyMgmt = keyMgmt;
    setTitle(tr("Import from..."));
    setSubTitle(tr("...existing gpg4usb"));

    auto *topLabel = new QLabel(tr("You can import keys and/or settings from existing gpg4usb. <br><br>"
                                   "Just check what you want to import, click the import button and choose "
                                   "the directory of your other gpg4usb in the appearing file dialog."), this);
    topLabel->setWordWrap(true);

    gpg4usbKeyCheckBox = new QCheckBox();
    gpg4usbKeyCheckBox->setChecked(true);
    auto *keyLabel = new QLabel(tr("Keys"));

    gpg4usbConfigCheckBox = new QCheckBox();
    gpg4usbConfigCheckBox->setChecked(true);
    auto *configLabel = new QLabel(tr("Configuration"));

    auto *importFromGpg4usbButton = new QPushButton(tr("Import from gpg4usb"));
    connect(importFromGpg4usbButton, SIGNAL(clicked()), this, SLOT(slotImportFromOlderGpg4usb()));

    auto *gpg4usbLayout = new QGridLayout();
    gpg4usbLayout->addWidget(topLabel, 1, 1, 1, 2);
    gpg4usbLayout->addWidget(gpg4usbKeyCheckBox, 2, 1, Qt::AlignRight);
    gpg4usbLayout->addWidget(keyLabel, 2, 2);
    gpg4usbLayout->addWidget(gpg4usbConfigCheckBox, 3, 1, Qt::AlignRight);
    gpg4usbLayout->addWidget(configLabel, 3, 2);
    gpg4usbLayout->addWidget(importFromGpg4usbButton, 4, 2);

    this->setLayout(gpg4usbLayout);
}

void ImportFromGpg4usbPage::slotImportFromOlderGpg4usb() {
    QString dir = QFileDialog::getExistingDirectory(this, tr("Other gpg4usb directory"));

    // Return, if cancel was hit
    if (dir.isEmpty()) {
        return;
    }

    // try to import keys, if appropriate box is checked, return, if import was unsuccessful
    if (gpg4usbKeyCheckBox->isChecked()) {
        if (!Wizard::importPubAndSecKeysFromDir(dir + "/keydb", mKeyMgmt)) {
            return;
        }
    }

    // try to import config, if appropriate box is checked
    if (gpg4usbConfigCheckBox->isChecked()) {
        slotImportConfFromGpg4usb(dir);

        QSettings settings;
        settings.setValue("wizard/nextPage", this->nextId());
        QMessageBox::information(nullptr, tr("Configuration Imported"),
                                 tr("Imported Configuration from old gpg4usb.<br>"
                                    "Will now restart to activate the configuration."));
        // TODO: edit->maybesave?
        qApp->exit(RESTART_CODE);
    }
    wizard()->next();
}

bool ImportFromGpg4usbPage::slotImportConfFromGpg4usb(const QString &dir) {
    QString path = dir + "/conf/gpg4usb.ini";
    QSettings oldconf(path, QSettings::IniFormat, this);
    QSettings actualConf;
            foreach(QString key, oldconf.allKeys()) {
            actualConf.setValue(key, oldconf.value(key));
        }
    return true;
}

int ImportFromGpg4usbPage::nextId() const {
    return Wizard::Page_Conclusion;
}

ImportFromGnupgPage::ImportFromGnupgPage(GpgME::GpgContext *ctx, KeyMgmt *keyMgmt, QWidget *parent)
        : QWizardPage(parent) {
    mCtx = ctx;
    mKeyMgmt = keyMgmt;
    setTitle(tr("Import keys..."));
    setSubTitle(tr("...from existing GnuPG installation"));

    auto *gnupgLabel = new QLabel(tr("You can import keys from a locally installed GnuPG.<br><br> The location is read "
                                     "from registry in Windows or assumed to be the .gnupg folder in the your home directory in Linux.<br>"));
    gnupgLabel->setWordWrap(true);

    importFromGnupgButton = new QPushButton(tr("Import keys from GnuPG"));
    connect(importFromGnupgButton, SIGNAL(clicked()), this, SLOT(slotrImportKeysFromGnupg()));

    auto *layout = new QGridLayout();
    layout->addWidget(gnupgLabel);
    layout->addWidget(importFromGnupgButton);

    this->setLayout(layout);
}

void ImportFromGnupgPage::slotrImportKeysFromGnupg() {
    // first get gnupghomedir and check, if it exists
    QString gnuPGHome = getGnuPGHome();
    if (gnuPGHome == nullptr) {
        QMessageBox::critical(0, tr("Import Error"), tr("Couldn't locate GnuPG home directory"));
        return;
    }

    // Try to import the keyring files and return the return value of the method
    Wizard::importPubAndSecKeysFromDir(gnuPGHome, mKeyMgmt);
    wizard()->next();
}

QString ImportFromGnupgPage::getGnuPGHome() {
    QString gnuPGHome = "";
#ifdef _WIN32
    bool existsAndSuccess = false;

    HKEY hKey;

    existsAndSuccess = (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\GNU\\GNUPG", 0, KEY_READ, &hKey) == ERROR_SUCCESS);

    if (existsAndSuccess) {
        QSettings gnuPGsettings("HKEY_CURRENT_USER\\Software\\GNU\\GNUPG", QSettings::NativeFormat);
            if (gnuPGsettings.contains("HomeDir")) {
                gnuPGHome = gnuPGsettings.value("HomeDir").toString();
            } else {
                return NULL;
            }
        }
#else
    gnuPGHome = QDir::homePath() + "/.gnupg";
    if (!QFile(gnuPGHome).exists()) {
        return nullptr;
    }
#endif

    return gnuPGHome;
}

int ImportFromGnupgPage::nextId() const {
    return Wizard::Page_Conclusion;
}

KeyGenPage::KeyGenPage(GpgME::GpgContext *ctx, QWidget *parent)
        : QWizardPage(parent) {
    mCtx = ctx;
    setTitle(tr("Create a keypair..."));
    setSubTitle(tr("...for decrypting and signing messages"));
    auto *topLabel = new QLabel(tr("You should create a new keypair."
                                   "The pair consists of a public and a private key.<br>"
                                   "Other users can use the public key to encrypt messages for you "
                                   "and verify messages signed by you."
                                   "You can use the private key to decrypt and sign messages.<br>"
                                   "For more information have a look at the offline tutorial (which then is shown in the main window):"));
    topLabel->setWordWrap(true);
    auto *linkLabel = new QLabel("<a href=""docu_keygen.html#content"">" + tr("Offline tutorial") + "</a>");
    //linkLabel->setOpenExternalLinks(true);

    connect(linkLabel, SIGNAL(linkActivated(QString)), parentWidget()->parentWidget(), SLOT(openHelp(QString)));

    auto *createKeyButtonBox = new QWidget(this);
    auto *createKeyButtonBoxLayout = new QHBoxLayout(createKeyButtonBox);
    auto *createKeyButton = new QPushButton(tr("Create New Key"));
    createKeyButtonBoxLayout->addWidget(createKeyButton);
    createKeyButtonBoxLayout->addStretch(1);
    auto *layout = new QVBoxLayout();
    layout->addWidget(topLabel);
    layout->addWidget(linkLabel);
    layout->addWidget(createKeyButtonBox);
    connect(createKeyButton, SIGNAL(clicked()), this, SLOT(slotGenerateKeyDialog()));

    setLayout(layout);
}

int KeyGenPage::nextId() const {
    return Wizard::Page_Conclusion;
}

void KeyGenPage::slotGenerateKeyDialog() {
    auto *keyGenDialog = new KeyGenDialog(mCtx, this);
    keyGenDialog->exec();
    wizard()->next();
}

ConclusionPage::ConclusionPage(QWidget *parent)
        : QWizardPage(parent) {
    setTitle(tr("Ready."));
    setSubTitle(tr("Have fun with gpg4usb!"));

    auto *bottomLabel = new QLabel(tr("You are ready to use gpg4usb now.<br><br>"
                                      "The offline help will get you started with gpg4usb. "
                                      "It will open in the main window.<br>"));
    bottomLabel->setWordWrap(true);

    openHelpCheckBox = new QCheckBox(tr("Open offline help."));
    openHelpCheckBox->setChecked(Qt::Checked);

    dontShowWizardCheckBox = new QCheckBox(tr("Dont show the wizard again."));
    dontShowWizardCheckBox->setChecked(Qt::Checked);

    registerField("showWizard", dontShowWizardCheckBox);
    registerField("openHelp", openHelpCheckBox);

    auto *layout = new QVBoxLayout;
    layout->addWidget(bottomLabel);
    layout->addWidget(openHelpCheckBox);
    layout->addWidget(dontShowWizardCheckBox);
    setLayout(layout);
    setVisible(true);
}

int ConclusionPage::nextId() const {
    return -1;
}
