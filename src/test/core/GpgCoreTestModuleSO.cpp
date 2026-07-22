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
#include "core/struct/settings_object/ModuleSO.h"

namespace GpgFrontend::Test {

TEST_F(GFCoreTest, ModuleSODefaultsAreFalse) {
  const ModuleSO so{};

  EXPECT_FALSE(so.auto_activate);
  EXPECT_FALSE(so.set_by_user);
  EXPECT_TRUE(so.module_id.isEmpty());
}

TEST_F(GFCoreTest, ModuleSOEmptyJsonKeepsFlagsFalse) {
  // a module without stored settings must never be auto activated by accident
  const ModuleSO so{QJsonObject{}};

  EXPECT_FALSE(so.auto_activate);
  EXPECT_FALSE(so.set_by_user);
  EXPECT_TRUE(so.module_id.isEmpty());
  EXPECT_TRUE(so.module_hash.isEmpty());
}

TEST_F(GFCoreTest, ModuleSORoundTrip) {
  ModuleSO so;
  so.module_id = "com.bktus.gpgfrontend.module.test";
  so.module_version = "1.2.3";
  so.module_hash = "a91f00ff";
  so.auto_activate = true;
  so.set_by_user = true;

  const ModuleSO restored{so.ToJson()};

  EXPECT_EQ(restored.module_id, so.module_id);
  EXPECT_EQ(restored.module_version, so.module_version);
  EXPECT_EQ(restored.module_hash, so.module_hash);
  EXPECT_TRUE(restored.auto_activate);
  EXPECT_TRUE(restored.set_by_user);
}

TEST_F(GFCoreTest, ModuleSOIgnoresWrongTypedJsonValues) {
  QJsonObject j;
  j["module_id"] = 42;
  j["auto_activate"] = "yes";

  const ModuleSO so{j};

  EXPECT_TRUE(so.module_id.isEmpty());
  EXPECT_FALSE(so.auto_activate);
}

}  // namespace GpgFrontend::Test
