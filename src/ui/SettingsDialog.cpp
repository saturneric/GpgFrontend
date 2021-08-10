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
#include "smtp/SmtpMime"
#include "ui/WaitingDialog.h"

#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"

SettingsDialog::SettingsDialog(GpgME::GpgContext *ctx, QWidget *parent)
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
    mainLayout->addWidget(buttonBox);
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

GeneralTab::GeneralTab(GpgME::GpgContext *ctx, QWidget *parent)
        : QWidget(parent), appPath(qApp->applicationDirPath()),
          settings(RESOURCE_DIR(appPath) + "/conf/gpgfrontend.ini",
                   QSettings::IniFormat) {
    mCtx = ctx;

    /*****************************************
     * GpgFrontend Server
     *****************************************/
    auto *serverBox = new QGroupBox(tr("GpgFrontend Server"));
    auto *serverBoxLayout = new QVBoxLayout();
    serverSelectBox = new QComboBox();
    serverBoxLayout->addWidget(serverSelectBox);
    serverBoxLayout->addWidget(new QLabel(
            tr("Server that provides short key and key exchange services")));
    serverBox->setLayout(serverBoxLayout);

    /*****************************************
     * Save-Checked-Keys-Box
     *****************************************/
    auto *saveCheckedKeysBox = new QGroupBox(tr("Save Checked Keys"));
    auto *saveCheckedKeysBoxLayout = new QHBoxLayout();
    saveCheckedKeysCheckBox = new QCheckBox(
            tr("Save checked private keys on exit and restore them on next start."),
            this);
    saveCheckedKeysBoxLayout->addWidget(saveCheckedKeysCheckBox);
    saveCheckedKeysBox->setLayout(saveCheckedKeysBoxLayout);

    /*****************************************
     * Key-Impport-Confirmation Box
     *****************************************/
    auto *importConfirmationBox =
            new QGroupBox(tr("Confirm drag'n'drop key import"));
    auto *importConfirmationBoxLayout = new QHBoxLayout();
    importConfirmationCheckBox = new QCheckBox(
            tr("Import files dropped on the keylist without confirmation."), this);
    importConfirmationBoxLayout->addWidget(importConfirmationCheckBox);
    importConfirmationBox->setLayout(importConfirmationBoxLayout);

    /*****************************************
     * Language Select Box
     *****************************************/
    auto *langBox = new QGroupBox(tr("Language"));
    auto *langBoxLayout = new QVBoxLayout();
    langSelectBox = new QComboBox;
    lang = SettingsDialog::listLanguages();

    for (const auto &l: lang) { langSelectBox->addItem(l); }

    langBoxLayout->addWidget(langSelectBox);
    langBoxLayout->addWidget(
            new QLabel(tr("<b>NOTE: </b> GpgFrontend will restart automatically if "
                          "you change the language!")));
    langBox->setLayout(langBoxLayout);
    connect(langSelectBox, SIGNAL(currentIndexChanged(int)), this,
            SLOT(slotLanguageChanged()));

    /*****************************************
     * Own Key Select Box
     *****************************************/
    auto *ownKeyBox = new QGroupBox(tr("Own key"));
    auto *ownKeyBoxLayout = new QVBoxLayout();
    auto *ownKeyServiceTokenLayout = new QHBoxLayout();
    ownKeySelectBox = new QComboBox;
    getServiceTokenButton = new QPushButton(tr("Get Service Token"));
    serviceTokenLabel = new QLabel(tr("No Service Token Found"));

    ownKeyBox->setLayout(ownKeyBoxLayout);
    mKeyList = new KeyList(mCtx);

    // Fill the keyid hashmap
    keyIds.insert("", tr("<none>"));

    for (const auto &keyid : *mKeyList->getAllPrivateKeys()) {
        auto &key = mCtx->getKeyById(keyid);
        keyIds.insert(key.id, key.uids.first().uid);
    }
    for (const auto &k : keyIds.keys()) {
        ownKeySelectBox->addItem(keyIds.find(k).value());
        keyIdsList.append(k);
    }
    connect(ownKeySelectBox, SIGNAL(currentIndexChanged(int)), this,
            SLOT(slotOwnKeyIdChanged()));
    connect(getServiceTokenButton, SIGNAL(clicked(bool)), this,
            SLOT(slotGetServiceToken()));

    ownKeyBoxLayout->addWidget(new QLabel(
            tr("Key pair for synchronization and identity authentication")));
    ownKeyBoxLayout->addWidget(ownKeySelectBox);
    ownKeyBoxLayout->addLayout(ownKeyServiceTokenLayout);
    ownKeyServiceTokenLayout->addWidget(getServiceTokenButton);
    ownKeyServiceTokenLayout->addWidget(serviceTokenLabel);
    ownKeyServiceTokenLayout->stretch(0);

    /*****************************************
     * Mainlayout
     *****************************************/
    auto *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(serverBox);
    mainLayout->addWidget(saveCheckedKeysBox);
    mainLayout->addWidget(importConfirmationBox);
    mainLayout->addWidget(langBox);
    mainLayout->addWidget(ownKeyBox);

    setSettings();
    mainLayout->addStretch(1);
    setLayout(mainLayout);
}

