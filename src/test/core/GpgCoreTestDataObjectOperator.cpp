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

#include "core/function/DataObjectOperator.h"
#include "core/model/SettingsObject.h"

namespace GpgFrontend::Test {

namespace {

auto StoredObjectPath(const QString& ref_hex) -> QString {
  return GetGSS().GetDataObjectsDir() + "/" + ref_hex;
}

auto ReadStoredObject(const QString& ref_hex) -> QByteArray {
  QFile file(StoredObjectPath(ref_hex));
  if (!file.open(QIODevice::ReadOnly)) return {};
  return file.readAll();
}

// Damage the ciphertext of a stored object while leaving its 32-byte key-id
// prefix intact, so the object still resolves to a key we hold and fails at
// decryption rather than at lookup. That is the shape a profile takes when the
// bytes are fine but the key that wrote them is gone.
auto CorruptStoredObjectBody(const QString& ref_hex) -> bool {
  auto bytes = ReadStoredObject(ref_hex);
  if (bytes.size() <= 40) return false;

  bytes[40] = static_cast<char>(bytes[40] ^ 0xFF);

  QFile file(StoredObjectPath(ref_hex));
  if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
  return file.write(bytes) == bytes.size();
}

}  // namespace

TEST(DataObjectOperatorSingletonTest, StoreAndLoadJson) {
  auto& op = DataObjectOperator::GetInstance();

  QJsonObject obj{{"testKey", 123}};
  QJsonDocument doc(obj);

  auto ref = op.StoreDataObj("singleton-key1", doc);
  EXPECT_FALSE(ref.isEmpty());

  auto result = op.GetDataObject("singleton-key1");
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result.value().toJson(), doc.toJson());
}

TEST(DataObjectOperatorSingletonTest, StoreAndLoadBuffer) {
  auto& op = DataObjectOperator::GetInstance();

  GFBuffer plain("singleton-secret");
  auto ref = op.StoreSecDataObj("singleton-sec-key", plain);
  EXPECT_FALSE(ref.isEmpty());

  auto got = op.GetSecDataObject("singleton-sec-key");
  ASSERT_TRUE(got.has_value());
  EXPECT_EQ(*got, plain);
}

TEST(DataObjectOperatorSingletonTest, GetByRef) {
  auto& op = DataObjectOperator::GetInstance();

  QJsonObject obj{{"foo", 321}};
  QJsonDocument doc(obj);

  auto ref = op.StoreDataObj("singleton-key2", doc);
  ASSERT_FALSE(ref.isEmpty());

  auto result = op.GetDataObjectByRef(ref);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result.value().toJson(), doc.toJson());
}

TEST(DataObjectOperatorSingletonTest, SealingForAPackageMatchesStoringIt) {
  // What makes a rewritten data object readable on the other machine. The
  // package carries a copy built rather than copied -- key database paths in
  // the portable form -- and it is only found and opened there if it lands
  // under the same name, sealed the same way, as the store would have written.
  // The name is derived from the profile's own key, and that key travels.
  auto& op = DataObjectOperator::GetInstance();

  QJsonDocument doc(QJsonObject{{"key_databases", QJsonArray{}}});

  auto sealed = op.SealDataObjForPackage("singleton-seal-key", doc);
  ASSERT_TRUE(sealed.has_value());

  const auto ref = op.StoreDataObj("singleton-seal-key", doc);
  ASSERT_FALSE(ref.isEmpty());
  EXPECT_EQ(sealed->first, ref) << "the package would carry it under a name "
                                   "the recipient never looks for";

  // Sealing writes nothing of its own, and the bytes it produced open as the
  // object they claim to be.
  auto read = op.GetDataObjectByRef(sealed->first);
  ASSERT_TRUE(read.has_value());
  EXPECT_EQ(read->toJson(), doc.toJson());

  // Not byte-identical to the stored file -- every seal draws a fresh nonce --
  // so the sizes are what can be compared, and a mismatch there would mean the
  // two went through different framing.
  EXPECT_EQ(static_cast<qint64>(sealed->second.Size()),
            QFileInfo(StoredObjectPath(ref)).size());
}

TEST(DataObjectOperatorSingletonTest, SealingRefusesAnObjectWithNoName) {
  // An empty name would otherwise take get_object_ref()'s random branch and
  // produce a member nothing on the other side can ever ask for.
  auto& op = DataObjectOperator::GetInstance();

  EXPECT_FALSE(
      op.SealDataObjForPackage({}, QJsonDocument(QJsonObject{})).has_value());
}

TEST(DataObjectOperatorSingletonTest, InvalidRefReturnsEmpty) {
  auto& op = DataObjectOperator::GetInstance();

  auto result = op.GetDataObjectByRef("badref");
  EXPECT_FALSE(result.has_value());
}

