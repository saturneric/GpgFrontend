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
#include "ui/main_window/ToolBarHelper.h"

namespace GpgFrontend::Test {

using UI::GroupCryptoOperas;
using Groups = std::vector<std::vector<GpgOperation>>;

TEST(GFUiToolBarHelperTest, AllOperasCollapseIntoFourFamilies) {
  const int all = kENCRYPT | kDECRYPT | kSIGN | kVERIFY | kENCRYPT_SIGN |
                  kDECRYPT_VERIFY | kSYMMETRIC_ENCRYPT;

  EXPECT_EQ(GroupCryptoOperas(all),
            (Groups{{kENCRYPT, kENCRYPT_SIGN, kSYMMETRIC_ENCRYPT},
                    {kDECRYPT, kDECRYPT_VERIFY},
                    {kSIGN},
                    {kVERIFY}}));
}

TEST(GFUiToolBarHelperTest, DefaultMaskKeepsOneButtonPerOpera) {
  // AppearanceSO's default: no variants ticked, so no split buttons.
  EXPECT_EQ(GroupCryptoOperas(kENCRYPT | kDECRYPT | kSIGN | kVERIFY),
            (Groups{{kENCRYPT}, {kDECRYPT}, {kSIGN}, {kVERIFY}}));
}

TEST(GFUiToolBarHelperTest, VariantsWithoutTheirPlainOperaStayInOrder) {
  EXPECT_EQ(GroupCryptoOperas(kSYMMETRIC_ENCRYPT | kENCRYPT_SIGN),
            (Groups{{kENCRYPT_SIGN, kSYMMETRIC_ENCRYPT}}));
  EXPECT_EQ(GroupCryptoOperas(kVERIFY | kDECRYPT_VERIFY),
            (Groups{{kDECRYPT_VERIFY}, {kVERIFY}}));
}

TEST(GFUiToolBarHelperTest, NonToolBarBitsAreDropped) {
  EXPECT_TRUE(GroupCryptoOperas(kNONE).empty());
  // kAUTH / kCERT, and the retired IM toolbar bits 1 << 9 and 1 << 10 that an
  // old stored mask may still carry.
  EXPECT_TRUE(GroupCryptoOperas(kAUTH | kCERT | (1 << 9) | (1 << 10)).empty());
}

}  // namespace GpgFrontend::Test
