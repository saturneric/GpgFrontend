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
#include "ui/function/ExportKey.h"

namespace GpgFrontend::Test {

TEST(ExportKeyFileNameTest, BuildsTheConventionalShape) {
  const auto name =
      UI::ExportKeyFileName("Alice", "alice@example.com", "4CAB31FD", "pub");

#ifdef Q_OS_WINDOWS
  EXPECT_EQ(name, "Alice[alice@example.com](4CAB31FD)_pub.asc");
#else
  EXPECT_EQ(name, "Alice<alice@example.com>(4CAB31FD)_pub.asc");
#endif
}

TEST(ExportKeyFileNameTest, SpacesInTheNameBecomeUnderscores) {
  const auto name =
      UI::ExportKeyFileName("Alice Example", "a@b.c", "ID", "pub");
  EXPECT_TRUE(name.startsWith("Alice_Example"));
  EXPECT_FALSE(name.contains(' '));
}

TEST(ExportKeyFileNameTest, StripsCharactersNoFilesystemAccepts) {
  // A user ID may legitimately hold a slash or a colon. Substituting one
  // straight into the template produced a default filename the save dialog
  // rejects — on every platform, not only Windows.
  const auto name =
      UI::ExportKeyFileName(R"(A/B\C:D"E|F?G*H)", "a@b.c", "ID", "pub");

  for (const auto forbidden : {'/', '\\', ':', '"', '|', '?', '*'}) {
    EXPECT_FALSE(name.contains(QChar(forbidden)))
        << "kept " << forbidden << " in " << name.toStdString();
  }
  EXPECT_TRUE(name.startsWith("ABCDEFGH"));
}

TEST(ExportKeyFileNameTest, AngleBracketsInAFieldDoNotBreakTheTemplate) {
  // On POSIX the template's own <> must survive, but a pair smuggled in
  // through a field would produce a nested, unreadable name.
  const auto name = UI::ExportKeyFileName("<Alice>", "a@b.c", "ID", "pub");
  EXPECT_TRUE(name.startsWith("Alice"));
}

TEST(ExportKeyFileNameTest, EmptyFieldsStillProduceAUsableName) {
  const auto name = UI::ExportKeyFileName("", "", "4CAB31FD", "pub");

  EXPECT_TRUE(name.endsWith("_pub.asc"));
  EXPECT_TRUE(name.contains("4CAB31FD"));
}

TEST(ExportKeyFileNameTest, TypeDistinguishesWhatWasExported) {
  // Exporting the public and the private half of one key into the same folder
  // must not offer the same filename twice.
  const auto pub = UI::ExportKeyFileName("Alice", "a@b.c", "ID", "pub");
  const auto full =
      UI::ExportKeyFileName("Alice", "a@b.c", "ID", "full_secret");
  const auto shortest =
      UI::ExportKeyFileName("Alice", "a@b.c", "ID", "short_secret");

  EXPECT_TRUE(pub.endsWith("_pub.asc"));
  EXPECT_TRUE(full.endsWith("_full_secret.asc"));
  EXPECT_TRUE(shortest.endsWith("_short_secret.asc"));
  EXPECT_NE(pub, full);
  EXPECT_NE(full, shortest);
}

}  // namespace GpgFrontend::Test
