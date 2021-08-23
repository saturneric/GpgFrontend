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
#include "server/ComUtils.h"

#include "rapidjson/prettywriter.h"

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
    serviceTokenLabel->setAlignment(Qt::AlignCenter);

    ownKeyBox->setLayout(ownKeyBoxLayout);
    mKeyList = new KeyList(mCtx);

    // Fill the keyid hashmap
    keyIds.insert("", tr("<none>"));

    for (const auto &keyid : *mKeyList->getAllPrivateKeys()) {
        auto key = mCtx->getKeyById(keyid);
        if (!key.good) continue;
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
    if (serverList.empty()) {
        serverList.append("service.gpgfrontend.pub");
        serverList.append("localhost");
    }
    for (const auto &s : serverList)
        serverSelectBox->addItem(s);

    qDebug() << "Current Gpgfrontend Server" << settings.value("general/currentGpgfrontendServer").toString();
    serverSelectBox->setCurrentText(settings.value("general/currentGpgfrontendServer",
                                                   "service.gpgfrontend.pub").toString());

    connect(serverSelectBox, QOverload<const QString &>::of(&QComboBox::currentTextChanged),
            this, [&](const QString &current) -> void {
        settings.setValue("general/currentGpgfrontendServer", current);
    });

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
    if (!serviceToken.isEmpty()) {
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

    qDebug() << "serverSelectBox currentText" << serverSelectBox->currentText();
    settings.setValue("general/currentGpgfrontendServer",
                      serverSelectBox->currentText());

    auto *serverList = new QStringList();
    for (int i = 0; i < serverSelectBox->count(); i++)
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

    auto utils = new ComUtils(this);

    QUrl reqUrl(utils->getUrl(ComUtils::GetServiceToken));
    QNetworkRequest request(reqUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");


    const auto keyId = keyIdsList[ownKeySelectBox->currentIndex()];

    qDebug() << "KeyId" << keyIdsList[ownKeySelectBox->currentIndex()];

    if (keyId.isEmpty()) {
        QMessageBox::critical(this, tr("Invalid Operation"),
                              tr("Own Key can not be None while getting service token."));
        return;
    }

    QStringList selectedKeyIds(keyIdsList[ownKeySelectBox->currentIndex()]);

    QByteArray keyDataBuf;
    mCtx->exportKeys(&selectedKeyIds, &keyDataBuf);

    GpgKey key = mCtx->getKeyById(keyId);

    if (!key.good) {
        QMessageBox::critical(this, tr("Error"),
                              tr("Key Not Exists"));
        return;
    }

    qDebug() << "keyDataBuf" << keyDataBuf;

    /**
     * {
     *      "publicKey" : ...
     *      "sha": ...
     *      "signedFpr": ...
     *      "version": ...
     * }
     */

    QCryptographicHash shaGen(QCryptographicHash::Sha256);
    shaGen.addData(keyDataBuf);

    auto shaStr = shaGen.result().toHex();

    auto signFprStr = ComUtils::getSignStringBase64(mCtx, key.fpr, key);

    rapidjson::Value pubkey, ver, sha, signFpr;

    rapidjson::Document doc;
    doc.SetObject();

    pubkey.SetString(keyDataBuf.constData(), keyDataBuf.count());

    auto version = qApp->applicationVersion();
    ver.SetString(version.toUtf8().constData(), qApp->applicationVersion().count());

    sha.SetString(shaStr.constData(), shaStr.count());
    signFpr.SetString(signFprStr.constData(), signFprStr.count());

    rapidjson::Document::AllocatorType &allocator = doc.GetAllocator();

    doc.AddMember("publicKey", pubkey, allocator);
    doc.AddMember("sha", sha, allocator);
    doc.AddMember("signedFpr", signFpr, allocator);
    doc.AddMember("version", ver, allocator);

    rapidjson::StringBuffer sb;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(sb);
    doc.Accept(writer);

    QByteArray postData(sb.GetString());

    QNetworkReply *reply = utils->getNetworkManager().post(request, postData);

    // Show Waiting Dailog
    auto dialog = new WaitingDialog("Getting Token From Server", this);
    dialog->show();

    while (reply->isRunning()) {
        QApplication::processEvents();
    }

    dialog->close();

    if (utils->checkServerReply(reply->readAll().constData())) {

        /**
         * {
         *      "serviceToken" : ...
         *      "fpr": ...
         * }
         */

        if (!utils->checkDataValueStr("serviceToken") || !utils->checkDataValueStr("fpr")) {
            QMessageBox::critical(this, tr("Error"),
                                  tr("The communication content with the server does not meet the requirements"));
            return;
        }

        QString serviceTokenTemp = utils->getDataValueStr("serviceToken");
        QString fpr = utils->getDataValueStr("fpr");
        auto key = mCtx->getKeyByFpr(fpr);
        if (utils->checkServiceTokenFormat(serviceTokenTemp) && key.good) {
            serviceToken = serviceTokenTemp;
            qDebug() << "Get Service Token" << serviceToken;
            // Auto update settings
            settings.setValue("general/serviceToken", serviceToken);
            serviceTokenLabel->setText(serviceToken);
            QMessageBox::information(this, tr("Notice"),
                                     tr("Succeed in getting service token"));
        } else {
            QMessageBox::critical(this, tr("Error"), tr("There is a problem with the communication with the server"));
        }
    }

}
