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

#include "SendMailDialog.h"

#include "core/function/gpg/GpgKeyGetter.h"
#include "ui/mail/EmailListEditor.h"
#include "ui/mail/RecipientsPicker.h"
#include "ui/mail/SenderPicker.h"
#include "ui_SendMailDialog.h"

#ifdef SMTP_SUPPORT
#include "ui/settings/GlobalSettingStation.h"
#include "ui/thread/SMTPSendMailThread.h"
#endif

namespace GpgFrontend::UI {

SendMailDialog::SendMailDialog(const QString& text, QWidget* parent)
    : QDialog(parent), ui_(std::make_shared<Ui_SendMailDialog>()) {
  // read from settings
  init_settings();

  if (smtp_address_.isEmpty()) {
    QMessageBox::critical(
        this->parentWidget(), _("Incomplete configuration"),
        _("The SMTP address is empty, please go to the setting interface to "
          "complete the configuration."));

    deleteLater();
    return;
  }

  ui_->setupUi(this);

  ui_->ccInputWidget->setHidden(true);
  ui_->bccInputWidget->setHidden(true);
  ui_->textEdit->setText(text);
  ui_->errorLabel->setHidden(true);

  ui_->senderEdit->setText(default_sender_);

  if (!default_sender_gpg_key_id_.isEmpty()) {
    auto key = GpgKeyGetter::GetInstance().GetKey(
        default_sender_gpg_key_id_.toStdString());
    if (key.IsGood() && key.IsPrivateKey() &&
        key.IsHasActualSigningCapability()) {
      sender_key_id_ = key.GetId();
      set_sender_value_label();
    }
  }

  connect(ui_->ccButton, &QPushButton::clicked, [=]() {
    ui_->ccInputWidget->setHidden(!ui_->ccInputWidget->isHidden());
    ui_->ccEdit->clear();
  });
  connect(ui_->bccButton, &QPushButton::clicked, [=]() {
    ui_->bccInputWidget->setHidden(!ui_->bccInputWidget->isHidden());
    ui_->bccEdit->clear();
  });

#ifdef SMTP_SUPPORT
  connect(ui_->sendMailButton, &QPushButton::clicked, this,
          &SendMailDialog::slot_confirm);
#endif

  connect(ui_->senderKeySelectButton, &QPushButton::clicked, this, [=]() {
    auto picker = new SenderPicker(sender_key_id_, this);
    sender_key_id_ = picker->GetCheckedSender();
    set_sender_value_label();
  });

  connect(ui_->recipientKeySelectButton, &QPushButton::clicked, this, [=]() {
    auto picker = new RecipientsPicker(recipients_key_ids_, this);
    recipients_key_ids_ = picker->GetCheckedRecipients();
    set_recipients_value_label();
  });

  connect(ui_->recipientsEditButton, &QPushButton::clicked, this, [=]() {
    auto editor = new EmailListEditor(ui_->recipientEdit->text(), this);
    ui_->recipientEdit->setText(editor->GetEmailList());
  });

  connect(ui_->ccEditButton, &QPushButton::clicked, this, [=]() {
    auto editor = new EmailListEditor(ui_->ccEdit->text(), this);
    ui_->ccEdit->setText(editor->GetEmailList());
  });

  connect(ui_->bccEditButton, &QPushButton::clicked, this, [=]() {
    auto editor = new EmailListEditor(ui_->bccEdit->text(), this);
    ui_->bccEdit->setText(editor->GetEmailList());
  });

  ui_->ccButton->setText(_("CC"));
  ui_->bccButton->setText(_("BCC"));
  ui_->senderLabel->setText(_("Sender"));
  ui_->recipientLabel->setText(_("Recipient"));
  ui_->subjectLabel->setText(_("Mail Subject"));
  ui_->bccLabel->setText(_("BCC"));
  ui_->ccLabel->setText(_("CC"));
  ui_->tipsLabel->setText(
      _("Tips: You can fill in multiple email addresses, please separate them "
        "with \";\"."));
  ui_->sendMailButton->setText(_("Send Message"));
  ui_->senderKeySelectButton->setText(_("Select Sender GPG Key"));
  ui_->recipientKeySelectButton->setText(_("Select Recipient(s) GPG Key"));
  ui_->gpgOperaLabel->setText(_("GPG Operations"));
  ui_->attacSignatureCheckBox->setText(_("Attach signature"));
  ui_->attachSenderPublickeyCheckBox->setText(_("Attach sender's public key"));
  ui_->contentEncryptCheckBox->setText(_("Encrypt content"));
  ui_->recipientsEditButton->setText(_("Edit Recipients(s)"));
  ui_->ccEditButton->setText(_("Edit CC(s)"));
  ui_->bccEditButton->setText(_("Edit BCC(s)"));
  ui_->senderKeyLabel->setText(_("Sender GPG Key: "));
  ui_->recipientKeysLabel->setText(_("Recipient(s) GPG Key: "));

  auto pos = QPoint(100, 100);
  LOG(INFO) << "parent" << parent;
  if (parent) pos += parent->pos();
  LOG(INFO) << "pos default" << pos.x() << pos.y();

  move(pos);

  this->setWindowTitle(_("New Message"));
  this->setAttribute(Qt::WA_DeleteOnClose);
}

bool SendMailDialog::check_email_address(const QString& str) {
  return re_email_.match(str).hasMatch();
}

#ifdef SMTP_SUPPORT

void SendMailDialog::slot_confirm() {
  QString errString;
  ui_->errorLabel->clear();
  ui_->errorLabel->setHidden(true);
  QStringList rcpt_string_list = ui_->recipientEdit->text().split(';');
  QStringList cc_string_list = ui_->ccEdit->text().split(';');
  QStringList bcc_string_list = ui_->bccEdit->text().split(';');

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
  if (ui_->senderEdit->text().isEmpty()) {
    errString.append(QString("  ") + _("Sender cannot be empty") + "  \n");
  } else if (!check_email_address(ui_->senderEdit->text())) {
    errString.append(QString("  ") + _("Sender's email is invalid") + "  \n");
  }

  if (ui_->subjectEdit->text().isEmpty()) {
    errString.append(QString("  ") + _("Subject cannot be empty") + "  \n");
  }

  if (!ui_->ccEdit->text().isEmpty())
    for (const auto& cc : cc_string_list) {
      LOG(INFO) << "cc" << cc.trimmed().toStdString();
      if (!check_email_address(cc.trimmed())) {
        errString.append(QString("  ") + _("One or more cc email is invalid") +
                         "  \n");
        break;
      }
    }

  if (!ui_->bccEdit->text().isEmpty())
    for (const auto& bcc : bcc_string_list) {
      LOG(INFO) << "bcc" << bcc.trimmed().toStdString();
      if (!check_email_address(bcc.trimmed())) {
        errString.append(QString("  ") + _("One or more bcc email is invalid") +
                         "  \n");
        break;
      }
    }

  if (!errString.isEmpty()) {
    ui_->errorLabel->setAutoFillBackground(true);
    QPalette error = ui_->errorLabel->palette();
    error.setColor(QPalette::Window, "#ff8080");
    ui_->errorLabel->setPalette(error);
    ui_->errorLabel->setText(errString);
    ui_->errorLabel->setHidden(false);
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
  auto sender_address = ui_->senderEdit->text().toStdString();

  auto thread = new SMTPSendMailThread(
      host, port, connection_type, identity_needed, username, password, this);

  thread->SetSender(ui_->senderEdit->text());
  thread->SetRecipient(ui_->recipientEdit->text());
  thread->SetCC(ui_->ccEdit->text());
  thread->SetBCC(ui_->bccEdit->text());
  thread->SetSubject(ui_->subjectEdit->text());
  thread->AddTextContent(ui_->textEdit->toPlainText());

  if (ui_->contentEncryptCheckBox->checkState() == Qt::Checked) {
    if (recipients_key_ids_ == nullptr || recipients_key_ids_->empty()) {
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
      thread->SetEncryptContent(true, std::move(key_ids));
    }
  }

  if (ui_->attacSignatureCheckBox->checkState() == Qt::Checked) {
    if (sender_key_id_.empty()) {
      QMessageBox::critical(
          this, _("Forbidden"),
          _("You checked the option to attach signature to the email, but did "
            "not specify the sender's GPG Key. This will cause the content of "
            "the email to be inconsistent with your expectations, so the "
            "operation is prohibited."));
      return;
    } else {
      thread->SetAttachSignatureFile(true, sender_key_id_);
    }
  }

  if (ui_->attachSenderPublickeyCheckBox->checkState() == Qt::Checked) {
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
      thread->SetAttachPublicKey(true, sender_key_id_);
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
  connect(thread, &SMTPSendMailThread::SignalSMTPResult, this,
          &SendMailDialog::slot_test_smtp_connection_result);
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

void SendMailDialog::init_settings() {
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
    default_sender_gpg_key_id_ =
        settings.lookup("smtp.default_sender_gpg_key_id").c_str();
  } catch (...) {
    LOG(ERROR) << _("Setting Operation Error")
               << _("default_sender_gpg_key_id");
  }
}
#endif

void SendMailDialog::set_sender_value_label() {
  auto key = GpgKeyGetter::GetInstance().GetKey(sender_key_id_);
  if (key.IsGood()) {
    ui_->senderKeyValueLabel->setText(key.GetUIDs()->front().GetUID().c_str());
  }
}

void SendMailDialog::set_recipients_value_label() {
  auto keys = GpgKeyGetter::GetInstance().GetKeys(recipients_key_ids_);
  std::stringstream ss;
  for (const auto& key : *keys) {
    if (key.IsGood()) {
      ss << key.GetUIDs()->front().GetUID().c_str() << ";";
    }
  }
  ui_->recipientsKeyValueLabel->setText(ss.str().c_str());
}

void SendMailDialog::slot_test_smtp_connection_result(const QString& result) {
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
  } else if (result == "Fail to encrypt with gpg keys") {
    QMessageBox::critical(
        this, _("Encryption Error"),
        _("An error occurred while encrypting the content of the email. This "
          "may be because you did not complete some operations during Gnupg "
          "encryption."));
  } else {
    QMessageBox::critical(this, _("Fail"), _("Unknown error."));
  }
}
void SendMailDialog::SetContentEncryption(bool on) {
  ui_->contentEncryptCheckBox->setCheckState(on ? Qt::Checked : Qt::Unchecked);
}

void SendMailDialog::SetAttachSignature(bool on) {
  ui_->attacSignatureCheckBox->setCheckState(on ? Qt::Checked : Qt::Unchecked);
}

}  // namespace GpgFrontend::UI
