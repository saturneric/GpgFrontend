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
#include "ui/thread/SMTPSendMailThread.h"
#include "ui/thread/SMTPTestThread.h"
#include "ui_SendMailSettings.h"

namespace GpgFrontend::UI {

SendMailTab::SendMailTab(QWidget* parent)
    : QWidget(parent), ui(std::make_shared<Ui_SendMailSettings>()) {
  ui->setupUi(this);

  connect(ui->enableCheckBox, &QCheckBox::stateChanged, this,
          [=](int state) { switch_ui_enabled(state == Qt::Checked); });

#ifdef SMTP_SUPPORT
  connect(ui->checkConnectionButton, &QPushButton::clicked, this,
          &SendMailTab::slotCheckConnection);
  connect(ui->senTestMailButton, &QPushButton::clicked, this,
          &SendMailTab::slotSendTestMail);
#endif

  connect(ui->identityCheckBox, &QCheckBox::stateChanged, this,
          [=](int state) { switch_ui_identity_enabled(state == Qt::Checked); });

  connect(ui->connextionSecurityComboBox, &QComboBox::currentTextChanged, this,
          [=](const QString& current_text) {
            if (current_text == "SSL") {
              connection_type_ = SmtpClient::ConnectionType::SslConnection;
            } else if (current_text == "TLS" || current_text == "STARTTLS") {
              connection_type_ = SmtpClient::ConnectionType::TlsConnection;
            } else {
              connection_type_ = SmtpClient::ConnectionType::TcpConnection;
            }
          });

  ui->generalGroupBox->setTitle(_("General"));
  ui->identityGroupBox->setTitle(_("Identity Information"));
  ui->preferenceGroupBox->setTitle(_("Preference"));
  ui->operationsGroupBox->setTitle(_("Operations"));

  ui->enableCheckBox->setText(_("Enable Send Mail Ability"));
  ui->identityCheckBox->setText(_("Need Auth"));

  ui->smtpServerAddressLabel->setText(_("SMTP Server Address"));
  ui->smtpServerPortLabel->setText(_("SMTP Server Port"));
  ui->connectionSecurityLabel->setText(_("SMTP Connection Security"));
  ui->usernameLabel->setText(_("Username"));
  ui->passwordLabel->setText(_("Password"));

  ui->senderLabel->setText(_("Default Sender Email"));
  ui->checkConnectionButton->setText(_("Check Connection"));
  ui->senTestMailButton->setText(_("Send Test Email"));
  ui->gpgkeyIdLabel->setText(_("Default Sender GPG Key ID"));

  ui->tipsLabel->setText(
      _("Tips: It is recommended that you build your own mail server or use "
        "a trusted mail server. If you don't know the detailed configuration "
        "information, you can get it from the mail service provider."));

  ui->senTestMailButton->setDisabled(true);

  auto* email_validator =
      new QRegularExpressionValidator(re_email, ui->defaultSenderEmailEdit);
  ui->defaultSenderEmailEdit->setValidator(email_validator);

  setSettings();
}

void SendMailTab::setSettings() {
  auto json_optional =
      GlobalSettingStation::GetInstance().GetDataObject("smtp_passport");

  if (!json_optional.has_value()) {
    LOG(WARNING) << "no smtp passport found";
    return;
  }

  auto& smtp_passport_json = json_optional.value();

  try {
    std::string smtp_address = smtp_passport_json["smtp_address"];
    ui->smtpServerAddressEdit->setText(smtp_address.c_str());
  } catch (...) {
    LOG(ERROR) << _("Setting Operation Error") << _("smtp_address");
  }

  try {
    std::string std_username = smtp_passport_json["username"];
    ui->usernameEdit->setText(std_username.c_str());
  } catch (...) {
    LOG(ERROR) << _("Setting Operation Error") << _("username");
  }

  try {
    std::string std_password = smtp_passport_json["password"];
    ui->passwordEdit->setText(std_password.c_str());
  } catch (...) {
    LOG(ERROR) << _("Setting Operation Error") << _("password");
  }

  try {
    int port = smtp_passport_json["port"];
    ui->portSpin->setValue(port);
  } catch (...) {
    LOG(ERROR) << _("Setting Operation Error") << _("port");
  }

  ui->connextionSecurityComboBox->setCurrentText("None");
  try {
    std::string connection_type = smtp_passport_json["connection_type"];
    ui->connextionSecurityComboBox->setCurrentText(connection_type.c_str());
  } catch (...) {
    LOG(ERROR) << _("Setting Operation Error") << _("connection_type");
  }

  try {
    std::string default_sender = smtp_passport_json["default_sender"];
    ui->defaultSenderEmailEdit->setText(default_sender.c_str());
  } catch (...) {
    LOG(ERROR) << _("Setting Operation Error") << _("default_sender");
  }

  try {
    std::string default_sender_gpg_key_id =
        smtp_passport_json["default_sender_gpg_key_id"];
    ui->gpgKeyIDEdit->setText(default_sender_gpg_key_id.c_str());
  } catch (...) {
    LOG(ERROR) << _("Setting Operation Error")
               << _("default_sender_gpg_key_id");
  }

  ui->identityCheckBox->setCheckState(Qt::Unchecked);
  try {
    bool identity_enable = smtp_passport_json["identity_enable"];
    if (identity_enable)
      ui->identityCheckBox->setCheckState(Qt::Checked);
    else
      ui->identityCheckBox->setCheckState(Qt::Unchecked);
  } catch (...) {
    LOG(ERROR) << _("Setting Operation Error") << _("identity_enable");
  }

  {
    auto state = ui->identityCheckBox->checkState();
    switch_ui_identity_enabled(state == Qt::Checked);
  }

  ui->enableCheckBox->setCheckState(Qt::Unchecked);
  try {
    bool smtp_enable = smtp_passport_json["enable"];
    if (smtp_enable)
      ui->enableCheckBox->setCheckState(Qt::Checked);
    else
      ui->enableCheckBox->setCheckState(Qt::Unchecked);
  } catch (...) {
    LOG(ERROR) << _("Setting Operation Error") << _("enable");
  }

  {
    auto state = ui->enableCheckBox->checkState();
    switch_ui_enabled(state == Qt::Checked);
  }
}

void SendMailTab::applySettings() {
  nlohmann::json smtp_passport_json;
  smtp_passport_json["smtp_address"] =
      ui->smtpServerAddressEdit->text().toStdString();

  smtp_passport_json["username"] = ui->usernameEdit->text().toStdString();

  smtp_passport_json["password"] = ui->passwordEdit->text().toStdString();

  smtp_passport_json["port"] = ui->portSpin->value();

  smtp_passport_json["connection_type"] =
      ui->connextionSecurityComboBox->currentText().toStdString();

  smtp_passport_json["default_sender"] =
      ui->defaultSenderEmailEdit->text().toStdString();

  smtp_passport_json["default_sender_gpg_key_id"] =
      ui->gpgKeyIDEdit->text().toStdString();

  smtp_passport_json["identity_enable"] = ui->identityCheckBox->isChecked();

  smtp_passport_json["enable"] = ui->enableCheckBox->isChecked();

  GpgFrontend::UI::GlobalSettingStation::GetInstance().SaveDataObj(
      "smtp_passport", smtp_passport_json);
}

#ifdef SMTP_SUPPORT
void SendMailTab::slotCheckConnection() {
  auto host = ui->smtpServerAddressEdit->text().toStdString();
  auto port = ui->portSpin->value();
  auto connection_type = connection_type_;
  bool identity_needed = ui->identityCheckBox->isChecked();
  auto username = ui->usernameEdit->text().toStdString();
  auto password = ui->passwordEdit->text().toStdString();

  auto thread = new SMTPTestThread(host, port, connection_type, identity_needed,
                                   username, password);

  // Waiting Dialog
  auto* waiting_dialog = new QProgressDialog(this);
  waiting_dialog->setMaximum(0);
  waiting_dialog->setMinimum(0);
  auto waiting_dialog_label =
      new QLabel(QString(_("Test SMTP Connection...")) + "<br /><br />" +
                 _("If the process does not end for a long time, please check "
                   "again whether your SMTP server configuration is correct."));
  waiting_dialog_label->setWordWrap(true);
  waiting_dialog->setLabel(waiting_dialog_label);
  waiting_dialog->resize(420, 120);
  connect(thread, &SMTPTestThread::signalSMTPTestResult, this,
          &SendMailTab::slotTestSMTPConnectionResult);
  connect(thread, &QThread::finished, [=]() {
    waiting_dialog->finished(0);
    waiting_dialog->deleteLater();
  });
  connect(waiting_dialog, &QProgressDialog::canceled, [=]() {
    LOG(INFO) << "cancel clicked";
    if (thread->isRunning()) thread->terminate();
  });

  // Show Waiting Dialog
  waiting_dialog->show();
  waiting_dialog->setFocus();

  thread->start();
  QEventLoop loop;
  connect(thread, &QThread::finished, &loop, &QEventLoop::quit);
  loop.exec();
}
#endif

#ifdef SMTP_SUPPORT
void SendMailTab::slotSendTestMail() {
  auto host = ui->smtpServerAddressEdit->text().toStdString();
  auto port = ui->portSpin->value();
  auto connection_type = connection_type_;
  bool identity_needed = ui->identityCheckBox->isChecked();
  auto username = ui->usernameEdit->text().toStdString();
  auto password = ui->passwordEdit->text().toStdString();
  auto sender_address = ui->defaultSenderEmailEdit->text();

  auto thread = new SMTPSendMailThread(host, port, connection_type,
                                       identity_needed, username, password);

  // Waiting Dialog
  auto* waiting_dialog = new QProgressDialog(this);
  waiting_dialog->setMaximum(0);
  waiting_dialog->setMinimum(0);
  auto waiting_dialog_label =
      new QLabel(QString(_("Test SMTP Send Mail Ability...")) + "<br /><br />" +
                 _("If the process does not end for a long time, please check "
                   "again whether your SMTP server configuration is correct."));
  waiting_dialog_label->setWordWrap(true);
  waiting_dialog->setLabel(waiting_dialog_label);
  waiting_dialog->resize(420, 120);
  connect(thread, &SMTPSendMailThread::signalSMTPResult, this,
          &SendMailTab::slotTestSMTPConnectionResult);
  connect(thread, &QThread::finished, [=]() {
    waiting_dialog->finished(0);
    waiting_dialog->deleteLater();
  });
  connect(waiting_dialog, &QProgressDialog::canceled, [=]() {
    LOG(INFO) << "cancel clicked";
    if (thread->isRunning()) thread->terminate();
  });

  thread->setSender(sender_address);
  thread->setRecipient(sender_address);
  thread->setSubject(_("Test Email from GpgFrontend"));
  thread->addTextContent(
      _("Hello, this is a test email from GpgFrontend. If you receive this "
        "email, it means that you have configured the correct SMTP server "
        "parameters."));

  // Show Waiting Dialog
  waiting_dialog->show();
  waiting_dialog->setFocus();

  thread->start();
  QEventLoop loop;
  connect(thread, &QThread::finished, &loop, &QEventLoop::quit);
  loop.exec();
}

void SendMailTab::slotTestSMTPConnectionResult(const QString& result) {
  if (result == "Fail to connect SMTP server") {
    QMessageBox::critical(this, _("Fail"), _("Fail to Connect SMTP Server."));
    ui->senTestMailButton->setDisabled(true);
  } else if (result == "Fail to login") {
    QMessageBox::critical(this, _("Fail"), _("Fail to Login."));
    ui->senTestMailButton->setDisabled(true);
  } else if (result == "Fail to send mail") {
    QMessageBox::critical(this, _("Fail"), _("Fail to Login."));
    ui->senTestMailButton->setDisabled(true);
  } else if (result == "Succeed in testing connection") {
    QMessageBox::information(this, _("Success"),
                             _("Succeed in connecting and login"));
    ui->senTestMailButton->setDisabled(false);
  } else if (result == "Succeed in sending a test email") {
    QMessageBox::information(
        this, _("Success"),
        _("Succeed in sending a test email to the SMTP Server"));
    ui->senTestMailButton->setDisabled(false);
  } else {
    QMessageBox::critical(this, _("Fail"), _("Unknown error."));
    ui->senTestMailButton->setDisabled(true);
  }
}

void SendMailTab::switch_ui_enabled(bool enabled) {
  ui->smtpServerAddressEdit->setDisabled(!enabled);
  ui->portSpin->setDisabled(!enabled);
  ui->connextionSecurityComboBox->setDisabled(!enabled);

  ui->identityCheckBox->setDisabled(!enabled);
  ui->usernameEdit->setDisabled(!enabled);
  ui->passwordEdit->setDisabled(!enabled);

  ui->defaultSenderEmailEdit->setDisabled(!enabled);
  ui->gpgKeyIDEdit->setDisabled(!enabled);
  ui->checkConnectionButton->setDisabled(!enabled);
}

void SendMailTab::switch_ui_identity_enabled(bool enabled) {
  ui->usernameEdit->setDisabled(!enabled);
  ui->passwordEdit->setDisabled(!enabled);
}
#endif

}  // namespace GpgFrontend::UI
