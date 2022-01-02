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

#ifndef GPGFRONTEND_SMTPTESTTHREAD_H
#define GPGFRONTEND_SMTPTESTTHREAD_H

#include <utility>

#ifdef SMTP_SUPPORT
#include "smtp/SmtpMime"
#endif

#include "ui/GpgFrontendUI.h"

class SMTPTestThread : public QThread {
  Q_OBJECT
 public:
  explicit SMTPTestThread(std::string host, int port,
                          SmtpClient::ConnectionType connection_type,
                          bool identify, std::string username,
                          std::string password, QWidget* parent = nullptr)
      : QThread(parent),
        host_(std::move(host)),
        port_(port),
        connection_type_(connection_type),
        identify_(identify),
        username_(std::move(username)),
        password_(std::move(password)) {

  }

 signals:
  void signalSMTPTestResult(const QString& result);

 protected:
  void run() override;

 private:
  std::string host_;
  int port_;
  SmtpClient::ConnectionType connection_type_;

  bool identify_;
  std::string username_;
  std::string password_;
};

#endif  // GPGFRONTEND_SMTPTESTTHREAD_H
