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

#include "GFCoreTest.h"
#include "core/model/GpgOpenPGPCard.h"

namespace GpgFrontend::Test {

namespace {

/// a trimmed but realistic SCD LEARN status dump of an OpenPGP v3.4 token
auto SampleCardStatus() -> QStringList {
  return {
      "READER Yubico+YubiKey+OTP+FIDO+CCID+00+00",
      "SERIALNO D2760001240103040006166156210000",
      "APPTYPE OPENPGP",
      "APPVERSION 0304",
      "CARDTYPE yubikey",
      "CARDVERSION 0201",
      "MANUFACTURER 6 Yubico",
      "DISP-NAME Lee<<Chris",
      "DISP-LANG en",
      "DISP-SEX 1",
      // chv1 cached, then the three max lengths, then the three retry counters
      "CHV-STATUS +0+127+127+127+3+0+3",
      "EXTCAP ki=1+aac=0+bt=0+kdf=1+si=3",
      "KEYPAIRINFO 1234ABCD OPENPGP.1 SC - ed25519",
      "KEY-FPR 1 aabbccddeeff00112233445566778899aabbccdd",
      "KEY-TIME 1 1700000000",
      "SIGCOUNT 42",
  };
}

}  // namespace

TEST_F(GFCoreTest, OpenPGPCardParsesIdentity) {
  const GpgOpenPGPCard card(SampleCardStatus());

  EXPECT_TRUE(card.good);
  // the reader name arrives with '+' as the space separator
  EXPECT_EQ(card.reader, "Yubico YubiKey OTP FIDO CCID 00 00");
  EXPECT_EQ(card.serial_number, "D2760001240103040006166156210000");
  EXPECT_EQ(card.app_type, "OPENPGP");
  EXPECT_EQ(card.card_type, "yubikey");
  EXPECT_EQ(card.manufacturer_id, 6);
  EXPECT_EQ(card.manufacturer, "Yubico");
  EXPECT_EQ(card.display_language, "en");
  EXPECT_EQ(card.display_sex, "Male");

  // DISP-NAME is stored surname first, it is shown given name first
  EXPECT_EQ(card.card_holder, "Chris Lee");
}

TEST_F(GFCoreTest, OpenPGPCardParsesChvStatus) {
  const GpgOpenPGPCard card(SampleCardStatus());

  EXPECT_EQ(card.chv1_cached, 0);

  EXPECT_EQ(card.chv_max_len.at(0), 127);
  EXPECT_EQ(card.chv_max_len.at(1), 127);
  EXPECT_EQ(card.chv_max_len.at(2), 127);

  // the dialog renders these three as the PIN / Reset Code / Admin PIN chips,
  // an exhausted counter has to survive parsing as a real 0
  EXPECT_EQ(card.chv_retry.at(0), 3);
  EXPECT_EQ(card.chv_retry.at(1), 0);
  EXPECT_EQ(card.chv_retry.at(2), 3);
}

TEST_F(GFCoreTest, OpenPGPCardParsesExtendedCapabilities) {
  const GpgOpenPGPCard card(SampleCardStatus());

  EXPECT_TRUE(card.ext_cap.ki);
  EXPECT_FALSE(card.ext_cap.aac);
  EXPECT_FALSE(card.ext_cap.bt);
  EXPECT_TRUE(card.ext_cap.kdf);
  EXPECT_EQ(card.ext_cap.status_indicator, 3);
}

TEST_F(GFCoreTest, OpenPGPCardParsesCardKeyInfo) {
  const GpgOpenPGPCard card(SampleCardStatus());

  ASSERT_TRUE(card.card_keys_info.contains(1));
  const auto& key = card.card_keys_info.value(1);

  EXPECT_EQ(key.key_type, "OPENPGP");
  EXPECT_EQ(key.grip, "1234ABCD");
  EXPECT_EQ(key.usage, "SC");
  EXPECT_EQ(key.algo, "ED25519");
  EXPECT_EQ(key.fingerprint, "AABBCCDDEEFF00112233445566778899AABBCCDD");
  EXPECT_EQ(key.created.toSecsSinceEpoch(), 1700000000);
}

TEST_F(GFCoreTest, OpenPGPCardParsesUifFlags) {
  auto status = SampleCardStatus();
  // 0xFF means "not supported / off", anything else enables the flag
  status << "UIF-1 %01%20" << "UIF-2 %FF%20" << "UIF-3 %FF%20";

  const GpgOpenPGPCard card(status);

  EXPECT_TRUE(card.uif.sign);
  EXPECT_FALSE(card.uif.encrypt);
  EXPECT_FALSE(card.uif.auth);
}

TEST_F(GFCoreTest, OpenPGPCardKeepsUnknownFieldsAsAdditionalInfo) {
  const GpgOpenPGPCard card(SampleCardStatus());

  // unrecognised keywords must not be dropped, the dialog lists them verbatim
  EXPECT_EQ(card.additional_card_infos.value("SIGCOUNT"), "42");
}

TEST_F(GFCoreTest, OpenPGPCardHandlesEmptyStatus) {
  const GpgOpenPGPCard card{};

  EXPECT_FALSE(card.good);
  EXPECT_TRUE(card.card_keys_info.isEmpty());
  EXPECT_EQ(card.chv_retry.at(0), -1);
  EXPECT_EQ(card.ext_cap.status_indicator, -1);
}

}  // namespace GpgFrontend::Test
