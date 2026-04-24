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

#include "core/function/GlobalSettingStation.h"

namespace GpgFrontend {

auto PassphraseGenerator::Generate(int len) -> GFBufferOrNone {
  static const std::array<char, 63> kAlphanum = {
      "0123456789"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      "abcdefghijklmnopqrstuvwxyz"};

  GFBufferOrNone buffer;
  if (GetGSS().IsEngineSupported(OpenPGPEngine::kGNUPG)) {
    buffer = rand_.GnuPGGenerate(len);
    if (!buffer || buffer->Empty() ||
        buffer->Size() < static_cast<size_t>(len)) {
      LOG_E() << "generate random bytes using gnupg failed, len: " << len;
    }
  }

  if (!buffer) {
    LOG_W() << "generate random bytes using gnupg failed, fallback to openssl, "
               "len: "
            << len;
    buffer = SecureRandomGenerator::OpenSSLGenerate(len);
    if (!buffer || buffer->Empty() ||
        buffer->Size() < static_cast<size_t>(len)) {
      LOG_E() << "generate random bytes using openssl failed, len: " << len;
    }
  }

  // fallback to qt random generator, which is not cryptographically secure, but
  // better than nothing
  if (!buffer) {
    LOG_W() << "generate random bytes using gnupg and openssl failed, fallback "
               "to qt random generator, len: "
            << len;
    QByteArray random_bytes(len, 0);
    for (int i = 0; i < len; ++i) {
      auto byte = QRandomGenerator::global()->bounded(-65535, 65535);
      random_bytes[i] = static_cast<char>(byte & 0xFF);
    }
    buffer = GFBuffer(random_bytes);
  }

  GFBuffer result(len);

  const size_t charset_size = sizeof(kAlphanum) - 1;
  const auto *data = buffer->Data();
  auto *result_data = result.Data();
  for (int i = 0; i < len; ++i) {
    size_t idx = static_cast<size_t>(data[i]) % charset_size;
    result_data[i] = kAlphanum[idx];
  }

  return result;
}

auto PassphraseGenerator::GenerateBytes(int len) -> GFBufferOrNone {
  auto buffer = rand_.GnuPGGenerate(len);
  if (!buffer || buffer->Empty() || buffer->Size() < static_cast<size_t>(len)) {
    LOG_E() << "generate random bytes failed, len: " << len;
    return {};
  }
  return buffer;
}

auto PassphraseGenerator::GenerateBytesByOpenSSL(int len) -> GFBufferOrNone {
  auto buffer = SecureRandomGenerator::OpenSSLGenerate(len);
  if (!buffer || buffer->Empty() || buffer->Size() < static_cast<size_t>(len)) {
    LOG_E() << "generate random bytes failed, len: " << len;
    return {};
  }
  return buffer;
}

PassphraseGenerator::PassphraseGenerator(int channel)
    : SingletonFunctionObject<PassphraseGenerator>(channel) {}
}  // namespace GpgFrontend