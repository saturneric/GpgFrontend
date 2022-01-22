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

#ifndef _GPGDATA_H
#define _GPGDATA_H

#include "gpg/GpgConstants.h"

namespace GpgFrontend {
/**
 * @brief
 *
 */
class GpgData {
 public:
  /**
   * @brief Construct a new Gpg Data object
   *
   */
  GpgData();

  /**
   * @brief Construct a new Gpg Data object
   *
   * @param buffer
   * @param size
   * @param copy
   */
  GpgData(void* buffer, size_t size, bool copy = true);

  /**
   * @brief
   *
   * @return gpgme_data_t
   */
  operator gpgme_data_t() { return data_ref_.get(); }

  /**
   * @brief
   *
   * @return ByteArrayPtr
   */
  ByteArrayPtr Read2Buffer();

 private:
  /**
   * @brief
   *
   */
  struct _data_ref_deleter {
    void operator()(gpgme_data_t _data) {
      if (_data != nullptr) gpgme_data_release(_data);
    }
  };

  std::unique_ptr<struct gpgme_data, _data_ref_deleter> data_ref_ =
      nullptr;  ///<
};

}  // namespace GpgFrontend

#endif  // _GPGDATA_H