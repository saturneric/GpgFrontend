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

#include "GpgFrontendTest.h"
#include "core/function/GlobalSettingStation.h"
#include "core/module/ModuleInit.h"

/**
 * @file GpgCoreTestModuleLoadingPolicy.cpp
 * @brief The one setting that decides what the loader is allowed to load.
 *
 * It is persisted, compared in three files, and used to be compared as a bare
 * string -- so an unrecognised value silently behaved exactly like a chosen
 * "only_integrated", and a stored typo was indistinguishable from an intent.
 * These pin both halves: every valid key round-trips, and every invalid one
 * fails closed AND says so.
 */

namespace GpgFrontend::Test {

using Module::ModuleLoadingPolicy;
using Module::ModuleLoadingPolicyKey;
using Module::ParseModuleLoadingPolicy;

TEST(ModuleLoadingPolicyTest, EveryValidKeyParsesToItsOwnPolicy) {
  EXPECT_EQ(ParseModuleLoadingPolicy("disable").policy,
            ModuleLoadingPolicy::kDISABLE);
  EXPECT_EQ(ParseModuleLoadingPolicy("only_integrated").policy,
            ModuleLoadingPolicy::kONLY_INTEGRATED);
  EXPECT_EQ(ParseModuleLoadingPolicy("all").policy, ModuleLoadingPolicy::kALL);
  EXPECT_EQ(ParseModuleLoadingPolicy("packaged_only").policy,
            ModuleLoadingPolicy::kPACKAGED_ONLY);

  for (const auto key :
       {"disable", "only_integrated", "all", "packaged_only"}) {
    EXPECT_TRUE(ParseModuleLoadingPolicy(key).recognised) << key;
  }
}

TEST(ModuleLoadingPolicyTest, EveryPolicyRoundTripsThroughItsKey) {
  for (const auto policy :
       {ModuleLoadingPolicy::kDISABLE, ModuleLoadingPolicy::kONLY_INTEGRATED,
        ModuleLoadingPolicy::kALL, ModuleLoadingPolicy::kPACKAGED_ONLY}) {
    const auto key = ModuleLoadingPolicyKey(policy);
    EXPECT_FALSE(key.isEmpty());

    const auto back = ParseModuleLoadingPolicy(key);
    EXPECT_TRUE(back.recognised) << key.toStdString();
    EXPECT_EQ(back.policy, policy) << key.toStdString();
  }
}

// The whole point of the `recognised` flag. A value this build does not
// understand must not come back looking like a choice the user made.
TEST(ModuleLoadingPolicyTest, AnUnknownKeyFailsClosedAndSaysSo) {
  for (const auto* key : {"", " ", "ALL", "All", "only integrated",
                          "only_integrated ", " disable", "packaged",
                          "packaged_only_please", "true", "1", "disabled"}) {
    const auto parsed = ParseModuleLoadingPolicy(QString::fromUtf8(key));

    EXPECT_FALSE(parsed.recognised)
        << "accepted an unknown policy key: '" << key << "'";
    EXPECT_EQ(parsed.policy, ModuleLoadingPolicy::kONLY_INTEGRATED)
        << "fell back to something other than the safe default for: '" << key
        << "'";
  }
}

// Case and whitespace are NOT quietly forgiven: a stored value that differs
// from what this build writes is a value that came from somewhere else.
TEST(ModuleLoadingPolicyTest, TheKeysAreExactAndLowerCase) {
  for (const auto policy :
       {ModuleLoadingPolicy::kDISABLE, ModuleLoadingPolicy::kONLY_INTEGRATED,
        ModuleLoadingPolicy::kALL, ModuleLoadingPolicy::kPACKAGED_ONLY}) {
    const auto key = ModuleLoadingPolicyKey(policy);
    EXPECT_EQ(key, key.toLower());
    EXPECT_EQ(key, key.trimmed());
  }
}

// It is a stored setting, so it has to survive being stored.
TEST(ModuleLoadingPolicyTest, EveryPolicySurvivesPersistence) {
  auto settings = GetSettings();
  const auto original = settings.value("basic/module_loading_policy");

  for (const auto policy :
       {ModuleLoadingPolicy::kDISABLE, ModuleLoadingPolicy::kONLY_INTEGRATED,
        ModuleLoadingPolicy::kALL, ModuleLoadingPolicy::kPACKAGED_ONLY}) {
    settings.setValue("basic/module_loading_policy",
                      ModuleLoadingPolicyKey(policy));
    settings.sync();

    const auto parsed = ParseModuleLoadingPolicy(
        GetSettings().value("basic/module_loading_policy").toString());
    EXPECT_TRUE(parsed.recognised);
    EXPECT_EQ(parsed.policy, policy);
  }

  // A settings file written by hand, or by a newer build.
  settings.setValue("basic/module_loading_policy", "something_else_entirely");
  settings.sync();
  const auto parsed = ParseModuleLoadingPolicy(
      GetSettings().value("basic/module_loading_policy").toString());
  EXPECT_FALSE(parsed.recognised);
  EXPECT_EQ(parsed.policy, ModuleLoadingPolicy::kONLY_INTEGRATED);

  if (original.isValid()) {
    settings.setValue("basic/module_loading_policy", original);
  } else {
    settings.remove("basic/module_loading_policy");
  }
  settings.sync();
}

}  // namespace GpgFrontend::Test
