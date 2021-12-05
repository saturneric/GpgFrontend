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

#ifndef _GPGDATA_H
#define _GPGDATA_H

#include "gpg/GpgConstants.h"

namespace GpgFrontend {

class GpgData {
 public:
  GpgData();

  GpgData(void* buffer, size_t size, bool copy = true);

  operator gpgme_data_t() { return data_.get(); }

  ByteArrayPtr Read2Buffer();

 private:
  struct __data_ref_deletor {
    void operator()(gpgme_data_t _data) {
      if (_data != nullptr) gpgme_data_release(_data);
    }
  };

  std::unique_ptr<struct gpgme_data, __data_ref_deletor> data_ = nullptr;
};

}  // namespace GpgFrontend

#endif  // _GPGDATA_H