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

#include "GpgData.h"

#include <cstddef>

#include "core/model/GFDataExchanger.h"
#include "core/typedef/GpgErrorTypedef.h"
#include "plat/common/PlatformFs.h"

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
  Adopt(err, data);
}

GpgData::GpgData(const GFBuffer& buffer) : cached_buffer_(buffer) {
  gpgme_data_t data;

  auto err = gpgme_data_new_from_mem(
      &data, reinterpret_cast<const char*>(cached_buffer_.Data()),
      cached_buffer_.Size(), 0);
  Adopt(err, data);
}

GpgData::GpgData(const void* buffer, size_t size, bool copy) {
  gpgme_data_t data;

  auto err = gpgme_data_new_from_mem(&data, static_cast<const char*>(buffer),
                                     size, static_cast<int>(copy));
  Adopt(err, data);
}

GpgData::GpgData(int fd) : fd_(fd), data_cbs_() {
  gpgme_data_t data;

  auto err = gpgme_data_new_from_fd(&data, fd_);
  Adopt(err, data);
}

GpgData::GpgData(const QString& path, bool read) {
  gpgme_data_t data;

  // support unicode path
  QFile file(path);
  if (!file.open(read ? QIODevice::ReadOnly : QIODevice::WriteOnly)) {
    LOG_W() << "cannot open" << path << ":" << file.errorString();
    error_ = file.error() == QFileDevice::PermissionsError ? GPG_ERR_EACCES
             : read && !file.exists()                      ? GPG_ERR_ENOENT
                                                           : GPG_ERR_EIO;
    return;
  }
  const auto fd = Platform::DuplicateFd(file.handle());
  fp_ = fd < 0 ? nullptr : fdopen(fd, read ? "rb" : "wb");
  if (fp_ == nullptr) {
    if (fd >= 0) Platform::CloseFd(fd);
    LOG_W() << "cannot open a stream on" << path;
    error_ = GPG_ERR_EIO;
    return;
  }

  auto err = gpgme_data_new_from_stream(&data, fp_);
  Adopt(err, data);
}

GpgData::GpgData(QSharedPointer<GFDataExchanger> ex)
    : data_cbs_(), data_ex_(std::move(ex)) {
  gpgme_data_t data;

  data_cbs_.read = GFReadExCb;
  data_cbs_.write = GFWriteExCb;
  data_cbs_.seek = nullptr;
  data_cbs_.release = GFReleaseExCb;

  auto err = gpgme_data_new_from_cbs(&data, &data_cbs_, data_ex_.get());
  Adopt(err, data);
}

GpgData::~GpgData() {
  if (fp_ != nullptr) {
    fclose(fp_);
  }

  if (fd_ >= 0) {
    Platform::CloseFd(fd_);
  }
}

auto GpgData::Read2GFBuffer() -> GFBuffer {
  if (data_ref_ == nullptr) return {};

  gpgme_off_t ret = gpgme_data_seek(*this, 0, SEEK_SET);
  if (ret != 0) {
    LOG_E() << "cannot rewind gpgme data, errno:" << errno;
    return {};
  }

  GFBuffer buffer;
  std::vector<char, SMASecAllocator<char>> buf(kSecBufferSize +
                                               (sizeof(std::byte) * 8));
  while ((ret = gpgme_data_read(*this, buf.data(), kSecBufferSize)) > 0) {
    buffer.Append(buf.data(), ret);
  }

  // Part of the output is not the output: handing it on as if it were, which
  // release builds did, presents a truncated plaintext as the whole of it.
  if (ret < 0) {
    LOG_E() << "cannot read gpgme data back, errno:" << errno;
    return {};
  }
  return buffer;
}

auto GpgData::Error() const -> GpgError { return error_; }

void GpgData::Adopt(GpgError err, gpgme_data_t data) {
  if (gpgme_err_code(err) != GPG_ERR_NO_ERROR) {
    LOG_W() << "cannot create gpgme data:" << gpgme_strerror(err);
    error_ = err;
    return;
  }
  data_ref_ = std::unique_ptr<struct gpgme_data, DataRefDeleter>(data);
}

GpgData::operator gpgme_data_t() { return data_ref_.get(); }
}  // namespace GpgFrontend