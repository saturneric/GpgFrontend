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

#ifdef SMTP_SUPPORT
#include "smtp/SmtpMime"
#endif

namespace GpgFrontend::UI {

SendMailDialog::SendMailDialog(QString text, QWidget* parent)
    : QDialog(parent),
      appPath(qApp->applicationDirPath()),
      settings(RESOURCE_DIR(appPath) + "/conf/gpgfrontend.ini",
               QSettings::IniFormat),
      mText(std::move(text)) {
  if (smtpAddress.isEmpty()) {
    QMessageBox::critical(
        this, _("Incomplete configuration"),
        _("The SMTP address is empty, please go to the setting interface to "
          "complete the configuration."));

    deleteLater();
    return;
  }

  senderEdit = new QLineEdit();
  senderEdit->setText(defaultSender);
  recipientEdit = new QTextEdit();
  recipientEdit->setPlaceholderText(
      "One or more email addresses. Please use ; to separate.");
  subjectEdit = new QLineEdit();

  errorLabel = new QLabel();

  qDebug() << "Send Mail Settings" << smtpAddress << username << password
           << defaultSender << connectionTypeSettings;

  confirmButton = new QPushButton("Confirm");

  auto layout = new QGridLayout();
  layout->addWidget(new QLabel("Sender"), 0, 0);
  layout->addWidget(senderEdit, 0, 1);
  layout->addWidget(new QLabel("Recipient"), 1, 0);
  layout->addWidget(recipientEdit, 1, 1);
  layout->addWidget(new QLabel("Subject"), 2, 0);
  layout->addWidget(subjectEdit, 2, 1);
  layout->addWidget(confirmButton, 3, 1);
  layout->addWidget(errorLabel, 4, 0, 1, 2);

#ifdef SMTP_SUPPORT
  connect(confirmButton, SIGNAL(clicked(bool)), this, SLOT(slotConfirm()));
#endif

  this->setLayout(layout);
  this->setWindowTitle("Send Mail");
  this->setModal(true);
  this->setFixedWidth(320);
  this->show();
}

bool SendMailDialog::check_email_address(const QString& str) {
  return re_email.match(str).hasMatch();
}

#ifdef SMTP_SUPPORT

void SendMailDialog::slotConfirm() {
  QString errString;
  errorLabel->clear();

  QStringList rcptStringList = recipientEdit->toPlainText().split(';');

  if (rcptStringList.isEmpty()) {
    errString.append(QString("  ") + _("Recipient cannot be empty") + "  \n");
  } else {
    for (const auto& reci : rcptStringList) {
      qDebug() << "Receiver" << reci.trimmed();
      if (!check_email_address(reci.trimmed())) {
        errString.append(QString("  ") +
                         _("One or more Recipient's Email Address is invalid") +
                         "  \n");
        break;
      }
    }
  }
  if (senderEdit->text().isEmpty()) {
    errString.append(QString("  ") + _("Sender cannot be empty") + "  \n");
  } else if (!check_email_address(senderEdit->text())) {
    errString.append(QString("  ") + _("Sender's Email Address is invalid") +
                     "  \n");
  }

  if (!errString.isEmpty()) {
    errorLabel->setAutoFillBackground(true);
    QPalette error = errorLabel->palette();
    error.setColor(QPalette::Window, "#ff8080");
    errorLabel->setPalette(error);
    errorLabel->setText(errString);
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

  message.setSender(new EmailAddress(senderEdit->text()));
  for (const auto& reci : rcptStringList) {
    if (!reci.isEmpty()) message.addRecipient(new EmailAddress(reci.trimmed()));
  }
  message.setSubject(subjectEdit->text());

  // Now add some text to the email.
  // First we create a MimeText object.

  MimeText text;

  text.setText(mText);

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

#endif

}  // namespace GpgFrontend::UI
