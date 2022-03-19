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
 * The source code version of this software was modified and released
 * by Saturneric<eric@bktus.com><eric@bktus.com> starting on May 12, 2021.
 *
 */

#ifndef GPGFRONTEND_SMTPCONNECTIONTESTTHREAD_H
#define GPGFRONTEND_SMTPCONNECTIONTESTTHREAD_H

#include <utility>

#include "ui/GpgFrontendUI.h"

namespace GpgFrontend::UI {

/**
 * @brief
 *
 */
class SMTPConnectionTestThread : public QThread {
  Q_OBJECT
 public:
  /**
   * @brief Construct a new SMTPConnectionTestThread object
   *
   * @param host
   * @param port
   * @param connection_type
   * @param identify
   * @param username
   * @param password
   * @param parent
   */
  explicit SMTPConnectionTestThread(std::string host, int port,
                                    SmtpClient::ConnectionType connection_type,
                                    bool identify, std::string username,
                                    std::string password,
                                    QWidget* parent = nullptr)
      : QThread(parent),
        host_(std::move(host)),
        port_(port),
        connection_type_(connection_type),
        identify_(identify),
        username_(std::move(username)),
        password_(std::move(password)) {}

 signals:
  /**
   * @brief
   *
   * @param result
   */
  void SignalSMTPConnectionTestResult(const QString& result);

 protected:
  /**
   * @brief
   *
   */
  void run() override;

 private:
  std::string host_;                            ///<
  int port_;                                    ///<
  SmtpClient::ConnectionType connection_type_;  ///<
  bool identify_;                               ///<
  std::string username_;                        ///<
  std::string password_;                        ///<
};

}  // namespace GpgFrontend::UI

#endif  // GPGFRONTEND_SMTPCONNECTIONTESTTHREAD_H
