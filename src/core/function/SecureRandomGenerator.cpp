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

#include <sodium.h>

#include <cstdint>
#include <cstring>
#include <string_view>

namespace {

constexpr std::string_view kZBase32Alphabet =
    "ybndrfg8ejkmcpqxot1uwisza345h769";

static_assert(kZBase32Alphabet.size() == 32);

auto EnsureSodiumInit() -> bool {
  static const int kRc = sodium_init();
  if (kRc < 0) {
    LOG_E() << "sodium_init failed";
    return false;
  }
  return true;
}

auto ZBase32Encode(const GpgFrontend::GFBuffer& data) -> GpgFrontend::GFBuffer {
  GpgFrontend::GFBuffer result;

  std::uint32_t buffer = 0;
  int bits_left = 0;

  for (size_t i = 0; i < data.Size(); ++i) {
    buffer = (buffer << 8) | static_cast<std::uint8_t>(
                                 static_cast<unsigned char>(data.Data()[i]));
    bits_left += 8;

    while (bits_left >= 5) {
      const auto index =
          static_cast<size_t>((buffer >> (bits_left - 5)) & 0x1F);
      result.Append(kZBase32Alphabet.data() + index, 1);
      bits_left -= 5;
    }

    if (bits_left > 0) {
      buffer &= (1U << bits_left) - 1U;
    } else {
      buffer = 0;
    }
  }

  if (bits_left > 0) {
    const auto index = static_cast<size_t>((buffer << (5 - bits_left)) & 0x1F);
    result.Append(kZBase32Alphabet.data() + index, 1);
  }

  return result;
}

}  // namespace

namespace GpgFrontend {

SecureRandomGenerator::SecureRandomGenerator(int channel)
    : SingletonFunctionObject<SecureRandomGenerator>(channel) {}

auto SecureRandomGenerator::Generate(size_t size) -> GFBufferOrNone {
  if (!EnsureSodiumInit()) return {};

  GFBuffer buffer(size);

  if (size > 0) {
    randombytes_buf(buffer.Data(), buffer.Size());
  }

  return buffer;
}

auto SecureRandomGenerator::GenerateZBase32() -> GFBufferOrNone {
  auto buffer = Generate(20);
  if (!buffer) return {};

  auto z_buffer = ZBase32Encode(*buffer);

  if (z_buffer.Size() > 31) {
    z_buffer.Resize(31);
  }

  return z_buffer;
}

}  // namespace GpgFrontend