/**********************************
 * Read the settings from config
 * and set the buttons and checkboxes
 * appropriately
 **********************************/
void GeneralTab::setSettings() {
    // Keysaving
    if (settings.value("keys/saveKeyChecked").toBool()) {
        saveCheckedKeysCheckBox->setCheckState(Qt::Checked);
    }

    auto serverList = settings.value("general/gpgfrontendServerList").toStringList();
    if(serverList.empty()) {
        serverList.append("service.gpgfrontend.pub");
    }
    for(const auto &s : serverList)
        serverSelectBox->addItem(s);
    serverSelectBox->setCurrentText(settings.value("general/currentGpgfrontendServer",
                                                   "service.gpgfrontend.pub").toString());

    // Language setting
    QString langKey = settings.value("int/lang").toString();
    QString langValue = lang.value(langKey);
    if (langKey != "") {
        langSelectBox->setCurrentIndex(langSelectBox->findText(langValue));
    }

    QString own_key_id = settings.value("general/ownKeyId").toString();
    qDebug() << "OwnKeyId" << own_key_id;
    if (own_key_id.isEmpty()) {
        ownKeySelectBox->setCurrentText("<none>");
    } else {
        const auto text = keyIds.find(own_key_id).value();
        qDebug() << "OwnKey" << own_key_id << text;
        ownKeySelectBox->setCurrentText(text);
    }

    serviceToken = settings.value("general/serviceToken").toString();
    qDebug() << "Load Service Token" << serviceToken;
    if(!serviceToken.isEmpty()) {
        serviceTokenLabel->setText(serviceToken);
    }

    // Get own key information from keydb/gpg.conf (if contained)
    if (settings.value("general/confirmImportKeys", Qt::Checked).toBool()) {
        importConfirmationCheckBox->setCheckState(Qt::Checked);
    }
}

/***********************************
 * get the values of the buttons and
 * write them to settings-file
 *************************************/
void GeneralTab::applySettings() {
    settings.setValue("keys/saveKeyChecked",
                      saveCheckedKeysCheckBox->isChecked());

    settings.setValue("general/currentGpgfrontendServer",
                      serverSelectBox->currentText());

    auto *serverList = new QStringList();
    for(int i = 0; i < serverSelectBox->count(); i++)
        serverList->append(serverSelectBox->itemText(i));
    settings.setValue("general/gpgfrontendServerList",
                      *serverList);
    delete serverList;

    settings.setValue("int/lang", lang.key(langSelectBox->currentText()));

    settings.setValue("general/ownKeyId",
                      keyIdsList[ownKeySelectBox->currentIndex()]);

    settings.setValue("general/serviceToken",
                     serviceToken);

    settings.setValue("general/confirmImportKeys",
                      importConfirmationCheckBox->isChecked());
}

void GeneralTab::slotLanguageChanged() { emit signalRestartNeeded(true); }

void GeneralTab::slotOwnKeyIdChanged() {
    // Set ownKeyId to currently selected
    this->serviceTokenLabel->setText(tr("No Service Token Found"));
    serviceToken.clear();
}

