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

#include "PassphraseGenerator.h"

namespace GpgFrontend {

namespace {

constexpr std::array<char, 63> kAlphanum = {
    "0123456789"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"};

// the array carries the literal's trailing NUL; the alphabet itself is 62.
constexpr size_t kAlphanumSize = kAlphanum.size() - 1;

// 62 * 4 == 248. Bytes from 248..255 would map onto '0'..'7' a fifth time and
// skew the alphabet, so they are rejected instead of folded.
constexpr unsigned char kRejectThreshold = 248;

static_assert(kAlphanumSize == 62, "alphabet size drives the reject threshold");
static_assert(kRejectThreshold % kAlphanumSize == 0,
              "reject threshold must be a whole multiple of the alphabet");

/**
 * @brief Draw @p len random bytes, falling back to Qt on any shortfall.
 */
auto DrawRandomBytes(int len) -> GFBufferOrNone {
  auto buffer = SecureRandomGenerator::Generate(len);

  // a short buffer is as unusable as a missing one: consuming it would read
  // past its end. both cases take the fallback.
  if (!buffer || buffer->Size() < static_cast<size_t>(len)) {
    LOG_W() << "generate random bytes using gnupg and openssl failed, fallback "
               "to qt random generator, len: "
            << len;

    QByteArray random_bytes(len, 0);
    for (int i = 0; i < len; ++i) {
      random_bytes[i] =
          static_cast<char>(QRandomGenerator::global()->bounded(256));
    }
    return GFBuffer(random_bytes);
  }

  return buffer;
}

}  // namespace

auto MapRandomBytesToAlphanum(const char *src, size_t src_size, char *dst,
                              size_t dst_size) -> size_t {
  if (src == nullptr || dst == nullptr) return 0;

  size_t written = 0;
  for (size_t i = 0; i < src_size && written < dst_size; ++i) {
    const auto byte = static_cast<unsigned char>(src[i]);
    if (byte >= kRejectThreshold) continue;

    dst[written++] = kAlphanum[byte % kAlphanumSize];
  }

  return written;
}

auto PassphraseGenerator::Generate(int len) -> GFBufferOrNone {
  if (len <= 0) return {};

  const auto target = static_cast<size_t>(len);
  GFBuffer result(target);
  auto *result_data = result.Data();

  // rejection sampling discards ~3% of input bytes, so a single draw of `len`
  // bytes usually falls a little short. top up until full, with a hard round
  // cap so a persistently failing random source cannot spin here.
  constexpr int kMaxRounds = 16;
  size_t filled = 0;

  for (int round = 0; round < kMaxRounds && filled < target; ++round) {
    const auto needed = static_cast<int>(target - filled);

    auto buffer = DrawRandomBytes(needed);
    if (!buffer || buffer->Empty()) {
      LOG_E() << "generate random bytes failed, len: " << needed;
      return {};
    }

    filled += MapRandomBytesToAlphanum(buffer->Data(), buffer->Size(),
                                       result_data + filled, target - filled);
  }

  if (filled < target) {
    LOG_E() << "failed to fill passphrase of length" << len << ", got"
            << filled;
    return {};
  }

  return result;
}

auto PassphraseGenerator::GenerateBytes(int len) -> GFBufferOrNone {
  auto buffer = SecureRandomGenerator::Generate(len);
  if (!buffer || buffer->Empty() || buffer->Size() < static_cast<size_t>(len)) {
    LOG_E() << "generate random bytes failed, len: " << len;
    return {};
  }
  return buffer;
}

auto PassphraseGenerator::GenerateBytesByOpenSSL(int len) -> GFBufferOrNone {
  auto buffer = SecureRandomGenerator::Generate(len);
  if (!buffer || buffer->Empty() || buffer->Size() < static_cast<size_t>(len)) {
    LOG_E() << "generate random bytes failed, len: " << len;
    return {};
  }
  return buffer;
}

PassphraseGenerator::PassphraseGenerator(int channel)
    : SingletonFunctionObject<PassphraseGenerator>(channel) {}
}  // namespace GpgFrontend