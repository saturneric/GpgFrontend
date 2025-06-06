/**
 * Copyright (C) 2021-2024 Saturneric <eric@bktus.com>
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

#include "core/model/GpgData.h"

#include <unistd.h>

#include <cstddef>

#include "core/model/GFDataExchanger.h"
#include "core/typedef/GpgErrorTypedef.h"

namespace {
auto GFReadExCb(void* handle, void* buffer, size_t size) -> ssize_t {
  auto* ex = static_cast<GpgFrontend::GFDataExchanger*>(handle);
  return ex->Read(static_cast<std::byte*>(buffer), size);
}

auto GFWriteExCb(void* handle, const void* buffer, size_t size) -> ssize_t {
  auto* ex = static_cast<GpgFrontend::GFDataExchanger*>(handle);
  return ex->Write(static_cast<const std::byte*>(buffer), size);
}

void GFReleaseExCb(void* handle) {
  auto* ex = static_cast<GpgFrontend::GFDataExchanger*>(handle);
  ex->CloseWrite();
}
}  // namespace

namespace GpgFrontend {

GpgData::GpgData() {
  gpgme_data_t data;

  auto err = gpgme_data_new(&data);
  assert(gpgme_err_code(err) == GPG_ERR_NO_ERROR);

  data_ref_ = std::unique_ptr<struct gpgme_data, DataRefDeleter>(data);
}

GpgData::GpgData(const GFBuffer& buffer) : cached_buffer_(buffer) {
  gpgme_data_t data;

  auto err = gpgme_data_new_from_mem(
      &data, reinterpret_cast<const char*>(cached_buffer_.Data()),
      cached_buffer_.Size(), 0);
  assert(gpgme_err_code(err) == GPG_ERR_NO_ERROR);

  data_ref_ = std::unique_ptr<struct gpgme_data, DataRefDeleter>(data);
}

GpgData::GpgData(const void* buffer, size_t size, bool copy) {
  gpgme_data_t data;

  auto err = gpgme_data_new_from_mem(&data, static_cast<const char*>(buffer),
                                     size, static_cast<int>(copy));
  assert(gpgme_err_code(err) == GPG_ERR_NO_ERROR);

  data_ref_ = std::unique_ptr<struct gpgme_data, DataRefDeleter>(data);
}

GpgData::GpgData(int fd) : fd_(fd), data_cbs_() {
  gpgme_data_t data;

  auto err = gpgme_data_new_from_fd(&data, fd_);
  assert(gpgme_err_code(err) == GPG_ERR_NO_ERROR);

  data_ref_ = std::unique_ptr<struct gpgme_data, DataRefDeleter>(data);
}

GpgData::GpgData(const QString& path, bool read) {
  gpgme_data_t data;

  // support unicode path
  QFile file(path);
  file.open(read ? QIODevice::ReadOnly : QIODevice::WriteOnly);
  fp_ = fdopen(dup(file.handle()), read ? "rb" : "wb");

  auto err = gpgme_data_new_from_stream(&data, fp_);
  assert(gpgme_err_code(err) == GPG_ERR_NO_ERROR);

  data_ref_ = std::unique_ptr<struct gpgme_data, DataRefDeleter>(data);
}

GpgData::GpgData(QSharedPointer<GFDataExchanger> ex)
    : data_cbs_(), data_ex_(std::move(ex)) {
  gpgme_data_t data;

  data_cbs_.read = GFReadExCb;
  data_cbs_.write = GFWriteExCb;
  data_cbs_.seek = nullptr;
  data_cbs_.release = GFReleaseExCb;

  auto err = gpgme_data_new_from_cbs(&data, &data_cbs_, data_ex_.get());
  assert(gpgme_err_code(err) == GPG_ERR_NO_ERROR);

  data_ref_ = std::unique_ptr<struct gpgme_data, DataRefDeleter>(data);
}

GpgData::~GpgData() {
  if (fp_ != nullptr) {
    fclose(fp_);
  }

  if (fd_ >= 0) {
    close(fd_);
  }
}

auto GpgData::Read2GFBuffer() -> GFBuffer {
  gpgme_off_t ret = gpgme_data_seek(*this, 0, SEEK_SET);
  GFBuffer buffer;

  if (ret != 0) {
    const GpgError err = gpgme_err_code_from_errno(errno);
    assert(gpgme_err_code(err) == GPG_ERR_NO_ERROR);
  } else {
    GFBuffer buf(kSecBufferSizeForFile + 8);

    while ((ret = gpgme_data_read(*this, buf.Data(), kSecBufferSizeForFile)) >
           0) {
      buffer.Append(buf);
    }

    if (ret < 0) {
      const GpgError err = gpgme_err_code_from_errno(errno);
      assert(gpgme_err_code(err) == GPG_ERR_NO_ERROR);
    }
  }
  return buffer;
}

GpgData::operator gpgme_data_t() { return data_ref_.get(); }
}  // namespace GpgFrontend