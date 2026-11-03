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

#include <QByteArray>
#include <QList>
#include <QPair>
#include <vector>

#include "GpgFrontendTest.h"
#include "core/function/ArchiveFileOperator.h"
#include "host/SdkTestContext.h"
#include "sdk/GFSDKArchive.h"
#include "sdk/GFSDKBuffer.h"
#include "sdk/GFSDKBuffer.hpp"

/**
 * @file GpgCoreTestSdkArchive.cpp
 * @brief In-memory ZIP for modules: round trip, and the Host's extraction
 * rules applied to what a module reads.
 */

namespace GpgFrontend::Test {

namespace {

auto Ctx() -> GFSDKContext* {
  static SdkTestContext context("com.example.sdk.archive");
  return context.get();
}

using Entries = QList<QPair<QByteArray, QByteArray>>;

auto Collect(void* user, const char* path, GFBufferView data) -> int {
  auto* out = static_cast<Entries*>(user);
  out->append({QByteArray(path),
               QByteArray(static_cast<const char*>(GFBufferData(Ctx(), data)),
                          static_cast<qsizetype>(GFBufferSize(Ctx(), data)))});
  return 0;
}

auto Write(const Entries& files) -> QByteArray {
  std::vector<GFBuf> keep;
  keep.reserve(files.size());
  std::vector<GFArchiveEntry> entries;
  for (const auto& [path, body] : files) {
    keep.push_back(GFBuf::Copy(Ctx(), body));
    entries.push_back({path.constData(), keep.back().View(), 0});
  }
  GFBuf out(Ctx());
  if (GFArchiveZipWrite(Ctx(), entries.data(), entries.size(), out.Out()) != 0)
    return {};
  return out.UnwipeableCopy();
}

auto Read(const QByteArray& zip, GFArchiveLimits limits, Entries* out) -> int {
  auto in = GFBuf::Copy(Ctx(), zip);
  return GFArchiveZipRead(Ctx(), in.View(), &limits, &Collect, out, nullptr);
}

auto NoLimits() -> GFArchiveLimits {
  return {sizeof(GFArchiveLimits), -1, -1, -1};
}

/// What a hostile archive holds: written by the core, which takes its paths
/// as given, so the reader's rules are what is tested.
auto WriteUnchecked(const Entries& files) -> QByteArray {
  qsizetype index = 0;
  QByteArray zip;
  const auto err = ArchiveFileOperator::NewArchiveFromMembersToSink(
      [&](ArchiveMemberEntry& entry) {
        if (index >= files.size()) return false;
        const auto& [path, body] = files.at(index++);
        entry.relative_path = QString::fromUtf8(path);
        entry.bytes = GFBuffer(body);
        return true;
      },
      [&](const char* data, qint64 size) {
        zip.append(data, size);
        return true;
      },
      ArchiveCompression::kNONE, ArchiveFormat::kZIP);
  return err == 0 ? zip : QByteArray();
}

}  // namespace

TEST(SdkArchiveTest, WhatIsWrittenIsWhatIsRead) {
  const QByteArray binary("a\0b\xff", 4);
  const auto zip = Write({{"Note.textbundle/info.json", "{\"version\":2}"},
                          {"Note.textbundle/text.markdown", "# hi\n"},
                          {"Note.textbundle/assets/x.png", binary}});
  ASSERT_FALSE(zip.isEmpty());
  EXPECT_TRUE(zip.startsWith("PK"));

  Entries read;
  ASSERT_EQ(Read(zip, NoLimits(), &read), 0);
  ASSERT_EQ(read.size(), 3);
  EXPECT_EQ(read[0].first, "Note.textbundle/info.json");
  EXPECT_EQ(read[1].second, "# hi\n");
  EXPECT_EQ(read[2].second, binary);
}

TEST(SdkArchiveTest, HighlyRepetitiveContentIsNotARatioFailure) {
  const QByteArray text(4 * 1024 * 1024, 'a');
  const auto zip = Write({{"text.markdown", text}});
  ASSERT_FALSE(zip.isEmpty());
  Entries read;
  ASSERT_EQ(Read(zip, NoLimits(), &read), 0);
  EXPECT_EQ(read.value(0).second.size(), text.size());
}

TEST(SdkArchiveTest, BudgetsAreEnforced) {
  const auto zip = Write({{"a", QByteArray(1000, 'x')}, {"b", "y"}});
  Entries read;
  EXPECT_NE(Read(zip, {sizeof(GFArchiveLimits), -1, 999, -1}, &read), 0)
      << "one entry over the per-entry budget";
  read.clear();
  EXPECT_NE(Read(zip, {sizeof(GFArchiveLimits), 1000, -1, -1}, &read), 0)
      << "all entries over the total budget";
  read.clear();
  EXPECT_NE(Read(zip, {sizeof(GFArchiveLimits), -1, -1, 1}, &read), 0)
      << "more entries than allowed";
}

TEST(SdkArchiveTest, AModuleCannotWriteAnEscapingPath) {
  EXPECT_TRUE(Write({{"../evil", "x"}}).isEmpty());
  EXPECT_TRUE(Write({{"/abs", "x"}}).isEmpty());
  EXPECT_TRUE(Write({{"", "x"}}).isEmpty());
  EXPECT_FALSE(Write({{"./a/b", "x"}}).isEmpty()) << "normalised, not refused";
}

TEST(SdkArchiveTest, TraversalAndDuplicatesAreRefused) {
  for (const auto& hostile :
       {Entries{{"../evil", "x"}}, Entries{{"/abs", "x"}},
        Entries{{"a", "1"}, {"a", "2"}}, Entries{{"a/B", "1"}, {"a/b", "2"}}}) {
    const auto zip = WriteUnchecked(hostile);
    ASSERT_FALSE(zip.isEmpty());
    Entries read;
    EXPECT_NE(Read(zip, NoLimits(), &read), 0) << hostile.value(0).first.data();
  }
}

TEST(SdkArchiveTest, ReadingNeedsLimits) {
  auto in = GFBuf::Copy(Ctx(), Write({{"a", "x"}}));
  Entries read;
  EXPECT_NE(
      GFArchiveZipRead(Ctx(), in.View(), nullptr, &Collect, &read, nullptr), 0)
      << "no ceiling is said with -1, never by omission";
  EXPECT_TRUE(read.isEmpty());
}

TEST(SdkArchiveTest, GarbageIsRefusedWithAReason) {
  auto in = GFBuf::Copy(Ctx(), QByteArray("not a zip at all"));
  Entries read;
  GFBuf why(Ctx());
  const auto limits = NoLimits();
  EXPECT_NE(
      GFArchiveZipRead(Ctx(), in.View(), &limits, &Collect, &read, why.Out()),
      0);
  EXPECT_FALSE(why.Empty());
}

}  // namespace GpgFrontend::Test
