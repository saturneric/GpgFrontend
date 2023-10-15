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

#ifndef GPGFRONTEND_PLUGIN_SOFTWAREVERSION_H
#define GPGFRONTEND_PLUGIN_SOFTWAREVERSION_H

#include <GpgFrontendPluginSDK.h>

#include <boost/date_time.hpp>

namespace GpgFrontend::Plugin::IntegradedPlugin::VersionCheckingPlugin {
/**
 * @brief
 *
 */
struct SoftwareVersion {
  std::string latest_version;          ///<
  std::string current_version;         ///<
  bool latest_prerelease = false;      ///<
  bool latest_draft = false;           ///<
  bool current_prerelease = false;     ///<
  bool current_draft = false;          ///<
  bool load_info_done = false;         ///<
  bool current_version_found = false;  ///<
  std::string publish_date;            ///<
  std::string release_note;            ///<

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] bool InfoValid() const { return load_info_done; }

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] bool NeedUpgrade() const;

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] bool VersionWithDrawn() const;

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] bool CurrentVersionReleased() const;

 private:
  static int version_compare(const std::string& a, const std::string& b);
};
}  // namespace GpgFrontend::Plugin::IntegradedPlugin::VersionCheckingPlugin

#endif  // GPGFRONTEND_PLUGIN_SOFTWAREVERSION_H
