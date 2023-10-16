/**
 * Copyright (C) 2021 Saturneric <eric@bktus.com>
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
 * Saturneric <eric@bktus.com> starting on May 12, 2021.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#ifndef GPGFRONTEND_VERSIONCHECKTHREAD_H
#define GPGFRONTEND_VERSIONCHECKTHREAD_H

#include <qnetworkreply.h>

#include <memory>
#include <string>

#include "core/thread/Task.h"
#include "ui/GpgFrontendUI.h"
#include "ui/struct/SoftwareVersion.h"

class QNetworkAccessManager;
class QNetworkReply;

namespace GpgFrontend::UI {

/**
 * @brief
 *
 */
class VersionCheckTask : public Thread::Task {
  Q_OBJECT

 public:
  /**
   * @brief Construct a new Version Check Thread object
   *
   */
  explicit VersionCheckTask();

 signals:

  /**
   * @brief
   *
   * @param version
   */
  void SignalUpgradeVersion(SoftwareVersion version);

 protected:
  /**
   * @brief

   *
   */
  void Run() override;

 private slots:

  /**
   * @brief
   *
   */
  void slot_parse_latest_version_info();

  /**
   * @brief
   *
   */
  void slot_parse_current_version_info();

 private:
  QByteArray latest_reply_bytes_;           ///<
  QByteArray current_reply_bytes_;          ///<
  QNetworkReply* latest_reply_ = nullptr;   ///< latest version info reply
  QNetworkReply* current_reply_ = nullptr;  ///< current version info reply
  QNetworkAccessManager* network_manager_;  ///<
  std::string current_version_;
  SoftwareVersion version_;
};

}  // namespace GpgFrontend::UI

#endif  // GPGFRONTEND_VERSIONCHECKTHREAD_H
