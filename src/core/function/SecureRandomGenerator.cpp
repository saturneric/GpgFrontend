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

#include "SecureRandomGenerator.h"

#include <openssl/err.h>
#include <openssl/provider.h>
#include <openssl/rand.h>

namespace GpgFrontend {

SecureRandomGenerator::SecureRandomGenerator(int channel)
    : SingletonFunctionObject<SecureRandomGenerator>(channel) {}

auto SecureRandomGenerator::GnuPGGenerate(size_t size) -> GFBufferOrNone {
  size = std::max(size, static_cast<size_t>(8));
  size = std::min(size, static_cast<size_t>(1024));

  GFBuffer buffer(size);

  GpgError err =
      gpgme_op_random_bytes(ctx_.DefaultContext(), GPGME_RANDOM_MODE_NORMAL,
                            buffer.Data(), buffer.Size());
  if (err != GPG_ERR_NO_ERROR) return {};
  return buffer;
}

auto SecureRandomGenerator::GnuPGGenerateZBase32() -> GFBufferOrNone {
  GFBuffer buffer(31);

  GpgError err =
      gpgme_op_random_bytes(ctx_.DefaultContext(), GPGME_RANDOM_MODE_ZBASE32,
                            buffer.Data(), buffer.Size());
  if (err != GPG_ERR_NO_ERROR) return {};
  return buffer;
}

auto SecureRandomGenerator::OpenSSLGenerate(size_t size) -> GFBufferOrNone {
  GFBuffer buffer(size);

  auto *data = reinterpret_cast<unsigned char *>(buffer.Data());
  int bytes = static_cast<int>(size);

  if (RAND_priv_bytes(data, bytes) != 1) {
    auto err = ERR_get_error();
    std::array<char, 256> err_buf = {0};
    ERR_error_string_n(err, err_buf.data(), sizeof(err_buf));
    LOG_D() << "RAND_priv_bytes failed: " << err_buf.data();
    return {};
  }
  return buffer;
}
};  // namespace GpgFrontend