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
#include "core/utils/GpgUtils.h"
#include "ui/widgets/StatusIndicatorInfo.h"

namespace GpgFrontend::Test {

namespace {

using UI::DescribeDeploymentIndicator;
using UI::DescribeEngineIndicator;
using UI::DescribeProfileIndicator;

}  // namespace

TEST(StatusIndicatorInfoTest, AProfileOnThisComputerShowsItsNameAndItsRoot) {
  const auto info = DescribeProfileIndicator(
      ProfileKind::kPERSIST, "Work", "/home/u/.gpgfrontend", QString(), true);

  EXPECT_EQ(info.value, "Work");
  EXPECT_FALSE(info.caption.isEmpty());
  EXPECT_TRUE(
      info.tooltip.contains(QDir::toNativeSeparators("/home/u/.gpgfrontend")));
}

TEST(StatusIndicatorInfoTest, AProfileOpenedFromAFileSaysItIsTemporary) {
  // The window looks exactly the same either way, so if the value does not say
  // it, nothing does — and the question on closing comes out of nowhere.
  const auto info =
      DescribeProfileIndicator(ProfileKind::kPACKAGED, "Handover",
                               "/tmp/extracted", "/mnt/usb/x.gfp", true);

  EXPECT_NE(info.value, "Handover");
  EXPECT_TRUE(info.value.contains("Handover"));

  // The package is what the user has to think about on closing, not the
  // directory it was unpacked into.
  EXPECT_TRUE(
      info.tooltip.contains(QDir::toNativeSeparators("/mnt/usb/x.gfp")));
  EXPECT_FALSE(
      info.tooltip.contains(QDir::toNativeSeparators("/tmp/extracted")));
}

TEST(StatusIndicatorInfoTest, TheProfileTooltipSurvivesAnUnknownRoot) {
  const auto info = DescribeProfileIndicator(
      ProfileKind::kINSTALLED_ROOT, "Default", QString(), QString(), true);

  EXPECT_EQ(info.value, "Default");
  EXPECT_FALSE(info.tooltip.isEmpty());
  EXPECT_FALSE(info.tooltip.contains("\n\n\n"));
}

TEST(StatusIndicatorInfoTest, AProfileThatCannotBeManagedPromisesNoClick) {
  // The App Store build ships without profiles, so the segment is inert there.
  // The hint is the only part of it that says otherwise, and a tooltip offering
  // to manage profiles on a build that has none is worse than no tooltip.
  const auto inert = DescribeProfileIndicator(
      ProfileKind::kINSTALLED_ROOT, "Default", "/x", QString(), false);
  const auto clickable = DescribeProfileIndicator(
      ProfileKind::kINSTALLED_ROOT, "Default", "/x", QString(), true);

  EXPECT_EQ(inert.value, clickable.value);
  EXPECT_EQ(inert.caption, clickable.caption);

  // Everything the reading itself says survives; only the hint goes.
  EXPECT_FALSE(inert.tooltip.isEmpty());
  EXPECT_TRUE(inert.tooltip.contains(QDir::toNativeSeparators("/x")));
  EXPECT_FALSE(inert.tooltip.contains("\n\n"));
  EXPECT_TRUE(clickable.tooltip.startsWith(inert.tooltip));
  EXPECT_GT(clickable.tooltip.length(), inert.tooltip.length());
}

TEST(StatusIndicatorInfoTest, TheEngineReadingCarriesItsVersion) {
  const auto info = DescribeEngineIndicator(OpenPGPEngine::kGNUPG, "2.4.3",
                                            "default", "/home/u/.gnupg");

  EXPECT_EQ(info.value, "GnuPG 2.4.3");
  EXPECT_TRUE(info.tooltip.contains("default"));
  EXPECT_TRUE(
      info.tooltip.contains(QDir::toNativeSeparators("/home/u/.gnupg")));
}

TEST(StatusIndicatorInfoTest, AnEngineStillComingUpShowsItsNameAlone) {
  // No version yet is not the same as a trailing space where one should be.
  const auto info = DescribeEngineIndicator(OpenPGPEngine::kRPGP, QString(),
                                            QString(), QString());

  EXPECT_EQ(info.value, ConvertOpenPGPEngine2String(OpenPGPEngine::kRPGP));
  EXPECT_EQ(info.value.trimmed(), info.value);
  EXPECT_FALSE(info.tooltip.isEmpty());
}

TEST(StatusIndicatorInfoTest, PortableAndInstalledAreToldApart) {
  const auto portable = DescribeDeploymentIndicator(true, false);
  const auto installed = DescribeDeploymentIndicator(false, false);

  EXPECT_FALSE(portable.value.isEmpty());
  EXPECT_NE(portable.value, installed.value);
  EXPECT_NE(portable.tooltip, installed.tooltip);
}

TEST(StatusIndicatorInfoTest, ASelfContainedProfileSaysSoAndNoOtherDoes) {
  const auto contained = DescribeDeploymentIndicator(false, true);
  const auto shared = DescribeDeploymentIndicator(false, false);

  EXPECT_GT(contained.tooltip.length(), shared.tooltip.length());
  EXPECT_TRUE(
      contained.tooltip.startsWith(shared.tooltip.section("\n\n", 0, 0)));
}

TEST(StatusIndicatorInfoTest, EveryTooltipEndsWithWhatAClickOpens) {
  // The segments look like text until you hover them; the hint is the only
  // thing that says they are not.
  const QList<UI::StatusIndicatorInfo> infos = {
      DescribeProfileIndicator(ProfileKind::kPERSIST, "Work", "/x", QString(),
                               true),
      DescribeEngineIndicator(OpenPGPEngine::kGNUPG, "2.4.3", "default", "/y"),
      DescribeDeploymentIndicator(true, true),
  };

  for (const auto& info : infos) {
    ASSERT_TRUE(info.tooltip.contains("\n\n"));
    EXPECT_FALSE(info.tooltip.section("\n\n", -1).isEmpty());
  }

  // The two segments that open the same dialog say the same thing about it.
  EXPECT_EQ(infos[1].tooltip.section("\n\n", -1),
            infos[2].tooltip.section("\n\n", -1));
  EXPECT_NE(infos[0].tooltip.section("\n\n", -1),
            infos[1].tooltip.section("\n\n", -1));
}

}  // namespace GpgFrontend::Test
