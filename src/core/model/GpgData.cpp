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

#include "core/model/GpgData.h"

GpgFrontend::GpgData::GpgData() {
  gpgme_data_t data;

  auto err = gpgme_data_new(&data);
  assert(gpgme_err_code(err) == GPG_ERR_NO_ERROR);

  data_ref_ = std::unique_ptr<struct gpgme_data, _data_ref_deleter>(data);
}

GpgFrontend::GpgData::GpgData(void* buffer, size_t size, bool copy) {
  gpgme_data_t data;

  auto err = gpgme_data_new_from_mem(&data, static_cast<const char*>(buffer),
                                     size, copy);
  assert(gpgme_err_code(err) == GPG_ERR_NO_ERROR);

  data_ref_ = std::unique_ptr<struct gpgme_data, _data_ref_deleter>(data);
}

/**
 * Read gpgme-Data to QByteArray
 *   mainly from http://basket.kde.org/ (kgpgme.cpp)
 */
#define BUF_SIZE (32 * 1024)

GpgFrontend::ByteArrayPtr GpgFrontend::GpgData::Read2Buffer() {
  gpgme_off_t ret = gpgme_data_seek(*this, 0, SEEK_SET);
  ByteArrayPtr out_buffer = std::make_unique<std::string>();

  if (ret) {
    gpgme_error_t err = gpgme_err_code_from_errno(errno);
    assert(gpgme_err_code(err) == GPG_ERR_NO_ERROR);
  } else {
    char buf[BUF_SIZE + 2];

    while ((ret = gpgme_data_read(*this, buf, BUF_SIZE)) > 0) {
      const size_t size = out_buffer->size();
      out_buffer->resize(static_cast<int>(size + ret));
      memcpy(out_buffer->data() + size, buf, ret);
    }
    if (ret < 0) {
      gpgme_error_t err = gpgme_err_code_from_errno(errno);
      assert(gpgme_err_code(err) == GPG_ERR_NO_ERROR);
    }
  }
  return out_buffer;
}

GpgFrontend::GpgData::operator gpgme_data_t() { return data_ref_.get(); }