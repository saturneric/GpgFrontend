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

#include "ui/SendMailDialog.h"
#include "smtp/SmtpMime"

SendMailDialog::SendMailDialog(QWidget *parent) : QDialog(parent), appPath(qApp->applicationDirPath()),
                                                  settings(RESOURCE_DIR(appPath) + "/conf/gpgfrontend.ini",
                                                           QSettings::IniFormat) {

    senderEdit = new QLineEdit();
    senderEdit->setText(defaultSender);
    recipientEdit = new QLineEdit();
    subjectEdit = new QLineEdit();

    qDebug() << "Send Mail Settings" << smtpAddress << username << password << defaultSender << connectionTypeSettings;

    confirmButton = new QPushButton("Confirm");

    auto layout = new QGridLayout();
    layout->addWidget(new QLabel("Sender"), 0, 0);
    layout->addWidget(senderEdit, 0, 1);
    layout->addWidget(new QLabel("Recipient"), 1, 0);
    layout->addWidget(recipientEdit, 1, 1);
    layout->addWidget(new QLabel("Subject"), 2, 0);
    layout->addWidget(subjectEdit, 2, 1);
    layout->addWidget(confirmButton, 3, 1);

    connect(confirmButton, SIGNAL(clicked(bool)), this, SLOT(slotConfirm()));

    this->setLayout(layout);
    this->setWindowTitle("Send Mail");
    this->setModal(true);
    this->setFixedSize(320, 160);
    this->show();
}

void SendMailDialog::slotConfirm() {

    SmtpClient::ConnectionType connectionType = SmtpClient::ConnectionType::TcpConnection;

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

    message.setSender(new EmailAddress(defaultSender));
    message.addRecipient(new EmailAddress("eric@bktus.com"));
    message.setSubject("GpgFrontend Mail Test");

    // Now add some text to the email.
    // First we create a MimeText object.

    MimeText text;

    text.setText("Hi,\nThis is a simple email message.\n");

    // Now add it to the mail
    message.addPart(&text);

    // Now we can send the mail
    if (!smtp.connectToHost()) {
        qDebug() << "Connect to SMTP Server Failed";
        return;
    }
    if (!smtp.login()) {
        qDebug() << "Login to SMTP Server Failed";
        return;
    }
    if (!smtp.sendMail(message)) {
        qDebug() << "Send Mail to SMTP Server Failed";
        return;
    }
    smtp.quit();
}
