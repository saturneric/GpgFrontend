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

#pragma once

#include "core/GpgFrontendCoreExport.h"
#include "core/model/GFBuffer.h"
#include "core/typedef/CoreTypedef.h"

namespace GpgFrontend {
/**
 * @brief
 *
 */
class GPGFRONTEND_CORE_EXPORT GpgData {
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
  GpgData(const void* buffer, size_t size, bool copy = true);

  /**
   * @brief Construct a new Gpg Data object
   *
   * @param fd
   */
  explicit GpgData(int fd);

  /**
   * @brief Construct a new Gpg Data object
   *
   * @param path
   */
  explicit GpgData(const std::filesystem::path& path, bool read);

  /**
   * @brief Construct a new Gpg Data object
   *
   */
  explicit GpgData(GFBuffer);

  /**
   * @brief Destroy the Gpg Data object
   *
   */
  ~GpgData();

  /**
   * @brief
   *
   * @return gpgme_data_t
   */
  operator gpgme_data_t();

  /**
   * @brief
   *
   * @return ByteArrayPtr
   */
  auto Read2Buffer() -> ByteArrayPtr;

  /**
   * @brief
   *
   * @return ByteArrayPtr
   */
  auto Read2GFBuffer() -> GFBuffer;

 private:
  /**
   * @brief
   *
   */
  struct DataRefDeleter {
    void operator()(gpgme_data_t _data) {
      if (_data != nullptr) gpgme_data_release(_data);
    }
  };

  GFBuffer cached_buffer_;

  std::unique_ptr<struct gpgme_data, DataRefDeleter> data_ref_ = nullptr;  ///<
  FILE* fp_ = nullptr;
  int fd_ = -1;
};

}  // namespace GpgFrontend
