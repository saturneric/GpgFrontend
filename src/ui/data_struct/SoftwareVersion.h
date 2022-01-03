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

#ifndef GPGFRONTEND_SOFTWAREVERSION_H
#define GPGFRONTEND_SOFTWAREVERSION_H

#include <boost/date_time.hpp>

namespace GpgFrontend::UI {
struct SoftwareVersion {
  std::string latest_version;
  std::string current_version;
  bool latest_prerelease = false;
  bool latest_draft = false;
  bool current_prerelease = false;
  bool current_draft = false;
  bool load_info_done = false;
  std::string publish_date;
  std::string release_note;

  [[nodiscard]] bool NeedUpgrade() const {
    return load_info_done && !latest_prerelease && !latest_draft &&
           current_version < latest_version;
  }

  [[nodiscard]] bool VersionWithDrawn() const {
    return load_info_done && current_prerelease && !current_draft;
  }
};
}  // namespace GpgFrontend::UI

#endif  // GPGFRONTEND_SOFTWAREVERSION_H