TEST(DataObjectOperatorSingletonTest, GetDataObjectByRefRejectsNonHexRef) {
  auto& op = DataObjectOperator::GetInstance();

  // exactly 64 characters, but not hex. QByteArray::fromHex() silently skips
  // invalid characters rather than failing, so a length-only check would let
  // this through and resolve it to a different, shorter filename.
  const QString ref(64, 'z');
  ASSERT_EQ(ref.size(), 64);

  EXPECT_FALSE(op.GetDataObjectByRef(ref).has_value());
}

TEST(DataObjectOperatorSingletonTest, GetSecDataObjectByRefRejectsNonHexRef) {
  auto& op = DataObjectOperator::GetInstance();

  const QString ref(64, 'z');
  ASSERT_EQ(ref.size(), 64);

  EXPECT_FALSE(op.GetSecDataObjectByRef(ref).has_value());
}

TEST(DataObjectOperatorSingletonTest, RefWithEmbeddedNonHexIsRejected) {
  auto& op = DataObjectOperator::GetInstance();

  QJsonObject obj{{"bar", 42}};
  QJsonDocument doc(obj);

  auto ref = op.StoreDataObj("singleton-key3", doc);
  ASSERT_EQ(ref.size(), 64);

  // corrupt a single character into a non-hex one, keeping the length at 64.
  // fromHex() would drop it and shift the remaining nibbles, silently pointing
  // at an unrelated object instead of reporting failure.
  auto corrupted = ref;
  corrupted[10] = QChar('!');
  ASSERT_EQ(corrupted.size(), 64);

  EXPECT_FALSE(op.GetDataObjectByRef(corrupted).has_value());
}

TEST(DataObjectOperatorSingletonTest, GetDataObjectByRefRejectsWrongLengthRef) {
  auto& op = DataObjectOperator::GetInstance();

  EXPECT_FALSE(op.GetDataObjectByRef(QString(63, 'a')).has_value());
  EXPECT_FALSE(op.GetDataObjectByRef(QString(65, 'a')).has_value());
  EXPECT_FALSE(op.GetDataObjectByRef(QString{}).has_value());
}

TEST(DataObjectOperatorSingletonTest, GetSecDataObjectByRefRoundTripsValidRef) {
  auto& op = DataObjectOperator::GetInstance();

  GFBuffer plain("singleton-sec-by-ref");
  auto ref = op.StoreSecDataObj("singleton-sec-key2", plain);
  ASSERT_EQ(ref.size(), 64);

  auto got = op.GetSecDataObjectByRef(ref);
  ASSERT_TRUE(got.has_value());
  EXPECT_EQ(*got, plain);
}

TEST(SettingsObjectTest, ModifiedSettingsReachDisk) {
  auto& op = DataObjectOperator::GetInstance();
  const QString name = "so-write-test";

  {
    SettingsObject so(name);
    so.insert("k", 7);
  }  // destructor writes the changed object through

  auto stored = op.GetDataObject(name);
  ASSERT_TRUE(stored.has_value());
  ASSERT_TRUE(stored->isObject());
  EXPECT_EQ(stored->object().value("k").toInt(), 7);
}

TEST(SettingsObjectTest, UnchangedSettingsSkipDiskWrite) {
  auto& op = DataObjectOperator::GetInstance();
  const QString name = "so-unchanged-test";

  {
    SettingsObject so(name);
    so.insert("k", 1);
  }
  ASSERT_TRUE(op.GetDataObject(name).has_value());

  // Drop the on-disk object behind the object's back, then construct a
  // read-only SettingsObject that mutates nothing. Because its contents match
  // what was loaded, the destructor must be a no-op -- the disk copy stays
  // gone.
  op.RemoveDataObj(name);
  ASSERT_FALSE(op.GetDataObject(name).has_value());

  {
    SettingsObject so(name);
    (void)so;
  }
  EXPECT_FALSE(op.GetDataObject(name).has_value());
}

TEST(SettingsObjectTest, ChangedSettingsWriteThrough) {
  auto& op = DataObjectOperator::GetInstance();
  const QString name = "so-changed-test";

  {
    SettingsObject so(name);
    so.insert("v", 1);
  }
  ASSERT_TRUE(op.GetDataObject(name).has_value());

  // Remove behind its back, then load-and-mutate: a genuine change must still
  // be written through even though the on-disk copy is absent at load time.
  op.RemoveDataObj(name);
  ASSERT_FALSE(op.GetDataObject(name).has_value());

  {
    SettingsObject so(name);
    so.insert("v", 2);
  }

  auto stored = op.GetDataObject(name);
  ASSERT_TRUE(stored.has_value());
  ASSERT_TRUE(stored->isObject());
  EXPECT_EQ(stored->object().value("v").toInt(), 2);
}

