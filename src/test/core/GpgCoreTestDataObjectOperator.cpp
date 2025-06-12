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

}  // namespace GpgFrontend::Test