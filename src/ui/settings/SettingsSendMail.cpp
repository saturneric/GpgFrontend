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

#include "SettingsSendMail.h"

#ifdef SMTP_SUPPORT
#include "smtp/SmtpMime"
#endif

#include "ui/settings/GlobalSettingStation.h"
#include "ui_SendMailSettings.h"

namespace GpgFrontend::UI {

SendMailTab::SendMailTab(QWidget* parent)
    : QWidget(parent), ui(std::make_shared<Ui_SendMailSettings>()) {
  ui->setupUi(this);

  connect(ui->enableCheckBox, &QCheckBox::stateChanged, this, [=](int state) {
    ui->smtpServerAddressEdit->setDisabled(state != Qt::Checked);
    ui->portSpin->setDisabled(state != Qt::Checked);
    ui->connextionSecurityComboBox->setDisabled(state != Qt::Checked);

    ui->identityCheckBox->setDisabled(state != Qt::Checked);
    ui->usernameEdit->setDisabled(state != Qt::Checked);
    ui->passwordEdit->setDisabled(state != Qt::Checked);

    ui->defaultSenderEmailEdit->setDisabled(state != Qt::Checked);
    ui->checkConnectionButton->setDisabled(state != Qt::Checked);
    ui->senTestMailButton->setDisabled(state != Qt::Checked);
  });

  connect(ui->checkConnectionButton, &QPushButton::clicked, this,
          &SendMailTab::slotCheckConnection);

  connect(ui->identityCheckBox, &QCheckBox::stateChanged, this, [=](int state) {
    ui->usernameEdit->setDisabled(state != Qt::Checked);
    ui->passwordEdit->setDisabled(state != Qt::Checked);
  });

  setSettings();
}

void SendMailTab::setSettings() {
  auto& settings = GlobalSettingStation::GetInstance().GetUISettings();

  ui->enableCheckBox->setCheckState(Qt::Unchecked);
  try {
    bool smtp_enable = settings.lookup("smtp.enable");
    if (smtp_enable) ui->enableCheckBox->setCheckState(Qt::Checked);
  } catch (...) {
    LOG(ERROR) << _("Setting Operation Error") << _("save_key_checked");
  }

  ui->identityCheckBox->setCheckState(Qt::Unchecked);
  try {
    bool identity_enable = settings.lookup("smtp.identity_enable");
    if (identity_enable) ui->identityCheckBox->setCheckState(Qt::Checked);
  } catch (...) {
    LOG(ERROR) << _("Setting Operation Error") << _("identity_enable");
  }

  try {
    std::string mail_address = settings.lookup("smtp.mail_address");
    ui->smtpServerAddressEdit->setText(mail_address.c_str());
  } catch (...) {
    LOG(ERROR) << _("Setting Operation Error") << _("mail_address");
  }

  try {
    std::string std_username = settings.lookup("smtp.username");
    ui->usernameEdit->setText(std_username.c_str());
  } catch (...) {
    LOG(ERROR) << _("Setting Operation Error") << _("username");
  }

  try {
    std::string std_password = settings.lookup("smtp.password");
    ui->passwordEdit->setText(std_password.c_str());
  } catch (...) {
    LOG(ERROR) << _("Setting Operation Error") << _("password");
  }

  try {
    int port = settings.lookup("smtp.port");
    ui->portSpin->setValue(port);
  } catch (...) {
    LOG(ERROR) << _("Setting Operation Error") << _("port");
  }

  ui->connextionSecurityComboBox->setCurrentText("None");
  try {
    std::string connection_type = settings.lookup("smtp.connection_type");
    ui->connextionSecurityComboBox->setCurrentText(connection_type.c_str());
  } catch (...) {
    LOG(ERROR) << _("Setting Operation Error") << _("connection_type");
  }

  try {
    std::string default_sender = settings.lookup("smtp.default_sender");
    ui->defaultSenderEmailEdit->setText(default_sender.c_str());
  } catch (...) {
    LOG(ERROR) << _("Setting Operation Error") << _("default_sender");
  }
}

void SendMailTab::applySettings() {
  auto& settings =
      GpgFrontend::UI::GlobalSettingStation::GetInstance().GetUISettings();

  if (!settings.exists("smtp") ||
      settings.lookup("smtp").getType() != libconfig::Setting::TypeGroup)
    settings.add("smtp", libconfig::Setting::TypeGroup);

  auto& smtp = settings["smtp"];

  if (!smtp.exists("mail_address"))
    smtp.add("mail_address", libconfig::Setting::TypeString) =
        ui->smtpServerAddressEdit->text().toStdString();
  else {
    smtp["mail_address"] = ui->smtpServerAddressEdit->text().toStdString();
  }

  if (!smtp.exists("username"))
    smtp.add("username", libconfig::Setting::TypeString) =
        ui->usernameEdit->text().toStdString();
  else {
    smtp["username"] = ui->usernameEdit->text().toStdString();
  }

  if (!smtp.exists("password"))
    smtp.add("password", libconfig::Setting::TypeString) =
        ui->passwordEdit->text().toStdString();
  else {
    smtp["password"] = ui->passwordEdit->text().toStdString();
  }

  if (!smtp.exists("port"))
    smtp.add("port", libconfig::Setting::TypeInt) = ui->portSpin->value();
  else {
    smtp["port"] = ui->portSpin->value();
  }

  if (!smtp.exists("connection_type"))
    smtp.add("connection_type", libconfig::Setting::TypeString) =
        ui->connextionSecurityComboBox->currentText().toStdString();
  else {
    smtp["connection_type"] =
        ui->connextionSecurityComboBox->currentText().toStdString();
  }

  if (!smtp.exists("default_sender"))
    smtp.add("default_sender", libconfig::Setting::TypeString) =
        ui->defaultSenderEmailEdit->text().toStdString();
  else {
    smtp["default_sender"] = ui->defaultSenderEmailEdit->text().toStdString();
  }

  if (!smtp.exists("identity_enable"))
    smtp.add("identity_enable", libconfig::Setting::TypeBoolean) =
        ui->identityCheckBox->isChecked();
  else {
    smtp["identity_enable"] = ui->identityCheckBox->isChecked();
  }

  if (!smtp.exists("enable"))
    smtp.add("enable", libconfig::Setting::TypeBoolean) =
        ui->enableCheckBox->isChecked();
  else {
    smtp["enable"] = ui->enableCheckBox->isChecked();
  }
}

#ifdef SMTP_SUPPORT
void SendMailTab::slotCheckConnection() {
  SmtpClient::ConnectionType connectionType;
  const auto selectedConnType = ui->connextionSecurityComboBox->currentText();
  if (selectedConnType == "SSL") {
    connectionType = SmtpClient::ConnectionType::SslConnection;
  } else if (selectedConnType == "TLS" || selectedConnType == "STARTTLS") {
    connectionType = SmtpClient::ConnectionType::TlsConnection;
  } else {
    connectionType = SmtpClient::ConnectionType::TcpConnection;
  }

  SmtpClient smtp(ui->smtpServerAddressEdit->text(), ui->portSpin->value(),
                  connectionType);

  if (ui->identityCheckBox->isChecked()) {
    smtp.setUser(ui->usernameEdit->text());
    smtp.setPassword(ui->passwordEdit->text());
  }

  bool if_success = true;

  if (!smtp.connectToHost()) {
    QMessageBox::critical(this, _("Fail"), _("Fail to Connect SMTP Server"));
    if_success = false;
  }
  if (if_success && !smtp.login()) {
    QMessageBox::critical(this, _("Fail"), _("Fail to Login"));
    if_success = false;
  }

  if (if_success)
    QMessageBox::information(this, _("Success"),
                             _("Succeed in connecting and login"));
}
#endif

void SendMailTab::slotCheckBoxSetEnableDisable(int state) {}

}  // namespace GpgFrontend::UI
