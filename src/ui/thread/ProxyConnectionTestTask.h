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

#ifndef GPGFRONTEND_PROXYCONNECTIONTESTTHREAD_H
#define GPGFRONTEND_PROXYCONNECTIONTESTTHREAD_H

class ProxyConnectionTestThread {};

#include <utility>

#include "GpgFrontendUI.h"

namespace GpgFrontend::UI {

/**
 * @brief
 *
 */
class ProxyConnectionTestTask : public Thread::Task {
  Q_OBJECT
 public:
  /**
   * @brief Construct a new Proxy Connection Test Thread object
   *
   * @param url
   * @param timeout
   * @param parent
   */
  explicit ProxyConnectionTestTask(QString url, int timeout);

 signals:
  /**
   * @brief
   *
   * @param result
   */
  void SignalProxyConnectionTestResult(const QString& result);

 protected:
  /**
   * @brief
   *
   */
  void run() override;

 private slots:
  void slot_process_network_reply(QNetworkReply* reply);

 private:
  QString url_;                             ///<
  QString result_;                          ///<
  int timeout_ = 500;                       ///<
  QNetworkAccessManager* network_manager_;  ///<
};

}  // namespace GpgFrontend::UI

#endif  // GPGFRONTEND_PROXYCONNECTIONTESTTHREAD_H
