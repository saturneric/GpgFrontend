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

#include <gtest/gtest.h>

#include "core/model/GpgAbstractKey.h"
#include "shell/rules/KeyGroupRules.h"

namespace GpgFrontend::Test {

namespace {

/// A key that is only its answers; nothing else about it is asked.
struct CapabilityKey : GpgAbstractKey {
  bool encr = true;

  [[nodiscard]] auto ID() const -> QString override { return u"AAAA0001"_s; }
  [[nodiscard]] auto Fingerprint() const -> QString override {
    return u"AAAA0001AAAA0001AAAA0001AAAA0001AAAA0001"_s;
  }
  [[nodiscard]] auto KeyType() const -> GpgAbstractKeyType override {
    return GpgAbstractKeyType::kGPG_KEY;
  }
  [[nodiscard]] auto Name() const -> QString override { return u"Ann"_s; }
  [[nodiscard]] auto Email() const -> QString override { return {}; }
  [[nodiscard]] auto Comment() const -> QString override { return {}; }
  [[nodiscard]] auto IsPrivateKey() const -> bool override { return false; }
  [[nodiscard]] auto IsHasEncrCap() const -> bool override { return encr; }
  [[nodiscard]] auto IsHasSignCap() const -> bool override { return true; }
  [[nodiscard]] auto IsHasCertCap() const -> bool override { return true; }
  [[nodiscard]] auto IsHasAuthCap() const -> bool override { return false; }
  [[nodiscard]] auto IsRevoked() const -> bool override { return false; }
  [[nodiscard]] auto IsDisabled() const -> bool override { return false; }
  [[nodiscard]] auto IsExpired() const -> bool override { return false; }
  [[nodiscard]] auto IsGood() const -> bool override { return true; }
  [[nodiscard]] auto PublicKeyAlgo() const -> QString override {
    return u"EdDSA"_s;
  }
  [[nodiscard]] auto Algo() const -> QString override { return u"ED25519"_s; }
  [[nodiscard]] auto CreationTime() const -> QDateTime override {
    return QDateTime::fromSecsSinceEpoch(1700000000);
  }
  [[nodiscard]] auto ExpirationTime() const -> QDateTime override { return {}; }
};

// Never reached: a key refused for what it is never asks the repository.
constexpr int kUnusedChannel = 4099;

}  // namespace

TEST(ShellKeyGroupRulesTest, AKeyThatCannotEncryptStaysOutOfAGroup) {
  CapabilityKey sign_only;
  sign_only.encr = false;
  EXPECT_FALSE(
      Shell::CanJoinKeyGroup(kUnusedChannel, u"#&GROUP"_s, &sign_only));
}

TEST(ShellKeyGroupRulesTest, NoKeyJoinsAGroup) {
  EXPECT_FALSE(Shell::CanJoinKeyGroup(kUnusedChannel, u"#&GROUP"_s, nullptr));
}

}  // namespace GpgFrontend::Test