void GeneralTab::slotGetServiceToken() {
    QUrl reqUrl("http://127.0.0.1:9048/user");
    QNetworkRequest request(reqUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    // Building Post Data
    QByteArray keyDataBuf;

    const auto keyId = keyIdsList[ownKeySelectBox->currentIndex()];

    qDebug() << "KeyId" << keyIdsList[ownKeySelectBox->currentIndex()];

    if(keyId.isEmpty()) {
        QMessageBox::critical(this, tr("Invalid Operation"), tr("Own Key can not be None while getting service token."));
        return;
    }

    QStringList selectedKeyIds(keyIdsList[ownKeySelectBox->currentIndex()]);
    mCtx->exportKeys(&selectedKeyIds, &keyDataBuf);

    qDebug() << "keyDataBuf" << keyDataBuf;

    rapidjson::Value p, v;

    rapidjson::Document doc;
    doc.SetObject();

    p.SetString(keyDataBuf.constData(), keyDataBuf.count());

    auto version = qApp->applicationVersion();
    v.SetString(version.toUtf8().constData(), qApp->applicationVersion().count());

    doc.AddMember("publicKey", p, doc.GetAllocator());
    doc.AddMember("version", v, doc.GetAllocator());

    rapidjson::StringBuffer sb;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(sb);
    doc.Accept(writer);

    QByteArray postData(sb.GetString());

    QNetworkReply *reply = manager.post(request, postData);

    auto dialog = new WaitingDialog("Getting Token From Server", this);
    dialog->show();

    while (reply->isRunning()) {
        QApplication::processEvents();
    }

    dialog->close();

    if(reply->error() == QNetworkReply::NoError) {
        rapidjson::Document docReply;
        docReply.Parse(reply->readAll().constData());
        QString serviceTokenTemp = docReply["serviceToken"].GetString();
        if(checkUUIDFormat(serviceTokenTemp)) {
            serviceToken = serviceTokenTemp;
            qDebug() << "Get Service Token" << serviceToken;
            serviceTokenLabel->setText(serviceToken);
        } else {
            QMessageBox::critical(this, tr("Error"), tr("Invalid Service Token Format"));
        }
    } else {
        QMessageBox::critical(this, tr("Error"), reply->errorString());
    }

}

bool GeneralTab::checkUUIDFormat(const QString& uuid) {
    return re_uuid.match(uuid).hasMatch();
}

SendMailTab::SendMailTab(QWidget *parent)
        : QWidget(parent), appPath(qApp->applicationDirPath()),
          settings(RESOURCE_DIR(appPath) + "/conf/gpgfrontend.ini",
                   QSettings::IniFormat) {

    enableCheckBox = new QCheckBox(tr("Enable"));
    enableCheckBox->setTristate(false);

    smtpAddress = new QLineEdit();
    username = new QLineEdit();
    password = new QLineEdit();
    password->setEchoMode(QLineEdit::Password);

    portSpin = new QSpinBox();
    portSpin->setMinimum(1);
    portSpin->setMaximum(65535);
    connectionTypeComboBox = new QComboBox();
    connectionTypeComboBox->addItem("None");
    connectionTypeComboBox->addItem("SSL");
    connectionTypeComboBox->addItem("TLS");
    connectionTypeComboBox->addItem("STARTTLS");

    defaultSender = new QLineEdit();;
    checkConnectionButton = new QPushButton(tr("Check Connection"));

    auto layout = new QGridLayout();
    layout->addWidget(enableCheckBox, 0, 0);
    layout->addWidget(new QLabel(tr("SMTP Address")), 1, 0);
    layout->addWidget(smtpAddress, 1, 1, 1, 4);
    layout->addWidget(new QLabel(tr("Username")), 2, 0);
    layout->addWidget(username, 2, 1, 1, 4);
    layout->addWidget(new QLabel(tr("Password")), 3, 0);
    layout->addWidget(password, 3, 1, 1, 4);
    layout->addWidget(new QLabel(tr("Port")), 4, 0);
    layout->addWidget(portSpin, 4, 1, 1, 1);
    layout->addWidget(new QLabel(tr("Connection Security")), 5, 0);
    layout->addWidget(connectionTypeComboBox, 5, 1, 1, 1);

    layout->addWidget(new QLabel(tr("Default Sender")), 6, 0);
    layout->addWidget(defaultSender, 6, 1, 1, 4);
    layout->addWidget(checkConnectionButton, 7, 0);

    connect(enableCheckBox, SIGNAL(stateChanged(int)), this, SLOT(slotCheckBoxSetEnableDisable(int)));
    connect(checkConnectionButton, SIGNAL(clicked(bool)), this, SLOT(slotCheckConnection()));


    this->setLayout(layout);
    setSettings();
}

/**********************************
 * Read the settings from config
 * and set the buttons and checkboxes
 * appropriately
 **********************************/
void SendMailTab::setSettings() {

    if (settings.value("sendMail/enable", false).toBool())
        enableCheckBox->setCheckState(Qt::Checked);
    else {
        enableCheckBox->setCheckState(Qt::Unchecked);
        smtpAddress->setDisabled(true);
        username->setDisabled(true);
        password->setDisabled(true);
        portSpin->setDisabled(true);
        connectionTypeComboBox->setDisabled(true);
        defaultSender->setDisabled(true);
        checkConnectionButton->setDisabled(true);
    }

    smtpAddress->setText(settings.value("sendMail/smtpAddress", QString()).toString());
    username->setText(settings.value("sendMail/username", QString()).toString());
    password->setText(settings.value("sendMail/password", QString()).toString());
    portSpin->setValue(settings.value("sendMail/port", 25).toInt());
    connectionTypeComboBox->setCurrentText(settings.value("sendMail/connectionType", "None").toString());
    defaultSender->setText(settings.value("sendMail/defaultSender", QString()).toString());

}

/***********************************
 * get the values of the buttons and
 * write them to settings-file
 *************************************/
void SendMailTab::applySettings() {

    settings.setValue("sendMail/smtpAddress", smtpAddress->text());
    settings.setValue("sendMail/username", username->text());
    settings.setValue("sendMail/password", password->text());
    settings.setValue("sendMail/port", portSpin->value());
    settings.setValue("sendMail/connectionType", connectionTypeComboBox->currentText());
    settings.setValue("sendMail/defaultSender", defaultSender->text());

    settings.setValue("sendMail/enable", enableCheckBox->isChecked());
}

void SendMailTab::slotCheckConnection() {

    SmtpClient::ConnectionType connectionType;
    const auto selectedConnType = connectionTypeComboBox->currentText();
    if (selectedConnType == "SSL") {
        connectionType = SmtpClient::ConnectionType::SslConnection;
    } else if (selectedConnType == "TLS" || selectedConnType == "STARTTLS") {
        connectionType = SmtpClient::ConnectionType::TlsConnection;
    } else {
        connectionType = SmtpClient::ConnectionType::TcpConnection;
    }

    SmtpClient smtp(smtpAddress->text(), portSpin->value(), connectionType);

    smtp.setUser(username->text());
    smtp.setPassword(password->text());

    bool if_success = true;

    if (!smtp.connectToHost()) {
        QMessageBox::critical(this, tr("Fail"), tr("Fail to Connect SMTP Server"));
        if_success = false;
    }
    if (if_success && !smtp.login()) {
        QMessageBox::critical(this, tr("Fail"), tr("Fail to Login"));
        if_success = false;
    }

    if (if_success)
        QMessageBox::information(this, tr("Success"), tr("Succeed in connecting and login"));

}

void SendMailTab::slotCheckBoxSetEnableDisable(int state) {
    if (state == Qt::Checked) {
        smtpAddress->setEnabled(true);
        username->setEnabled(true);
        password->setEnabled(true);
        portSpin->setEnabled(true);
        connectionTypeComboBox->setEnabled(true);
        defaultSender->setEnabled(true);
        checkConnectionButton->setEnabled(true);
    } else {
        smtpAddress->setDisabled(true);
        username->setDisabled(true);
        password->setDisabled(true);
        portSpin->setDisabled(true);
        connectionTypeComboBox->setDisabled(true);
        defaultSender->setDisabled(true);
        checkConnectionButton->setDisabled(true);
    }
}

AppearanceTab::AppearanceTab(QWidget *parent)
        : QWidget(parent), appPath(qApp->applicationDirPath()),
          settings(RESOURCE_DIR(appPath) + "/conf/gpgfrontend.ini",
                   QSettings::IniFormat) {
    /*****************************************
     * Icon-Size-Box
     *****************************************/
    auto *iconSizeBox = new QGroupBox(tr("Iconsize"));
    iconSizeGroup = new QButtonGroup();
    iconSizeSmall = new QRadioButton(tr("small"));
    iconSizeMedium = new QRadioButton(tr("medium"));
    iconSizeLarge = new QRadioButton(tr("large"));

    iconSizeGroup->addButton(iconSizeSmall, 1);
    iconSizeGroup->addButton(iconSizeMedium, 2);
    iconSizeGroup->addButton(iconSizeLarge, 3);

    auto *iconSizeBoxLayout = new QHBoxLayout();
    iconSizeBoxLayout->addWidget(iconSizeSmall);
    iconSizeBoxLayout->addWidget(iconSizeMedium);
    iconSizeBoxLayout->addWidget(iconSizeLarge);

    iconSizeBox->setLayout(iconSizeBoxLayout);

    /*****************************************
     * Icon-Style-Box
     *****************************************/
    auto *iconStyleBox = new QGroupBox(tr("Iconstyle"));
    iconStyleGroup = new QButtonGroup();
    iconTextButton = new QRadioButton(tr("just text"));
    iconIconsButton = new QRadioButton(tr("just icons"));
    iconAllButton = new QRadioButton(tr("text and icons"));

    iconStyleGroup->addButton(iconTextButton, 1);
    iconStyleGroup->addButton(iconIconsButton, 2);
    iconStyleGroup->addButton(iconAllButton, 3);

    auto *iconStyleBoxLayout = new QHBoxLayout();
    iconStyleBoxLayout->addWidget(iconTextButton);
    iconStyleBoxLayout->addWidget(iconIconsButton);
    iconStyleBoxLayout->addWidget(iconAllButton);

    iconStyleBox->setLayout(iconStyleBoxLayout);

    /*****************************************
     * Window-Size-Box
     *****************************************/
    auto *windowSizeBox = new QGroupBox(tr("Windowstate"));
    auto *windowSizeBoxLayout = new QHBoxLayout();
    windowSizeCheckBox =
            new QCheckBox(tr("Save window size and position on exit."), this);
    windowSizeBoxLayout->addWidget(windowSizeCheckBox);
    windowSizeBox->setLayout(windowSizeBoxLayout);

    /*****************************************
     * Info-Board-Font-Size-Box
     *****************************************/

    auto *infoBoardBox = new QGroupBox(tr("Information Board"));
    auto *infoBoardLayout = new QHBoxLayout();
    infoBoardFontSizeSpin = new QSpinBox();
    infoBoardFontSizeSpin->setRange(9, 18);
    infoBoardFontSizeSpin->setValue(10);
    infoBoardFontSizeSpin->setSingleStep(1);
    infoBoardLayout->addWidget(new QLabel(tr(" Front Size")));
    infoBoardLayout->addWidget(infoBoardFontSizeSpin);
    infoBoardBox->setLayout(infoBoardLayout);

    auto *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(iconSizeBox);
    mainLayout->addWidget(iconStyleBox);
    mainLayout->addWidget(windowSizeBox);
    mainLayout->addWidget(infoBoardBox);
    mainLayout->addStretch(1);
    setSettings();
    setLayout(mainLayout);
}

/**********************************
 * Read the settings from config
 * and set the buttons and checkboxes
 * appropriately
 **********************************/
void AppearanceTab::setSettings() {

    // Iconsize
    QSize iconSize = settings.value("toolbar/iconsize", QSize(24, 24)).toSize();
    switch (iconSize.height()) {
        case 12:
            iconSizeSmall->setChecked(true);
            break;
        case 24:
            iconSizeMedium->setChecked(true);
            break;
        case 32:
            iconSizeLarge->setChecked(true);
            break;
    }
    // Iconstyle
    Qt::ToolButtonStyle iconStyle = static_cast<Qt::ToolButtonStyle>(
            settings.value("toolbar/iconstyle", Qt::ToolButtonTextUnderIcon)
                    .toUInt());
    switch (iconStyle) {
        case Qt::ToolButtonTextOnly:
            iconTextButton->setChecked(true);
            break;
        case Qt::ToolButtonIconOnly:
            iconIconsButton->setChecked(true);
            break;
        case Qt::ToolButtonTextUnderIcon:
            iconAllButton->setChecked(true);
            break;
        default:
            break;
    }

    // Window Save and Position
    if (settings.value("window/windowSave").toBool())
        windowSizeCheckBox->setCheckState(Qt::Checked);

    // infoBoardFontSize
    auto infoBoardFontSize = settings.value("informationBoard/fontSize", 10).toInt();
    if (infoBoardFontSize < 9 || infoBoardFontSize > 18)
        infoBoardFontSize = 10;
    infoBoardFontSizeSpin->setValue(infoBoardFontSize);
}

/***********************************
 * get the values of the buttons and
 * write them to settings-file
 *************************************/
void AppearanceTab::applySettings() {
    switch (iconSizeGroup->checkedId()) {
        case 1:
            settings.setValue("toolbar/iconsize", QSize(12, 12));
            break;
        case 2:
            settings.setValue("toolbar/iconsize", QSize(24, 24));
            break;
        case 3:
            settings.setValue("toolbar/iconsize", QSize(32, 32));
            break;
    }

    switch (iconStyleGroup->checkedId()) {
        case 1:
            settings.setValue("toolbar/iconstyle", Qt::ToolButtonTextOnly);
            break;
        case 2:
            settings.setValue("toolbar/iconstyle", Qt::ToolButtonIconOnly);
            break;
        case 3:
            settings.setValue("toolbar/iconstyle", Qt::ToolButtonTextUnderIcon);
            break;
    }

    settings.setValue("window/windowSave", windowSizeCheckBox->isChecked());

    settings.setValue("informationBoard/fontSize", infoBoardFontSizeSpin->value());
}

KeyserverTab::KeyserverTab(QWidget *parent)
        : QWidget(parent), appPath(qApp->applicationDirPath()),
          settings(RESOURCE_DIR(appPath) + "/conf/gpgfrontend.ini",
                   QSettings::IniFormat) {

    auto keyServerList = settings.value("keyserver/keyServerList").toStringList();

    auto *mainLayout = new QVBoxLayout(this);

    auto *label = new QLabel(tr("Default Key Server for import:"));
    comboBox = new QComboBox;
    comboBox->setEditable(false);
    comboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    for (const auto &keyServer : keyServerList) {
        comboBox->addItem(keyServer);
        qDebug() << "KeyserverTab Get ListItemText" << keyServer;
    }

    comboBox->setCurrentText(
            settings.value("keyserver/defaultKeyServer").toString());

    auto *addKeyServerBox = new QWidget(this);
    auto *addKeyServerLayout = new QHBoxLayout(addKeyServerBox);
    auto *http = new QLabel("URL: ");
    newKeyServerEdit = new QLineEdit(this);
    auto *newKeyServerButton = new QPushButton(tr("Add"), this);
    connect(newKeyServerButton, SIGNAL(clicked()), this, SLOT(addKeyServer()));
    addKeyServerLayout->addWidget(http);
    addKeyServerLayout->addWidget(newKeyServerEdit);
    addKeyServerLayout->addWidget(newKeyServerButton);

    mainLayout->addWidget(label);
    mainLayout->addWidget(comboBox);
    mainLayout->addWidget(addKeyServerBox);
    mainLayout->addStretch(1);

    // Read keylist from ini-file and fill it into combobox
    setSettings();
}

/**********************************
 * Read the settings from config
 * and set the buttons and checkboxes
 * appropriately
 **********************************/
void KeyserverTab::setSettings() {
    auto *keyServerList = new QStringList();
    for (int i = 0; i < comboBox->count(); i++) {
        keyServerList->append(comboBox->itemText(i));
        qDebug() << "KeyserverTab ListItemText" << comboBox->itemText(i);
    }
    settings.setValue("keyserver/keyServerList", *keyServerList);
    delete keyServerList;
    settings.setValue("keyserver/defaultKeyServer", comboBox->currentText());
}

void KeyserverTab::addKeyServer() {
    if (newKeyServerEdit->text().startsWith("http://") ||
        newKeyServerEdit->text().startsWith("https://")) {
        comboBox->addItem(newKeyServerEdit->text());
    } else {
        comboBox->addItem("http://" + newKeyServerEdit->text());
    }
    comboBox->setCurrentIndex(comboBox->count() - 1);
}

/***********************************
 * get the values of the buttons and
 * write them to settings-file
 *************************************/
void KeyserverTab::applySettings() {
    settings.setValue("keyserver/defaultKeyServer", comboBox->currentText());
}

AdvancedTab::AdvancedTab(QWidget *parent)
        : QWidget(parent), appPath(qApp->applicationDirPath()),
          settings(RESOURCE_DIR(appPath) + "/conf/gpgfrontend.ini",
                   QSettings::IniFormat) {
    /*****************************************
     * Steganography Box
     *****************************************/
    auto *steganoBox = new QGroupBox(tr("Show Steganography Options [Advanced]"));
    auto *steganoBoxLayout = new QHBoxLayout();
    steganoCheckBox = new QCheckBox(tr("Show Steganographic Options."), this);
    steganoBoxLayout->addWidget(steganoCheckBox);
    steganoBox->setLayout(steganoBoxLayout);

    auto *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(steganoBox);
    setSettings();
    mainLayout->addStretch(1);
    setLayout(mainLayout);
}

void AdvancedTab::setSettings() {
    if (settings.value("advanced/steganography").toBool()) {
        steganoCheckBox->setCheckState(Qt::Checked);
    }
}

void AdvancedTab::applySettings() {
    settings.setValue("advanced/steganography", steganoCheckBox->isChecked());
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
