/**
 * This file is part of GPGFrontend.
 *
 * GPGFrontend is free software: you can redistribute it and/or modify
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

#include "gpg/model/GpgData.h"

GpgFrontend::GpgData::GpgData() {
  gpgme_data_t data;

  gpgme_error_t err = gpgme_data_new(&data);
  assert(gpgme_err_code(err) == GPG_ERR_NO_ERROR);

  data_ =
      std::unique_ptr<struct gpgme_data, __data_ref_deletor>(std::move(data));
}

GpgFrontend::GpgData::GpgData(void *buffer, size_t size, bool copy) {
  gpgme_data_t data;

  gpgme_error_t err =
      gpgme_data_new_from_mem(&data, (const char *)buffer, size, copy);
  assert(gpgme_err_code(err) == GPG_ERR_NO_ERROR);

  data_ =
      std::unique_ptr<struct gpgme_data, __data_ref_deletor>(std::move(data));
}

/**
 * Read gpgme-Data to QByteArray
 *   mainly from http://basket.kde.org/ (kgpgme.cpp)
 */
#define BUF_SIZE (32 * 1024)

GpgFrontend::BypeArrayPtr GpgFrontend::GpgData::Read2Buffer() {
  gpgme_off_t ret = gpgme_data_seek(*this, 0, SEEK_SET);
  gpgme_error_t err = gpg_error(GPG_ERR_NO_ERROR);

  BypeArrayPtr out_buffer = std::make_unique<std::string>();

  if (ret) {
    err = gpgme_err_code_from_errno(errno);
    assert(gpgme_err_code(err) == GPG_ERR_NO_ERROR);
  } else {
    char buf[BUF_SIZE + 2];

    while ((ret = gpgme_data_read(*this, buf, BUF_SIZE)) > 0) {
      const size_t size = out_buffer->size();
      out_buffer->resize(static_cast<int>(size + ret));
      memcpy(out_buffer->data() + size, buf, ret);
    }
    if (ret < 0) {
      err = gpgme_err_code_from_errno(errno);
      assert(gpgme_err_code(err) == GPG_ERR_NO_ERROR);
    }
  }
  return out_buffer;
}