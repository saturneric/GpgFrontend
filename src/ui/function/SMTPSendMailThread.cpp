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
 * by Saturneric<eric@bktus.com><eric@bktus.com> starting on May 12, 2021.
 *
 */

#include "SMTPSendMailThread.h"

void SMTPSendMailThread::run() {
  SmtpClient smtp(host_.c_str(), port_, connection_type_);

  if (identify_) {
    smtp.setUser(username_.c_str());
    smtp.setPassword(password_.c_str());
  }

  // Now we can send the mail
  if (!smtp.connectToHost()) {
    emit signalSMTPResult("Fail to connect SMTP server");
    return;
  }
  if (!smtp.login()) {
    emit signalSMTPResult("Fail to login");
    return;
  }
  if (!smtp.sendMail(message)) {
    emit signalSMTPResult("Fail to send mail");
    return;
  }
  smtp.quit();
  emit signalSMTPResult("Succeed in sending a test email");
}

void SMTPSendMailThread::setBCC(const QString& bccs) {
  QStringList bcc_string_list = bccs.split(';');
  for (const auto& bcc : bcc_string_list) {
    if (!bcc.isEmpty()) message.addBcc(new EmailAddress(bcc.trimmed()));
  }
}

void SMTPSendMailThread::setCC(const QString& ccs) {
  QStringList cc_string_list = ccs.split(';');
  for (const auto& cc : cc_string_list) {
    if (!cc.isEmpty()) message.addCc(new EmailAddress(cc.trimmed()));
  }
}

void SMTPSendMailThread::setRecipient(const QString& recipients) {
  QStringList rcpt_string_list = recipients.split(';');
  for (const auto& rcpt : rcpt_string_list) {
    if (!rcpt.isEmpty()) message.addRecipient(new EmailAddress(rcpt.trimmed()));
  }
}

void SMTPSendMailThread::setSender(const QString& sender) {
  message.setSender(new EmailAddress(sender));
}

void SMTPSendMailThread::addTextContent(const QString& content) {
  auto text = std::make_unique<MimeText>(content);
  texts_.push_back(std::move(text));
  message.addPart(texts_.back().get());
}

void SMTPSendMailThread::addFileContent(const QString& file_name,
                                        const QByteArray& content) {
  auto file = std::make_unique<MimeFile>(content, file_name);
  files_.push_back(std::move(file));
  message.addPart(files_.back().get());
}
