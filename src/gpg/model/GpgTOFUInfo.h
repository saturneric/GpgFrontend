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

#ifndef GPGFRONTEND_GPGTOFU_H
#define GPGFRONTEND_GPGTOFU_H

#include "gpg/GpgConstants.h"

namespace GpgFrontend {

class GpgTOFUInfo {
 public:
  [[nodiscard]] unsigned validity() const { return _tofu_info_ref->validity; }

  [[nodiscard]] unsigned policy() const { return _tofu_info_ref->policy; }

  [[nodiscard]] unsigned long sign_count() const {
    return _tofu_info_ref->signcount;
  }

  [[nodiscard]] unsigned long encr_count() const {
    return _tofu_info_ref->encrcount;
  }

  [[nodiscard]] unsigned long sign_first() const {
    return _tofu_info_ref->signfirst;
  }

  [[nodiscard]] unsigned long sign_last() const {
    return _tofu_info_ref->signlast;
  }

  [[nodiscard]] unsigned long encr_last() const {
    return _tofu_info_ref->encrlast;
  }

  [[nodiscard]] std::string description() const {
    return _tofu_info_ref->description;
  }

  GpgTOFUInfo() = default;

  explicit GpgTOFUInfo(gpgme_tofu_info_t tofu_info);

  GpgTOFUInfo(GpgTOFUInfo&& o) noexcept {
    swap(_tofu_info_ref, o._tofu_info_ref);
  }

  GpgTOFUInfo(const GpgTOFUInfo&) = delete;

  GpgTOFUInfo& operator=(GpgTOFUInfo&& o) noexcept {
    swap(_tofu_info_ref, o._tofu_info_ref);
    return *this;
  };

  GpgTOFUInfo& operator=(const GpgTOFUInfo&) = delete;

 private:
  using SubkeyRefHandler =
      std::unique_ptr<struct _gpgme_tofu_info,
                      std::function<void(gpgme_tofu_info_t)>>;

  SubkeyRefHandler _tofu_info_ref = nullptr;
};

}  // namespace GpgFrontend

#endif  // GPGFRONTEND_GPGTOFU_H
