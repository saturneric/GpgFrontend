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

}  // namespace GpgFrontend::Test