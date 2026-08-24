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
#include "ui/function/FilePanelPath.h"

namespace GpgFrontend::Test {

using UI::FilePanelDefaultPathMode;

TEST(FilePanelPathTest, ModeSurvivesAStringRoundTrip) {
  for (const auto mode :
       {FilePanelDefaultPathMode::kWORKSPACE, FilePanelDefaultPathMode::kHOME,
        FilePanelDefaultPathMode::kCWD}) {
    EXPECT_EQ(UI::FilePanelDefaultPathModeFromString(
                  UI::FilePanelDefaultPathModeToString(mode)),
              mode);
  }
}

TEST(FilePanelPathTest, UnknownStoredValuesFallBackToHome) {
  // A setting written by a newer build, or corrupted, must still open
  // something rather than leave the panel with no root at all.
  for (const auto* s : {"", "  ", "nonsense", "WORKSPACE_"}) {
    EXPECT_EQ(UI::FilePanelDefaultPathModeFromString(s),
              FilePanelDefaultPathMode::kHOME);
  }
}

TEST(FilePanelPathTest, StoredValuesAreReadCaseAndSpaceInsensitively) {
  EXPECT_EQ(UI::FilePanelDefaultPathModeFromString(" WorkSpace "),
            FilePanelDefaultPathMode::kWORKSPACE);
  EXPECT_EQ(UI::FilePanelDefaultPathModeFromString("CWD"),
            FilePanelDefaultPathMode::kCWD);
}

TEST(FilePanelPathTest, EachModeResolvesToItsOwnDirectory) {
  EXPECT_EQ(UI::ResolveFilePanelDefaultPath(
                FilePanelDefaultPathMode::kWORKSPACE, "/ws", "/home", "/cwd"),
            "/ws");
  EXPECT_EQ(UI::ResolveFilePanelDefaultPath(FilePanelDefaultPathMode::kHOME,
                                            "/ws", "/home", "/cwd"),
            "/home");
  EXPECT_EQ(UI::ResolveFilePanelDefaultPath(FilePanelDefaultPathMode::kCWD,
                                            "/ws", "/home", "/cwd"),
            "/cwd");
}

TEST(FilePanelPathTest, AnUnresolvableWorkspaceFallsBackToHome) {
  // Opening nothing at all is worse than opening the old default.
  EXPECT_EQ(UI::ResolveFilePanelDefaultPath(
                FilePanelDefaultPathMode::kWORKSPACE, "", "/home", "/cwd"),
            "/home");
}

}  // namespace GpgFrontend::Test
