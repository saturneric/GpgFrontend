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

#include "shell/rules/KeyExportRules.h"

namespace GpgFrontend::Test {

using Shell::KeyExportKind;
using Shell::KeyExportOptionsFor;

TEST(ShellKeyExportRulesTest, AShortPrivateKeyIsTheMinimalExport) {
  // What sets it apart from the full one; it was once the full export under
  // another name.
  const auto shortest = KeyExportOptionsFor(KeyExportKind::kShortSecret);
  EXPECT_TRUE(shortest.secret);
  EXPECT_TRUE(shortest.shortest);
  EXPECT_EQ(shortest.type, u"short_secret"_s);

  const auto full = KeyExportOptionsFor(KeyExportKind::kFullSecret);
  EXPECT_TRUE(full.secret);
  EXPECT_FALSE(full.shortest);
  EXPECT_EQ(full.type, u"full_secret"_s);
}

TEST(ShellKeyExportRulesTest, APublicKeyCarriesNoSecretAndIsText) {
  const auto pub = KeyExportOptionsFor(KeyExportKind::kPublic);
  EXPECT_FALSE(pub.secret);
  EXPECT_FALSE(pub.shortest);
  EXPECT_TRUE(pub.ascii);
  EXPECT_EQ(pub.type, u"pub"_s);
}

}  // namespace GpgFrontend::Test
