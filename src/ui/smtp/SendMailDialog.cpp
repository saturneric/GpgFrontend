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

#include "gpg/function/GpgKeyGetter.h"
#include "ui/smtp/EmailListEditor.h"
#include "ui/smtp/RecipientsPicker.h"
#include "ui/smtp/SenderPicker.h"
#include "ui_SendMailDialog.h"

#ifdef SMTP_SUPPORT
#include "smtp/SmtpMime"
#include "ui/function/SMTPSendMailThread.h"
#include "ui/settings/GlobalSettingStation.h"
#endif

namespace GpgFrontend::UI {

SendMailDialog::SendMailDialog(const QString& text, QWidget* parent)
    : QDialog(parent), ui(std::make_shared<Ui_SendMailDialog>()) {
  // read from settings
  initSettings();

  if (smtp_address_.isEmpty()) {
    QMessageBox::critical(
        this->parentWidget(), _("Incomplete configuration"),
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

  ui->senderEdit->setText(default_sender_);

  if (!default_sender_gpg_key_id.isEmpty()) {
    auto key = GpgKeyGetter::GetInstance().GetKey(
        default_sender_gpg_key_id.toStdString());
    if (key.good() && key.is_private_key() && key.CanSignActual()) {
      sender_key_id_ = key.id();
      set_sender_value_label();
    }
  }

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

  connect(ui->senderKeySelectButton, &QPushButton::clicked, this, [=]() {
    auto picker = new SenderPicker(sender_key_id_, this);
    sender_key_id_ = picker->getCheckedSender();
    set_sender_value_label();
  });

  connect(ui->recipientKeySelectButton, &QPushButton::clicked, this, [=]() {
    auto picker = new RecipientsPicker(recipients_key_ids_, this);
    recipients_key_ids_ = picker->getCheckedRecipients();
    set_recipients_value_label();
  });

  connect(ui->recipientsEditButton, &QPushButton::clicked, this, [=]() {
    auto editor = new EmailListEditor(ui->recipientEdit->text(), this);
    ui->recipientEdit->setText(editor->getEmailList());
  });

  connect(ui->ccEditButton, &QPushButton::clicked, this, [=]() {
    auto editor = new EmailListEditor(ui->ccEdit->text(), this);
    ui->ccEdit->setText(editor->getEmailList());
  });

  connect(ui->bccEditButton, &QPushButton::clicked, this, [=]() {
    auto editor = new EmailListEditor(ui->bccEdit->text(), this);
    ui->bccEdit->setText(editor->getEmailList());
  });

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
  ui->sendMailButton->setText(_("Send Message"));
  ui->senderKeySelectButton->setText(_("Select Sender GPG Key"));
  ui->recipientKeySelectButton->setText(_("Select Recipient(s) GPG Key"));
  ui->gpgOperaLabel->setText(_("GPG Operations"));
  ui->attacSignatureCheckBox->setText(_("Attach signature"));
  ui->attachSenderPublickeyCheckBox->setText(_("Attach sender's public key"));
  ui->contentEncryptCheckBox->setText(_("Encrypt content"));
  ui->recipientEdit->setText(_("Edit Recipients(s)"));
  ui->ccEdit->setText(_("Edit CC(s)"));
  ui->bccEdit->setText(_("Edit BCC(s)"));
  ui->senderKeyLabel->setText(_("Sender GPG Key: "));
  ui->recipientKeysLabel->setText(_("Recipient(s) GPG Key: "));

  auto pos = QPoint(100, 100);
  LOG(INFO) << "parent" << parent;
  if (parent) pos += parent->pos();
  LOG(INFO) << "pos default" << pos.x() << pos.y();

  move(pos);

  this->setWindowTitle(_("New Message"));
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

  SmtpClient::ConnectionType connection_type_ =
      SmtpClient::ConnectionType::TcpConnection;

  if (connection_type_settings_ == "SSL") {
    connection_type_ = SmtpClient::ConnectionType::SslConnection;
  } else if (connection_type_settings_ == "TLS") {
    connection_type_ = SmtpClient::ConnectionType::TlsConnection;
  } else if (connection_type_settings_ == "STARTTLS") {
    connection_type_ = SmtpClient::ConnectionType::TlsConnection;
  } else {
    connection_type_ = SmtpClient::ConnectionType::TcpConnection;
  }

  auto host = smtp_address_.toStdString();
  auto port = port_;
  auto connection_type = connection_type_;
  bool identity_needed = identity_enable_;
  auto username = username_.toStdString();
  auto password = password_.toStdString();
  auto sender_address = ui->senderEdit->text().toStdString();

  auto thread = new SMTPSendMailThread(
      host, port, connection_type, identity_needed, username, password, this);

  thread->setSender(ui->senderEdit->text());
  thread->setRecipient(ui->recipientEdit->text());
  thread->setCC(ui->ccEdit->text());
  thread->setBCC(ui->bccEdit->text());
  thread->setSubject(ui->subjectEdit->text());
  thread->addTextContent(ui->textEdit->toPlainText());

  if (ui->contentEncryptCheckBox->checkState() == Qt::Checked) {
    if (recipients_key_ids_ == nullptr) {
      QMessageBox::critical(
          this, _("Forbidden"),
          _("You have checked the encrypted email content, but you have not "
            "selected the recipient's GPG key. This is dangerous and the mail "
            "will not be encrypted. So the send operation is forbidden"));
      return;
    } else {
      auto key_ids = std::make_unique<KeyIdArgsList>();
      for (const auto& key_id : *recipients_key_ids_)
        key_ids->push_back(key_id);
      thread->setEncryptContent(true, std::move(key_ids));
    }
  }

  if (ui->attacSignatureCheckBox->checkState() == Qt::Checked) {
    if (sender_key_id_.empty()) {
      QMessageBox::critical(
          this, _("Forbidden"),
          _("You checked the option to attach signature to the email, but did "
            "not specify the sender's GPG Key. This will cause the content of "
            "the email to be inconsistent with your expectations, so the "
            "operation is prohibited."));
      return;
    } else {
      thread->setAttachSignatureFile(true, sender_key_id_);
    }
  }

  if (ui->attachSenderPublickeyCheckBox->checkState() == Qt::Checked) {
    if (sender_key_id_.empty()) {
      QMessageBox::critical(
          this, _("Forbidden"),
          _("You checked the option to attach your public key to the email, "
            "but did not specify the sender's GPG Key. This will cause the "
            "content of "
            "the email to be inconsistent "
            "with your expectations, so the operation is prohibited."));
      return;
    } else {
      thread->setAttachPublicKey(true, sender_key_id_);
    }
  }

  // Waiting Dialog
  auto* waiting_dialog = new QProgressDialog(this);
  waiting_dialog->setMaximum(0);
  waiting_dialog->setMinimum(0);
  auto waiting_dialog_label =
      new QLabel(QString(_("Sending Email...")) + "<br /><br />" +
                 _("If the process does not end for a long time, please check "
                   "again whether your SMTP server configuration is correct."));
  waiting_dialog_label->setWordWrap(true);
  waiting_dialog->setLabel(waiting_dialog_label);
  waiting_dialog->resize(420, 120);
  connect(thread, &SMTPSendMailThread::signalSMTPResult, this,
          &SendMailDialog::slotTestSMTPConnectionResult);
  connect(thread, &QThread::finished, [=]() {
    waiting_dialog->finished(0);
    waiting_dialog->deleteLater();
  });
  connect(waiting_dialog, &QProgressDialog::canceled, [=]() {
    LOG(INFO) << "cancel clicked";
    if (thread->isRunning()) thread->terminate();
    QCoreApplication::quit();
    exit(0);
  });

  // Show Waiting Dialog
  waiting_dialog->show();
  waiting_dialog->setFocus();

  thread->start();
  QEventLoop loop;
  connect(thread, &QThread::finished, &loop, &QEventLoop::quit);
  loop.exec();
}

void SendMailDialog::initSettings() {
  auto& settings = GlobalSettingStation::GetInstance().GetUISettings();

  try {
    ability_enable_ = settings.lookup("smtp.enable");
  } catch (...) {
    LOG(ERROR) << _("Setting Operation Error") << _("save_key_checked");
  }

  try {
    identity_enable_ = settings.lookup("smtp.identity_enable");
  } catch (...) {
    LOG(ERROR) << _("Setting Operation Error") << _("identity_enable");
  }

  try {
    smtp_address_ = settings.lookup("smtp.mail_address").c_str();
  } catch (...) {
    LOG(ERROR) << _("Setting Operation Error") << _("mail_address");
  }

  try {
    username_ = settings.lookup("smtp.username").c_str();
  } catch (...) {
    LOG(ERROR) << _("Setting Operation Error") << _("username");
  }

  try {
    password_ = settings.lookup("smtp.password").c_str();
  } catch (...) {
    LOG(ERROR) << _("Setting Operation Error") << _("password");
  }

  try {
    port_ = settings.lookup("smtp.port");
  } catch (...) {
    LOG(ERROR) << _("Setting Operation Error") << _("port");
  }

  try {
    connection_type_settings_ = settings.lookup("smtp.connection_type").c_str();
  } catch (...) {
    LOG(ERROR) << _("Setting Operation Error") << _("connection_type");
  }

  try {
    default_sender_ = settings.lookup("smtp.default_sender").c_str();
  } catch (...) {
    LOG(ERROR) << _("Setting Operation Error") << _("default_sender");
  }

  try {
    default_sender_gpg_key_id =
        settings.lookup("smtp.default_sender_gpg_key_id").c_str();
  } catch (...) {
    LOG(ERROR) << _("Setting Operation Error")
               << _("default_sender_gpg_key_id");
  }
}
#endif

void SendMailDialog::set_sender_value_label() {
  auto key = GpgKeyGetter::GetInstance().GetKey(sender_key_id_);
  if (key.good()) {
    ui->senderKeyValueLabel->setText(key.uids()->front().uid().c_str());
  }
}

void SendMailDialog::set_recipients_value_label() {
  auto keys = GpgKeyGetter::GetInstance().GetKeys(recipients_key_ids_);
  std::stringstream ss;
  for (const auto& key : *keys) {
    if (key.good()) {
      ss << key.uids()->front().uid().c_str() << ";";
    }
  }
  ui->recipientsKeyValueLabel->setText(ss.str().c_str());
}

void SendMailDialog::slotTestSMTPConnectionResult(const QString& result) {
  if (result == "Fail to connect SMTP server") {
    QMessageBox::critical(this, _("Fail"), _("Fail to Connect SMTP Server."));
  } else if (result == "Fail to login") {
    QMessageBox::critical(this, _("Fail"), _("Fail to Login."));
  } else if (result == "Fail to send mail") {
    QMessageBox::critical(this, _("Fail"), _("Fail to Login."));
  } else if (result == "Succeed in testing connection") {
    QMessageBox::information(this, _("Success"),
                             _("Succeed in connecting and login"));
  } else if (result == "Succeed in sending a test email") {
    QMessageBox::information(
        this, _("Success"),
        _("Succeed in sending the message to the SMTP Server"));
  } else {
    QMessageBox::critical(this, _("Fail"), _("Unknown error."));
  }
}
void SendMailDialog::setContentEncryption(bool on) {
  ui->contentEncryptCheckBox->setCheckState(on ? Qt::Checked : Qt::Unchecked);
}

void SendMailDialog::setAttachSignature(bool on) {
  ui->attacSignatureCheckBox->setCheckState(on ? Qt::Checked : Qt::Unchecked);
}

}  // namespace GpgFrontend::UI
