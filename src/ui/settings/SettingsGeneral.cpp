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

#include "SettingsGeneral.h"

#ifdef SERVER_SUPPORT
#include "server/ComUtils.h"
#endif

#ifdef MULTI_LANG_SUPPORT
#include "SettingsDialog.h"
#endif

#include "GlobalSettingStation.h"
#include "gpg/function/GpgKeyGetter.h"
#include "rapidjson/prettywriter.h"
#include "ui/widgets/KeyList.h"

namespace GpgFrontend::UI {

GeneralTab::GeneralTab(QWidget* parent) : QWidget(parent) {
#ifdef SERVER_SUPPORT
  /*****************************************
   * GpgFrontend Server
   *****************************************/
  auto* serverBox = new QGroupBox(_("GpgFrontend Server"));
  auto* serverBoxLayout = new QVBoxLayout();
  serverSelectBox = new QComboBox();
  serverBoxLayout->addWidget(serverSelectBox);
  serverBoxLayout->addWidget(new QLabel(
      _("Server that provides short key and key exchange services")));

  serverBox->setLayout(serverBoxLayout);
#endif

  /*****************************************
   * Save-Checked-Keys-Box
   *****************************************/
  auto* saveCheckedKeysBox = new QGroupBox(_("Save Checked Keys"));
  auto* saveCheckedKeysBoxLayout = new QHBoxLayout();
  saveCheckedKeysCheckBox = new QCheckBox(
      _("Save checked private keys on exit and restore them on next start."),
      this);
  saveCheckedKeysBoxLayout->addWidget(saveCheckedKeysCheckBox);
  saveCheckedKeysBox->setLayout(saveCheckedKeysBoxLayout);

  /*****************************************
   * Key-Impport-Confirmation Box
   *****************************************/
  auto* importConfirmationBox =
      new QGroupBox(_("Confirm drag'n'drop key import"));
  auto* importConfirmationBoxLayout = new QHBoxLayout();
  importConfirmationCheckBox = new QCheckBox(
      _("Import files dropped on the Key List without confirmation."), this);
  importConfirmationBoxLayout->addWidget(importConfirmationCheckBox);
  importConfirmationBox->setLayout(importConfirmationBoxLayout);

#ifdef MULTI_LANG_SUPPORT
  /*****************************************
   * Language Select Box
   *****************************************/
  auto* langBox = new QGroupBox(_("Language"));
  auto* langBoxLayout = new QVBoxLayout();
  langSelectBox = new QComboBox;
  lang = SettingsDialog::listLanguages();

  for (const auto& l : lang) {
    langSelectBox->addItem(l);
  }

  langBoxLayout->addWidget(langSelectBox);
  langBoxLayout->addWidget(new QLabel(
      "<b>" + QString(_("NOTE")) + _(": ") + "</b>" +
      _("GpgFrontend will restart automatically if you change the language!")));
  langBox->setLayout(langBoxLayout);
  connect(langSelectBox, SIGNAL(currentIndexChanged(int)), this,
          SLOT(slotLanguageChanged()));
#endif

#ifdef SERVER_SUPPORT
  /*****************************************
   * Own Key Select Box
   *****************************************/
  auto* ownKeyBox = new QGroupBox(_("Own key"));
  auto* ownKeyBoxLayout = new QVBoxLayout();
  auto* ownKeyServiceTokenLayout = new QHBoxLayout();
  ownKeySelectBox = new QComboBox;
  getServiceTokenButton = new QPushButton(_("Get Service Token"));
  serviceTokenLabel = new QLabel(_("No Service Token Found"));
  serviceTokenLabel->setAlignment(Qt::AlignCenter);

  ownKeyBox->setLayout(ownKeyBoxLayout);

  mKeyList = new KeyList();

  // Fill the keyid hashmap
  keyIds.insert({QString(), "<none>"});

  auto private_keys = mKeyList->getAllPrivateKeys();

  for (const auto& keyid : *private_keys) {
    auto key = GpgKeyGetter::GetInstance().GetKey(keyid);
    if (!key.good()) continue;
    keyIds.insert({key.id(), key.uids()->front().uid()});
  }
  for (const auto& k : keyIds) {
    ownKeySelectBox->addItem(QString::fromStdString(k.second));
    keyIdsList.push_back(k.first);
  }
  connect(ownKeySelectBox, SIGNAL(currentIndexChanged(int)), this,
          SLOT(slotOwnKeyIdChanged()));
  connect(getServiceTokenButton, SIGNAL(clicked(bool)), this,
          SLOT(slotGetServiceToken()));

  ownKeyBoxLayout->addWidget(new QLabel(
      _("Key pair for synchronization and identity authentication")));
  ownKeyBoxLayout->addWidget(ownKeySelectBox);
  ownKeyBoxLayout->addLayout(ownKeyServiceTokenLayout);
  ownKeyServiceTokenLayout->addWidget(getServiceTokenButton);
  ownKeyServiceTokenLayout->addWidget(serviceTokenLabel);
  ownKeyServiceTokenLayout->stretch(0);
#endif

  /*****************************************
   * Mainlayout
   *****************************************/
  auto* mainLayout = new QVBoxLayout;
#ifdef SERVER_SUPPORT
  mainLayout->addWidget(serverBox);
#endif
  mainLayout->addWidget(saveCheckedKeysBox);
  mainLayout->addWidget(importConfirmationBox);
#ifdef MULTI_LANG_SUPPORT
  mainLayout->addWidget(langBox);
#endif
#ifdef SERVER_SUPPORT
  mainLayout->addWidget(ownKeyBox);
#endif

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
  auto& settings = GlobalSettingStation::GetInstance().GetUISettings();
  try {
    bool save_key_checked = settings.lookup("general.save_key_checked");
    if (save_key_checked) saveCheckedKeysCheckBox->setCheckState(Qt::Checked);
  } catch (...) {
    LOG(ERROR) << _("Setting Operation Error") << _("save_key_checked");
  }

#ifdef SERVER_SUPPORT
  auto serverList =
      settings.value("general/gpgfrontendServerList").toStringList();
  if (serverList.empty()) {
    serverList.append("service.gpgfrontend.pub");
    serverList.append("localhost");
  }
  for (const auto& s : serverList) serverSelectBox->addItem(s);

  qDebug() << "Current Gpgfrontend Server"
           << settings.value("general/currentGpgfrontendServer").toString();
  serverSelectBox->setCurrentText(
      settings
          .value("general/currentGpgfrontendServer", "service.gpgfrontend.pub")
          .toString());

  connect(serverSelectBox,
          QOverload<const QString&>::of(&QComboBox::currentTextChanged), this,
          [&](const QString& current) -> void {
            settings.setValue("general/currentGpgfrontendServer", current);
          });
#endif

#ifdef MULTI_LANG_SUPPORT
  try {
    std::string lang_key = settings.lookup("general.lang");
    QString lang_value = lang.value(lang_key.c_str());
    LOG(INFO) << "lang settings current" << lang_value.toStdString();
    if (!lang.empty()) {
      langSelectBox->setCurrentIndex(langSelectBox->findText(lang_value));
    } else {
      langSelectBox->setCurrentIndex(0);
    }
  } catch (...) {
    LOG(ERROR) << _("Setting Operation Error") << _("lang");
  }
#endif

#ifdef SERVER_SUPPORT
  auto own_key_id = settings.value("general/ownKeyId").toString().toStdString();
  if (own_key_id.empty()) {
    ownKeySelectBox->setCurrentText("<none>");
  } else {
    const auto uid = keyIds.find(own_key_id)->second;
    ownKeySelectBox->setCurrentText(QString::fromStdString(uid));
  }

  serviceToken =
      settings.value("general/serviceToken").toString().toStdString();
  if (!serviceToken.empty()) {
    serviceTokenLabel->setText(QString::fromStdString(serviceToken));
  }
#endif

  try {
    bool confirm_import_keys = settings.lookup("general.confirm_import_keys");
    LOG(INFO) << "confirm_import_keys" << confirm_import_keys;
    if (confirm_import_keys)
      importConfirmationCheckBox->setCheckState(Qt::Checked);
  } catch (...) {
    LOG(ERROR) << _("Setting Operation Error") << _("confirm_import_keys");
  }
}

/***********************************
 * get the values of the buttons and
 * write them to settings-file
 *************************************/
void GeneralTab::applySettings() {
  auto& settings =
      GpgFrontend::UI::GlobalSettingStation::GetInstance().GetUISettings();

  if (!settings.exists("general") ||
      settings.lookup("general").getType() != libconfig::Setting::TypeGroup)
    settings.add("general", libconfig::Setting::TypeGroup);

  auto& general = settings["general"];

  if (!general.exists("save_key_checked"))
    general.add("save_key_checked", libconfig::Setting::TypeBoolean) =
        saveCheckedKeysCheckBox->isChecked();
  else {
    general["save_key_checked"] = saveCheckedKeysCheckBox->isChecked();
  }

#ifdef SERVER_SUPPORT
  qDebug() << "serverSelectBox currentText" << serverSelectBox->currentText();
  settings.setValue("general/currentGpgfrontendServer",
                    serverSelectBox->currentText());

  auto* serverList = new QStringList();
  for (int i = 0; i < serverSelectBox->count(); i++)
    serverList->append(serverSelectBox->itemText(i));
  settings.setValue("general/gpgfrontendServerList", *serverList);
  delete serverList;
#endif

#ifdef MULTI_LANG_SUPPORT
  if (!general.exists("lang"))
    general.add("lang", libconfig::Setting::TypeBoolean) =
        lang.key(langSelectBox->currentText()).toStdString();
  else {
    general["lang"] = lang.key(langSelectBox->currentText()).toStdString();
  }
#endif
  
#ifdef SERVER_SUPPORT
  settings.setValue(
      "general/ownKeyId",
      QString::fromStdString(keyIdsList[ownKeySelectBox->currentIndex()]));

  settings.setValue("general/serviceToken",
                    QString::fromStdString(serviceToken));
#endif

  if (!general.exists("confirm_import_keys"))
    general.add("confirm_import_keys", libconfig::Setting::TypeBoolean) =
        importConfirmationCheckBox->isChecked();
  else {
    general["confirm_import_keys"] = importConfirmationCheckBox->isChecked();
  }
}

#ifdef MULTI_LANG_SUPPORT
void GeneralTab::slotLanguageChanged() { emit signalRestartNeeded(true); }
#endif

#ifdef SERVER_SUPPORT
void GeneralTab::slotOwnKeyIdChanged() {
  // Set ownKeyId to currently selected
  this->serviceTokenLabel->setText(_("No Service Token Found"));
  serviceToken.clear();
}
#endif

#ifdef SERVER_SUPPORT
void GeneralTab::slotGetServiceToken() {
  auto utils = new ComUtils(this);

  QUrl reqUrl(utils->getUrl(ComUtils::GetServiceToken));
  QNetworkRequest request(reqUrl);
  request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

  const auto keyId = keyIdsList[ownKeySelectBox->currentIndex()];

  qDebug() << "KeyId" << keyIdsList[ownKeySelectBox->currentIndex()];

  if (keyId.isEmpty()) {
    QMessageBox::critical(
        this, _("Invalid Operation"),
        _("Own Key can not be None while getting service token."));
    return;
  }

  QStringList selectedKeyIds(keyIdsList[ownKeySelectBox->currentIndex()]);

  QByteArray keyDataBuf;
  mCtx->exportKeys(&selectedKeyIds, &keyDataBuf);

  GpgKey key = mCtx->getKeyRefById(keyId);

  if (!key.good) {
    QMessageBox::critical(this, _("Error"), _("Key Not Exists"));
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
  ver.SetString(version.toUtf8().constData(),
                qApp->applicationVersion().count());

  sha.SetString(shaStr.constData(), shaStr.count());
  signFpr.SetString(signFprStr.constData(), signFprStr.count());

  rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();

  doc.AddMember("publicKey", pubkey, allocator);
  doc.AddMember("sha", sha, allocator);
  doc.AddMember("signedFpr", signFpr, allocator);
  doc.AddMember("version", ver, allocator);

  rapidjson::StringBuffer sb;
  rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(sb);
  doc.Accept(writer);

  QByteArray postData(sb.GetString());

  QNetworkReply* reply = utils->getNetworkManager().post(request, postData);

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

    if (!utils->checkDataValueStr("serviceToken") ||
        !utils->checkDataValueStr("fpr")) {
      QMessageBox::critical(this, _("Error"),
                            _("The communication content with the server does "
                              "not meet the requirements"));
      return;
    }

    QString serviceTokenTemp = utils->getDataValueStr("serviceToken");
    QString fpr = utils->getDataValueStr("fpr");
    auto key = mCtx->getKeyRefByFpr(fpr);
    if (utils->checkServiceTokenFormat(serviceTokenTemp) && key.good) {
      serviceToken = serviceTokenTemp;
      qDebug() << "Get Service Token" << serviceToken;
      // Auto update settings
      settings.setValue("general/serviceToken", serviceToken);
      serviceTokenLabel->setText(serviceToken);
      QMessageBox::information(this, _("Notice"),
                               _("Succeed in getting service token"));
    } else {
      QMessageBox::critical(
          this, _("Error"),
          _("There is a problem with the communication with the server"));
    }
  }
}
#endif

}  // namespace GpgFrontend::UI