// A name nothing was ever stored under must stay writable. "Absent" and
// "unreadable" both surface as an empty load, and conflating them here would
// mean no profile could ever write its first settings object.
TEST(SettingsObjectTest, StoresNormallyWhenObjectAbsent) {
  auto& op = DataObjectOperator::GetInstance();
  const QString name = "so-absent-test";

  op.RemoveDataObj(name);
  ASSERT_FALSE(op.HasDataObj(name));

  {
    SettingsObject so(name);
    EXPECT_FALSE(so.LoadFailed());
    so.insert("fresh", 1);
  }

  auto stored = op.GetDataObject(name);
  ASSERT_TRUE(stored.has_value());
  ASSERT_TRUE(stored->isObject());
  EXPECT_EQ(stored->object().value("fresh").toInt(), 1);
}

// The Leg B regression test: a stored object that cannot be decrypted must
// survive a load-and-mutate cycle untouched. Overwriting it would replace the
// user's only copy with the empty object we fell back to.
TEST(SettingsObjectTest, RefusesToOverwriteUnreadableObject) {
  auto& op = DataObjectOperator::GetInstance();
  const QString name = "so-unreadable-test";

  const QJsonObject original{{"v", 1},
                             {"pad", "some padding to grow the body"}};
  const auto ref = op.StoreDataObj(name, QJsonDocument(original));
  ASSERT_FALSE(ref.isEmpty());
  ASSERT_TRUE(CorruptStoredObjectBody(ref));

  const auto before = ReadStoredObject(ref);
  ASSERT_FALSE(before.isEmpty());

  ASSERT_TRUE(op.HasDataObj(name));
  ASSERT_FALSE(op.GetDataObject(name).has_value());

  {
    SettingsObject so(name);
    EXPECT_TRUE(so.LoadFailed());
    so.insert("v", 2);
  }  // destructor must decline to write

  EXPECT_EQ(ReadStoredObject(ref), before);
}

TEST(SettingsObjectTest, StoreReturnsFalseAfterFailedLoad) {
  auto& op = DataObjectOperator::GetInstance();
  const QString name = "so-store-refused-test";

  const QJsonObject original{{"v", 1},
                             {"pad", "some padding to grow the body"}};
  const auto ref = op.StoreDataObj(name, QJsonDocument(original));
  ASSERT_FALSE(ref.isEmpty());
  ASSERT_TRUE(CorruptStoredObjectBody(ref));

  const auto before = ReadStoredObject(ref);

  {
    SettingsObject so(name);
    ASSERT_TRUE(so.LoadFailed());
    EXPECT_FALSE(so.Store(QJsonObject{{"replaced", true}}));
    EXPECT_FALSE(so.contains("replaced"));
  }

  EXPECT_EQ(ReadStoredObject(ref), before);
}

// The escape hatch: without it the guard above would lock a user out of the
// one screen that can repair a broken profile.
TEST(SettingsObjectTest, StoreOverridingUnreadableWritesThrough) {
  auto& op = DataObjectOperator::GetInstance();
  const QString name = "so-override-test";

  const QJsonObject original{{"v", 1},
                             {"pad", "some padding to grow the body"}};
  const auto ref = op.StoreDataObj(name, QJsonDocument(original));
  ASSERT_FALSE(ref.isEmpty());
  ASSERT_TRUE(CorruptStoredObjectBody(ref));

  {
    SettingsObject so(name);
    ASSERT_TRUE(so.LoadFailed());
    so.StoreOverridingUnreadable(QJsonObject{{"repaired", 42}});
  }

  auto stored = op.GetDataObject(name);
  ASSERT_TRUE(stored.has_value());
  ASSERT_TRUE(stored->isObject());
  EXPECT_EQ(stored->object().value("repaired").toInt(), 42);
}

// Nothing decrypted, but objects are asking for a key we do not hold: the key
// is wrong, not the objects. Collecting here is what destroyed profiles.
TEST(DataObjectGCTest, KeySetSuspectWhenNothingDecrypts) {
  EXPECT_TRUE(IsWholesaleKeyFailure(/*ok=*/0, /*missing_key=*/10));
}

// One healthy object proves the active key works, so the stragglers really are
// orphans and may be collected.
TEST(DataObjectGCTest, KeySetNotSuspectWithAnyHealthyObject) {
  EXPECT_FALSE(IsWholesaleKeyFailure(/*ok=*/9, /*missing_key=*/1));
}

TEST(DataObjectGCTest, KeySetNotSuspectWhenNothingIsMissing) {
  EXPECT_FALSE(IsWholesaleKeyFailure(/*ok=*/10, /*missing_key=*/0));
}

TEST(DataObjectGCTest, KeySetNotSuspectOnEmptyProfile) {
  EXPECT_FALSE(IsWholesaleKeyFailure(/*ok=*/0, /*missing_key=*/0));
}

}  // namespace GpgFrontend::Test