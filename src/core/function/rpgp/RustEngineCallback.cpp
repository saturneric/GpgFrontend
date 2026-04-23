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

#include "RustEngineCallback.h"

#include <cstring>

#include "core/function/CoreSignalStation.h"
#include "core/function/GFKeyDatabase.h"
#include "core/function/openpgp/AbstractKeyRepository.h"
#include "core/model/GpgPassphraseContext.h"

namespace GpgFrontend {

auto FetchPublicKeyCallback(const char* fpr, void* user_data) -> char* {
  if ((fpr == nullptr) || (user_data == nullptr)) return nullptr;

  auto* key_db = static_cast<GFKeyDatabase*>(user_data);
  QString fingerprint = QString::fromUtf8(fpr);

  LOG_D() << "Rust FFI requested public key for issuer: " << fingerprint;

  auto key_block = key_db->GetKeyBlocks(fingerprint);
  if (key_block && !key_block->public_key.isEmpty()) {
    QByteArray utf8 = key_block->public_key.toUtf8();
    // Allocate memory using strdup (or standard malloc) for Rust to safely
    // consume
    char* c_str = reinterpret_cast<char*>(
        SMAMalloc(utf8.size() + 1));  // +1 for null terminator
    std::memcpy(c_str, utf8.constData(), utf8.size());
    c_str[utf8.size()] = '\0';
    return c_str;
  }
  return nullptr;
}

auto FetchSecretKeyCallback(const char* fpr, void* user_data) -> char* {
  if ((fpr == nullptr) || (user_data == nullptr)) return nullptr;

  auto* key_db = static_cast<GFKeyDatabase*>(user_data);
  QString fingerprint = QString::fromUtf8(fpr);

  LOG_D() << "Rust FFI requested secret key for issuer: " << fingerprint;

  auto key_block = key_db->GetKeyBlocks(fingerprint);
  if (key_block && !key_block->secret_key.isEmpty()) {
    QByteArray utf8 = key_block->secret_key.toUtf8();
    // Allocate memory using strdup (or standard malloc) for Rust to safely
    // consume
    char* c_str = reinterpret_cast<char*>(
        SMAMalloc(utf8.size() + 1));  // +1 for null terminator
    std::memcpy(c_str, utf8.constData(), utf8.size());
    c_str[utf8.size()] = '\0';
    return c_str;
  }
  return nullptr;
}

auto FetchPasswordCallback(int channel, const char* fpr, const char* info,
                           uint8_t** out_pwd, void* /*user_data*/) -> int {
  QString qs_fpr = QString::fromUtf8(fpr).toUpper();
  GpgAbstractKeyPtr key = nullptr;
  if (qs_fpr.length() > 0) {
    key = AbstractKeyRepository::GetInstance(channel).GetKey(qs_fpr);
  }
  auto qs_info = info != nullptr ? QString::fromUtf8(info) : QString();

  LOG_D() << "Rust FFI requested password for key fpr: " << qs_fpr
          << ", info: " << qs_info << ", channel: " << channel;

  if (key) {
    LOG_D() << "Key ID: " << key->ID() << ", User IDs: " << key->UID().size();
  } else {
    LOG_D() << "No key found for fpr: " << qs_fpr;
  }

  QString result_pwd;

  auto c = QSharedPointer<GpgPassphraseContext>::create(channel, key, qs_info,
                                                        false, false);

  QEventLoop loop;
  QTimer timeout_timer;
  timeout_timer.setSingleShot(true);
  QObject::connect(&timeout_timer, &QTimer::timeout, &loop, &QEventLoop::quit);

  QObject::connect(
      CoreSignalStation::GetInstance(),
      &CoreSignalStation::SignalUserInputPassphraseReady, &loop,
      [&result_pwd, &c, &qs_fpr,
       &loop](const QSharedPointer<GpgPassphraseContext>& ctx) -> void {
        if ((qs_fpr.isEmpty() && ctx->GetKey() == nullptr) ||
            (c->GetKey() != nullptr &&
             ctx->GetKey()->ID() == c->GetKey()->ID())) {
          result_pwd = ctx->GetPassphrase();
        }
        loop.quit();
      });

  QTimer::singleShot(0, [c]() -> void {
    emit CoreSignalStation::GetInstance()->SignalNeedUserInputPassphrase(c);
  });

  timeout_timer.start(60000);
  loop.exec();
  timeout_timer.stop();

  if (result_pwd.isEmpty()) {
    *out_pwd = nullptr;
    return 0;
  }

  // Convert QString to UTF-8 byte array
  QByteArray utf8_pwd = result_pwd.toUtf8();

  // Allocate raw memory for Rust to consume
  auto* c_pwd = reinterpret_cast<uint8_t*>(SMAMalloc(utf8_pwd.size()));
  std::memcpy(c_pwd, utf8_pwd.constData(), utf8_pwd.size());

  // Attempt to clear the Qt buffers (Best effort since QString handles its own
  // memory)
  result_pwd.fill('X');
  utf8_pwd.fill('X');

  *out_pwd = c_pwd;
  return static_cast<int>(utf8_pwd.size());
}

void FreeCallback(void* ptr, void*) {
  if (ptr != nullptr) {
    SMAFree(ptr);
  }
}

}  // namespace GpgFrontend
