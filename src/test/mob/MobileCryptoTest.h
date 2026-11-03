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

#pragma once

#include <future>

#include "MobileTest.h"
#include "core/function/openpgp/ChannelHandle.h"
#include "core/function/openpgp/GpgKeyRepository.h"
#include "core/function/openpgp/KeyGenerationOperation.h"
#include "core/model/GpgGenerateKeyResult.h"
#include "core/model/GpgKeyGenerateInfo.h"
#include "core/utils/GpgUtils.h"
#include "core/utils/MemoryUtils.h"
#include "crypto/PassphraseBroker.h"

/**
 * @file MobileCryptoTest.h
 * @brief Answering passphrase prompts and making keys, for the tests that
 * run real operations. The prompts reach the application's PassphraseBroker,
 * on the interface thread, exactly as they would reach the sheet.
 */

namespace GpgFrontend::Test::Mobile {

using GpgFrontend::Mobile::PassphraseBroker;

inline auto Broker() -> PassphraseBroker* {
  return PassphraseBroker::create(nullptr, nullptr);
}

/// The request the passphrase sheet would show, read on its thread.
inline auto CurrentRequest() -> QVariantMap {
  QVariantMap request;
  RunOnMainThread([&request]() { request = Broker()->Current(); });
  return request;
}

inline auto WaitForRequest(int timeout_ms = 20000) -> QVariantMap {
  QVariantMap request;
  WaitFor(
      [&request]() {
        request = CurrentRequest();
        return !request.isEmpty();
      },
      timeout_ms);
  return request;
}

inline void Answer(const QString& passphrase) {
  RunOnMainThread([&passphrase]() { Broker()->submit(passphrase); });
}

inline void Decline() {
  RunOnMainThread([]() { Broker()->cancel(); });
}

/// A new ed25519 key with a cv25519 subkey to encrypt to; with @p passphrase,
/// protected by it, typed into the broker as a person would.
inline auto GenerateKey(const QString& name, const QString& passphrase) -> QString {
  auto info = SecureCreateSharedObject<KeyGenerateInfo>();
  info->SetName(name);
  info->SetEmail(name.toLower() + "@mobile.example.invalid");
  info->SetAlgo(std::get<1>(KeyGenerateInfo::SearchPrimaryKeyAlgo("ed25519")));
  info->SetNonPassPhrase(passphrase.isEmpty());
  info->SetNonExpired(true);
  info->SetKeyVersion(4);

  auto subkey = SecureCreateSharedObject<KeyGenerateInfo>();
  subkey->SetIsSubKey(true);
  subkey->SetAlgo(std::get<1>(KeyGenerateInfo::SearchSubKeyAlgo("cv25519")));
  subkey->SetNonPassPhrase(passphrase.isEmpty());
  subkey->SetNonExpired(true);

  const auto channel = UsableDefaultChannelHandle();
  auto generating = std::async(std::launch::async, [channel, info, subkey]() {
    return KeyGenerationOperation::GetInstance(channel)
        .GenerateKeyWithSubkeySync(info, subkey);
  });

  // The new key's passphrase, asked as often as the engine asks.
  while (!passphrase.isEmpty() &&
         generating.wait_for(std::chrono::milliseconds(20)) !=
             std::future_status::ready) {
    if (!CurrentRequest().isEmpty()) Answer(passphrase);
  }

  auto [err, data] = generating.get();
  if (!err.Ok() || data == nullptr) return {};
  GpgKeyRepository::GetInstance(channel).FlushKeyCache();
  return ExtractParams<GpgGenerateKeyResult>(data, 0).GetFingerprint();
}

}  // namespace GpgFrontend::Test::Mobile
