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

#include "SendMailDialog.h"

#include <utility>

#include "ui_SendMailDialog.h"

#ifdef SMTP_SUPPORT
#include "smtp/SmtpMime"
#include "ui/settings/GlobalSettingStation.h"
#endif

namespace GpgFrontend::UI {

SendMailDialog::SendMailDialog(const QString& text, QWidget* parent)
    : QDialog(parent), ui(std::make_shared<Ui_SendMailDialog>()) {
  // read from settings
  initSettings();

  if (smtpAddress.isEmpty()) {
    QMessageBox::critical(
        this, _("Incomplete configuration"),
        _("The SMTP address is empty, please go to the setting interface to "
          "complete the configuration."));

    deleteLater();
    return;
  }

  ui->setupUi(this);

  ui->ccInputWidget->setHidden(true);
  ui->bccInputWidget->setHidden(true);
  ui->textEdit->setText(text);
  ui->errorLabel->setHidden(true);

  ui->senderEdit->setText(defaultSender);
  connect(ui->ccButton, &QPushButton::clicked, [=]() {
    ui->ccInputWidget->setHidden(!ui->ccInputWidget->isHidden());
    ui->ccEdit->clear();
  });
  connect(ui->bccButton, &QPushButton::clicked, [=]() {
    ui->bccInputWidget->setHidden(!ui->bccInputWidget->isHidden());
    ui->bccEdit->clear();
  });

#ifdef SMTP_SUPPORT
  connect(ui->sendMailButton, &QPushButton::clicked, this,
          &SendMailDialog::slotConfirm);
#endif

  ui->ccButton->setText(_("CC"));
  ui->bccButton->setText(_("BCC"));
  ui->senderLabel->setText(_("Sender"));
  ui->recipientLabel->setText(_("Recipient"));
  ui->subjectLabel->setText(_("Mail Subject"));
  ui->bccLabel->setText(_("BCC"));
  ui->ccLabel->setText(_("CC"));
  ui->tipsLabel->setText(
      _("Tips: You can fill in multiple email addresses, please separate them "
        "with \";\"."));
  ui->sendMailButton->setText(_("Send Mail"));

  this->setWindowTitle(_("Send Mail"));
  this->setAttribute(Qt::WA_DeleteOnClose);
}

bool SendMailDialog::check_email_address(const QString& str) {
  return re_email.match(str).hasMatch();
}

#ifdef SMTP_SUPPORT

void SendMailDialog::slotConfirm() {
  QString errString;
  ui->errorLabel->clear();
  ui->errorLabel->setHidden(true);
  QStringList rcpt_string_list = ui->recipientEdit->text().split(';');
  QStringList cc_string_list = ui->ccEdit->text().split(';');
  QStringList bcc_string_list = ui->bccEdit->text().split(';');

  if (rcpt_string_list.isEmpty()) {
    errString.append(QString("  ") + _("Recipient cannot be empty") + "  \n");
  } else {
    for (const auto& reci : rcpt_string_list) {
      LOG(INFO) << "Receiver" << reci.trimmed().toStdString();
      if (!check_email_address(reci.trimmed())) {
        errString.append(QString("  ") +
                         _("One or more recipient's email is invalid") +
                         "  \n");
        break;
      }
    }
  }
  if (ui->senderEdit->text().isEmpty()) {
    errString.append(QString("  ") + _("Sender cannot be empty") + "  \n");
  } else if (!check_email_address(ui->senderEdit->text())) {
    errString.append(QString("  ") + _("Sender's email is invalid") + "  \n");
  }

  if (ui->subjectEdit->text().isEmpty()) {
    errString.append(QString("  ") + _("Subject cannot be empty") + "  \n");
  }

  if (!ui->ccEdit->text().isEmpty())
    for (const auto& cc : cc_string_list) {
      LOG(INFO) << "cc" << cc.trimmed().toStdString();
      if (!check_email_address(cc.trimmed())) {
        errString.append(QString("  ") + _("One or more cc email is invalid") +
                         "  \n");
        break;
      }
    }

  if (!ui->bccEdit->text().isEmpty())
    for (const auto& bcc : bcc_string_list) {
      LOG(INFO) << "bcc" << bcc.trimmed().toStdString();
      if (!check_email_address(bcc.trimmed())) {
        errString.append(QString("  ") + _("One or more bcc email is invalid") +
                         "  \n");
        break;
      }
    }

  if (!errString.isEmpty()) {
    ui->errorLabel->setAutoFillBackground(true);
    QPalette error = ui->errorLabel->palette();
    error.setColor(QPalette::Window, "#ff8080");
    ui->errorLabel->setPalette(error);
    ui->errorLabel->setText(errString);
    ui->errorLabel->setHidden(false);
    return;
  }

  SmtpClient::ConnectionType connectionType =
      SmtpClient::ConnectionType::TcpConnection;

  if (connectionTypeSettings == "SSL") {
    connectionType = SmtpClient::ConnectionType::SslConnection;
  } else if (connectionTypeSettings == "TLS") {
    connectionType = SmtpClient::ConnectionType::TlsConnection;
  } else if (connectionTypeSettings == "STARTTLS") {
    connectionType = SmtpClient::ConnectionType::TlsConnection;
  } else {
    connectionType = SmtpClient::ConnectionType::TcpConnection;
  }

  SmtpClient smtp(smtpAddress, port, connectionType);

  // We need to set the username (your email address) and the password
  // for smtp authentification.

  smtp.setUser(username);
  smtp.setPassword(password);

  // Now we create a MimeMessage object. This will be the email.

  MimeMessage message;

  message.setSender(new EmailAddress(ui->senderEdit->text()));
  for (const auto& reci : rcpt_string_list) {
    if (!reci.isEmpty()) message.addRecipient(new EmailAddress(reci.trimmed()));
  }
  for (const auto& cc : cc_string_list) {
    if (!cc.isEmpty()) message.addCc(new EmailAddress(cc.trimmed()));
  }
  for (const auto& bcc : cc_string_list) {
    if (!bcc.isEmpty()) message.addBcc(new EmailAddress(bcc.trimmed()));
  }
  message.setSubject(ui->subjectEdit->text());

  // Now add some text to the email.
  // First we create a MimeText object.

  MimeText text;
  text.setText(ui->textEdit->toPlainText());

  // Now add it to the mail
  message.addPart(&text);

  // Now we can send the mail
  if (!smtp.connectToHost()) {
    qDebug() << "Connect to SMTP Server Failed";
    QMessageBox::critical(this, _("Fail"), _("Fail to Connect SMTP Server"));
    return;
  }
  if (!smtp.login()) {
    qDebug() << "Login to SMTP Server Failed";
    QMessageBox::critical(this, _("Fail"), _("Fail to Login into SMTP Server"));
    return;
  }
  if (!smtp.sendMail(message)) {
    qDebug() << "Send Mail to SMTP Server Failed";
    QMessageBox::critical(this, _("Fail"),
                          _("Fail to Send Mail to SMTP Server"));
    return;
  }
  smtp.quit();

  // Close after sending email
  QMessageBox::information(this, _("Success"),
                           _("Succeed in Sending Mail to SMTP Server"));
  deleteLater();
}

void SendMailDialog::initSettings() {
  auto& settings = GlobalSettingStation::GetInstance().GetUISettings();

  try {
    ability_enable = settings.lookup("smtp.enable");
  } catch (...) {
    LOG(ERROR) << _("Setting Operation Error") << _("save_key_checked");
  }

  try {
    identity_enable = settings.lookup("smtp.identity_enable");
  } catch (...) {
    LOG(ERROR) << _("Setting Operation Error") << _("identity_enable");
  }

  try {
    smtpAddress = settings.lookup("smtp.mail_address").c_str();
  } catch (...) {
    LOG(ERROR) << _("Setting Operation Error") << _("mail_address");
  }

  try {
    username = settings.lookup("smtp.username").c_str();
  } catch (...) {
    LOG(ERROR) << _("Setting Operation Error") << _("username");
  }

  try {
    password = settings.lookup("smtp.password").c_str();
  } catch (...) {
    LOG(ERROR) << _("Setting Operation Error") << _("password");
  }

  try {
    port = settings.lookup("smtp.port");
  } catch (...) {
    LOG(ERROR) << _("Setting Operation Error") << _("port");
  }

  try {
    connectionTypeSettings = settings.lookup("smtp.connection_type").c_str();
  } catch (...) {
    LOG(ERROR) << _("Setting Operation Error") << _("connection_type");
  }

  try {
    defaultSender = settings.lookup("smtp.default_sender").c_str();
  } catch (...) {
    LOG(ERROR) << _("Setting Operation Error") << _("default_sender");
  }
}

#endif

}  // namespace GpgFrontend::UI
