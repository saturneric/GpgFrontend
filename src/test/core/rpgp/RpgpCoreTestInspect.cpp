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

// The OpenPGP structure inspector, from the C++ side of the FFI.
//
// The packet walk itself is checked exhaustively by the Rust inline tests in
// rust/src/inspect.rs. What can only be checked here is the chain: that the
// core wrapper hands the bytes across, gets the document back, and frees what
// Rust allocated. So these assert the shape of the document, not the meaning
// of every field.

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include "RpgpCoreTestRfc9580.h"
#include "core/utils/RustUtils.h"

namespace GpgFrontend::Test {

namespace {

auto InspectVector(const QString& name) -> QJsonObject {
  auto json = InspectOpenPGPData(LoadRfc9580Vector(name));
  EXPECT_FALSE(json.isEmpty()) << "no document for " << name.toStdString();

  QJsonParseError error{};
  auto parsed = QJsonDocument::fromJson(json, &error);
  EXPECT_EQ(error.error, QJsonParseError::NoError)
      << error.errorString().toStdString();
  return parsed.object();
}

auto FirstBlockPackets(const QJsonObject& document) -> QJsonArray {
  return document.value("blocks")
      .toArray()
      .at(0)
      .toObject()
      .value("packets")
      .toArray();
}

}  // namespace

TEST_F(RpgpCoreTest, InspectDetachedSignatureReportsOneSignaturePacket) {
  auto document = InspectVector("sig_good_detached.sig");

  EXPECT_EQ(document.value("format").toString(), QString("armored"));
  auto packets = FirstBlockPackets(document);
  ASSERT_EQ(packets.size(), 1);
  EXPECT_EQ(packets.at(0).toObject().value("tag").toInt(), 2);
}

TEST_F(RpgpCoreTest, InspectEncryptedMessageNeverOpensTheContainer) {
  auto packets = FirstBlockPackets(InspectVector("enc_v1seipd_mdc.pgp"));
  ASSERT_GT(packets.size(), 0);

  auto container = packets.at(packets.size() - 1).toObject();
  EXPECT_EQ(container.value("tag").toInt(), 18);
  EXPECT_TRUE(container.value("children").toArray().isEmpty());
}

TEST_F(RpgpCoreTest, InspectCompressedMessageRecurses) {
  auto packets = FirstBlockPackets(InspectVector("sig_inline_compressed.pgp"));

  bool found_children = false;
  for (const auto& entry : packets) {
    auto packet = entry.toObject();
    if (packet.value("tag").toInt() != 8) continue;
    found_children = !packet.value("children").toArray().isEmpty();
  }
  EXPECT_TRUE(found_children) << "a compressed container must be expanded";
}

TEST_F(RpgpCoreTest, InspectCleartextSignedMessageIsReportedAsCleartext) {
  EXPECT_EQ(InspectVector("sig_good_cleartext.asc").value("format").toString(),
            QString("cleartext"));
}

TEST_F(RpgpCoreTest, InspectGarbageIsADocumentNotAFailure) {
  // The distinction the dialog depends on: unparsable input still produces a
  // document, so the reader is told what is wrong instead of nothing at all.
  auto document = InspectVector("garbage.bin");
  EXPECT_EQ(document.value("size").toInt(), 49);
  EXPECT_FALSE(document.value("errors").toArray().isEmpty());
}

TEST_F(RpgpCoreTest, InspectEmptyInputIsAnEmptyDocument) {
  auto json = InspectOpenPGPData(GFBuffer());
  ASSERT_FALSE(json.isEmpty());

  auto document = QJsonDocument::fromJson(json).object();
  EXPECT_EQ(document.value("size").toInt(), 0);
  EXPECT_TRUE(document.value("blocks").toArray().isEmpty());
}

TEST_F(RpgpCoreTest, InspectPacketOffsetsAbut) {
  // The invariant that proves the framing is read off the wire rather than
  // reconstructed from what rPGP would write.
  auto packets = FirstBlockPackets(InspectVector("two_signer.pgp"));
  ASSERT_GT(packets.size(), 1);

  for (int i = 0; i + 1 < packets.size(); ++i) {
    auto a = packets.at(i).toObject();
    auto b = packets.at(i + 1).toObject();
    EXPECT_EQ(a.value("offset").toInt() + a.value("headerLength").toInt() +
                  a.value("bodyLength").toInt(),
              b.value("offset").toInt());
  }
}

TEST_F(RpgpCoreTest, InspectIsRepeatableAndReleasesItsDocument) {
  // Each call gets a fresh allocation that the wrapper frees; running it many
  // times is what would surface a leak or a double free under the sanitizer.
  auto vector = LoadRfc9580Vector("enc_multi_recipient.pgp");
  for (int i = 0; i < 32; ++i) {
    EXPECT_FALSE(InspectOpenPGPData(vector).isEmpty());
  }
}

}  // namespace GpgFrontend::Test
