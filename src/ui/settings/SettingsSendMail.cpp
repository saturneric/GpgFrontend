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

    auto generalGroupBox = new QGroupBox(tr("General"));
    auto connectGroupBox = new QGroupBox(tr("Connection"));
    auto preferenceGroupBox = new QGroupBox(tr("Preference"));

    auto generalLayout = new QGridLayout();
    generalLayout->addWidget(enableCheckBox);

    auto connectLayout = new QGridLayout();
    connectLayout->addWidget(new QLabel(tr("SMTP Address")), 1, 0);
    connectLayout->addWidget(smtpAddress, 1, 1, 1, 4);
    connectLayout->addWidget(new QLabel(tr("Username")), 2, 0);
    connectLayout->addWidget(username, 2, 1, 1, 4);
    connectLayout->addWidget(new QLabel(tr("Password")), 3, 0);
    connectLayout->addWidget(password, 3, 1, 1, 4);
    connectLayout->addWidget(new QLabel(tr("Port")), 4, 0);
    connectLayout->addWidget(portSpin, 4, 1, 1, 1);
    connectLayout->addWidget(new QLabel(tr("Connection Security")), 5, 0);
    connectLayout->addWidget(connectionTypeComboBox, 5, 1, 1, 1);
    connectLayout->addWidget(checkConnectionButton, 6, 0);

    auto preferenceLayout = new QGridLayout();

    preferenceLayout->addWidget(new QLabel(tr("Default Sender")), 0, 0);
    preferenceLayout->addWidget(defaultSender, 0, 1, 1, 4);

    generalGroupBox->setLayout(generalLayout);
    connectGroupBox->setLayout(connectLayout);
    preferenceGroupBox->setLayout(preferenceLayout);

    auto vBox = new QVBoxLayout();
    vBox->addWidget(generalGroupBox);
    vBox->addWidget(connectGroupBox);
    vBox->addWidget(preferenceGroupBox);
    vBox->addStretch(0);

    connect(enableCheckBox, SIGNAL(stateChanged(int)), this, SLOT(slotCheckBoxSetEnableDisable(int)));
    connect(checkConnectionButton, SIGNAL(clicked(bool)), this, SLOT(slotCheckConnection()));


    this->setLayout(vBox);
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

