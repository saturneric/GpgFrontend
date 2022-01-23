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

#include "SettingsSendMail.h"

#include "ui/struct/SettingsObject.h"
#include "ui/thread/SMTPConnectionTestThread.h"
#include "ui/thread/SMTPSendMailThread.h"
#include "ui_SendMailSettings.h"

namespace GpgFrontend::UI {

SendMailTab::SendMailTab(QWidget* parent)
    : QWidget(parent), ui_(std::make_shared<Ui_SendMailSettings>()) {
  ui_->setupUi(this);

  connect(ui_->enableCheckBox, &QCheckBox::stateChanged, this,
          [=](int state) { switch_ui_enabled(state == Qt::Checked); });

#ifdef SMTP_SUPPORT
  connect(ui_->checkConnectionButton, &QPushButton::clicked, this,
          &SendMailTab::slot_check_connection);
  connect(ui_->senTestMailButton, &QPushButton::clicked, this,
          &SendMailTab::slot_send_test_mail);
#endif

  connect(ui_->identityCheckBox, &QCheckBox::stateChanged, this,
          [=](int state) { switch_ui_identity_enabled(state == Qt::Checked); });

  connect(ui_->connextionSecurityComboBox, &QComboBox::currentTextChanged, this,
          [=](const QString& current_text) {
            if (current_text == "SSL") {
              connection_type_ = SmtpClient::ConnectionType::SslConnection;
            } else if (current_text == "TLS" || current_text == "STARTTLS") {
              connection_type_ = SmtpClient::ConnectionType::TlsConnection;
            } else {
              connection_type_ = SmtpClient::ConnectionType::TcpConnection;
            }
          });

  ui_->generalGroupBox->setTitle(_("General"));
  ui_->identityGroupBox->setTitle(_("Identity Information"));
  ui_->preferenceGroupBox->setTitle(_("Preference"));
  ui_->operationsGroupBox->setTitle(_("Operations"));

  ui_->enableCheckBox->setText(_("Enable Send Mail Ability"));
  ui_->identityCheckBox->setText(_("Need Auth"));

  ui_->smtpServerAddressLabel->setText(_("SMTP Server Address"));
  ui_->smtpServerPortLabel->setText(_("SMTP Server Port"));
  ui_->connectionSecurityLabel->setText(_("SMTP Connection Security"));
  ui_->usernameLabel->setText(_("Username"));
  ui_->passwordLabel->setText(_("Password"));

  ui_->senderLabel->setText(_("Default Sender Email"));
  ui_->checkConnectionButton->setText(_("Check Connection"));
  ui_->senTestMailButton->setText(_("Send Test Email"));
  ui_->gpgkeyIdLabel->setText(_("Default Sender GPG Key ID"));

  ui_->tipsLabel->setText(
      _("Tips: It is recommended that you build your own mail server or use "
        "a trusted mail server. If you don't know the detailed configuration "
        "information, you can get it from the mail service provider."));

  ui_->senTestMailButton->setDisabled(true);

  auto* email_validator =
      new QRegularExpressionValidator(re_email_, ui_->defaultSenderEmailEdit);
  ui_->defaultSenderEmailEdit->setValidator(email_validator);

  SetSettings();
}

void SendMailTab::SetSettings() {
  auto smtp_passport = SettingsObject("smtp_passport");

  ui_->smtpServerAddressEdit->setText(
      std::string{smtp_passport.Check("smtp_address", {})}.c_str());

  ui_->usernameEdit->setText(
      std::string{smtp_passport.Check("username", {})}.c_str());

  ui_->passwordEdit->setText(
      std::string{smtp_passport.Check("password", {})}.c_str());

  ui_->portSpin->setValue(int{smtp_passport.Check("port", 25)});

  ui_->connextionSecurityComboBox->setCurrentText(
      std::string{smtp_passport.Check("connection_type", "None")}.c_str());

  ui_->defaultSenderEmailEdit->setText(
      std::string{smtp_passport.Check("default_sender", {})}.c_str());

  ui_->gpgKeyIDEdit->setText(
      std::string{smtp_passport.Check("default_sender_gpg_key_id", {})}
          .c_str());

  ui_->identityCheckBox->setChecked(
      bool{smtp_passport.Check("identity_enable", false)});

  ui_->enableCheckBox->setChecked(bool{smtp_passport.Check("enable", false)});

  {
    auto state = ui_->identityCheckBox->checkState();
    switch_ui_identity_enabled(state == Qt::Checked);
  }

  {
    auto state = ui_->enableCheckBox->checkState();
    switch_ui_enabled(state == Qt::Checked);
  }
}

void SendMailTab::ApplySettings() {
  try {
    auto smtp_passport = SettingsObject("smtp_passport");

    smtp_passport["smtp_address"] =
        ui_->smtpServerAddressEdit->text().toStdString();

    smtp_passport["username"] = ui_->usernameEdit->text().toStdString();

    smtp_passport["password"] = ui_->passwordEdit->text().toStdString();

    smtp_passport["port"] = ui_->portSpin->value();

    smtp_passport["connection_type"] =
        ui_->connextionSecurityComboBox->currentText().toStdString();

    smtp_passport["default_sender"] =
        ui_->defaultSenderEmailEdit->text().toStdString();

    smtp_passport["default_sender_gpg_key_id"] =
        ui_->gpgKeyIDEdit->text().toStdString();

    smtp_passport["identity_enable"] = ui_->identityCheckBox->isChecked();

    smtp_passport["enable"] = ui_->enableCheckBox->isChecked();

  } catch (...) {
    LOG(ERROR) << _("apply settings failed");
  }
}

#ifdef SMTP_SUPPORT
void SendMailTab::slot_check_connection() {
  auto host = ui_->smtpServerAddressEdit->text().toStdString();
  auto port = ui_->portSpin->value();
  auto connection_type = connection_type_;
  bool identity_needed = ui_->identityCheckBox->isChecked();
  auto username = ui_->usernameEdit->text().toStdString();
  auto password = ui_->passwordEdit->text().toStdString();

  auto thread = new SMTPConnectionTestThread(
      host, port, connection_type, identity_needed, username, password);

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
  connect(thread, &SMTPConnectionTestThread::SignalSMTPConnectionTestResult,
          this, &SendMailTab::slot_test_smtp_connection_result);
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
void SendMailTab::slot_send_test_mail() {
  auto host = ui_->smtpServerAddressEdit->text().toStdString();
  auto port = ui_->portSpin->value();
  auto connection_type = connection_type_;
  bool identity_needed = ui_->identityCheckBox->isChecked();
  auto username = ui_->usernameEdit->text().toStdString();
  auto password = ui_->passwordEdit->text().toStdString();
  auto sender_address = ui_->defaultSenderEmailEdit->text();

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
  connect(thread, &SMTPSendMailThread::SignalSMTPResult, this,
          &SendMailTab::slot_test_smtp_connection_result);
  connect(thread, &QThread::finished, [=]() {
    waiting_dialog->finished(0);
    waiting_dialog->deleteLater();
  });
  connect(waiting_dialog, &QProgressDialog::canceled, [=]() {
    LOG(INFO) << "cancel clicked";
    if (thread->isRunning()) thread->terminate();
  });

  thread->SetSender(sender_address);
  thread->SetRecipient(sender_address);
  thread->SetSubject(_("Test Email from GpgFrontend"));
  thread->AddTextContent(
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

void SendMailTab::slot_test_smtp_connection_result(const QString& result) {
  if (result == "Fail to connect SMTP server") {
    QMessageBox::critical(this, _("Fail"), _("Fail to Connect SMTP Server."));
    ui_->senTestMailButton->setDisabled(true);
  } else if (result == "Fail to login") {
    QMessageBox::critical(this, _("Fail"), _("Fail to Login."));
    ui_->senTestMailButton->setDisabled(true);
  } else if (result == "Fail to send mail") {
    QMessageBox::critical(this, _("Fail"), _("Fail to Login."));
    ui_->senTestMailButton->setDisabled(true);
  } else if (result == "Succeed in testing connection") {
    QMessageBox::information(this, _("Success"),
                             _("Succeed in connecting and login"));
    ui_->senTestMailButton->setDisabled(false);
  } else if (result == "Succeed in sending a test email") {
    QMessageBox::information(
        this, _("Success"),
        _("Succeed in sending a test email to the SMTP Server"));
    ui_->senTestMailButton->setDisabled(false);
  } else {
    QMessageBox::critical(this, _("Fail"), _("Unknown error."));
    ui_->senTestMailButton->setDisabled(true);
  }
}

void SendMailTab::switch_ui_enabled(bool enabled) {
  ui_->smtpServerAddressEdit->setDisabled(!enabled);
  ui_->portSpin->setDisabled(!enabled);
  ui_->connextionSecurityComboBox->setDisabled(!enabled);

  ui_->identityCheckBox->setDisabled(!enabled);
  ui_->usernameEdit->setDisabled(!enabled);
  ui_->passwordEdit->setDisabled(!enabled);

  ui_->defaultSenderEmailEdit->setDisabled(!enabled);
  ui_->gpgKeyIDEdit->setDisabled(!enabled);
  ui_->checkConnectionButton->setDisabled(!enabled);
}

void SendMailTab::switch_ui_identity_enabled(bool enabled) {
  ui_->usernameEdit->setDisabled(!enabled);
  ui_->passwordEdit->setDisabled(!enabled);
}
#endif

}  // namespace GpgFrontend::UI
