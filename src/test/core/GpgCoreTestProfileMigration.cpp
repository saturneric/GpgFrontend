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

#include <QCryptographicHash>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QTemporaryDir>

#include "core/profile/ProfileMigration.h"
#include "core/utils/BuildInfoUtils.h"

namespace GpgFrontend::Test {

namespace {

constexpr auto kNow = "2026-08-03T10:00:00Z";

auto MakeMarker(int schema, int min_reader = 1) -> ProfileMarker {
  ProfileMarker marker;
  marker.schema_version = schema;
  marker.min_reader_version = min_reader;
  marker.profile = "GpgFrontend";
  marker.last_writer_version = "9.9.9";
  return marker;
}

/// A rung that does nothing but record that it ran, so a ladder's ordering and
/// commit behaviour can be asserted without any real migration existing yet.
auto NoopRung(int from, int to, ProfileMigrationStage stage,
              QStringList* trace = nullptr) -> ProfileMigration {
  ProfileMigration rung;
  rung.from = from;
  rung.to = to;
  rung.name = QString("noop_%1_%2").arg(from).arg(to);
  rung.stage = stage;
  rung.apply = [name = rung.name, trace](const QString&) {
    if (trace != nullptr) trace->append(name);
    return ProfileMigrationOutcome{};
  };
  return rung;
}

auto FailingRung(int from, int to, ProfileMigrationStage stage)
    -> ProfileMigration {
  ProfileMigration rung;
  rung.from = from;
  rung.to = to;
  rung.name = QString("fail_%1_%2").arg(from).arg(to);
  rung.stage = stage;
  rung.apply = [](const QString&) {
    return ProfileMigrationOutcome{false, "deliberate test failure"};
  };
  return rung;
}

/// Hash of every file in a tree, so "nothing was touched" can be asserted
/// rather than sampled.
auto HashTree(const QString& root) -> QByteArray {
  QCryptographicHash hash(QCryptographicHash::Sha256);
  QStringList entries;
  QDirIterator it(root, QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden,
                  QDirIterator::Subdirectories);
  while (it.hasNext()) entries.append(it.next());
  entries.sort();

  for (const auto& path : entries) {
    hash.addData(QDir(root).relativeFilePath(path).toUtf8());
    QFile f(path);
    if (f.open(QIODevice::ReadOnly)) hash.addData(f.readAll());
  }
  return hash.result();
}

auto WriteMarker(const QString& path, const ProfileMarker& marker) -> bool {
  return WriteProfileMarker(path, marker);
}

}  // namespace

// --------------------------------------------------------------- the plan

TEST(ProfileMigrationPlanTest, MissingMarkerIsAFirstRunNotAFailure) {
  const auto plan = PlanProfileMigration(ProfileMarker{}, false, 2, {});
  EXPECT_EQ(plan.verdict, ProfileMigrationVerdict::kNONE);
}

TEST(ProfileMigrationPlanTest, EqualVersionsNeedNothing) {
  const auto plan = PlanProfileMigration(MakeMarker(2), true, 2, {});
  EXPECT_EQ(plan.verdict, ProfileMigrationVerdict::kNONE);
}

// The path that runs on every start for the rest of the product's life: a
// profile already at the current version has nothing to do, and that is
// asserted explicitly rather than assumed.
TEST(ProfileMigrationPlanTest, AnUpToDateProfileNeedsNoWork) {
  const auto current = GetAppProfileSchemaVersion();

  const auto plan = PlanProfileMigration(MakeMarker(current), true, current,
                                         AllProfileMigrationNames());
  EXPECT_EQ(plan.verdict, ProfileMigrationVerdict::kNONE);

  for (const auto stage :
       {ProfileMigrationStage::kPRE_KEY, ProfileMigrationStage::kPOST_KEY}) {
    EXPECT_TRUE(
        ProfileMigrationsFor(current, current, stage, AllProfileMigrations())
            .isEmpty());
  }
}

// Every rung must be reachable from the ladder: a rung whose `to` is above the
// build's own version could never run, and one below the oldest supported
// version could never be reached either.
TEST(ProfileMigrationPlanTest, TheShippedLadderIsConsistentWithTheBuild) {
  const auto current = GetAppProfileSchemaVersion();
  const auto ladder = AllProfileMigrations();

  QSet<QString> names;
  for (const auto& rung : ladder) {
    EXPECT_EQ(rung.to, rung.from + 1)
        << "rungs are consecutive: " << rung.name.toStdString();
    EXPECT_GE(rung.from, kOldestSupportedProfileSchema)
        << rung.name.toStdString();
    EXPECT_LE(rung.to, current)
        << "a rung above the build's own layout version can never run: "
        << rung.name.toStdString();

    EXPECT_FALSE(rung.name.isEmpty());
    EXPECT_FALSE(names.contains(rung.name))
        << "rung names are stable identifiers and must be unique: "
        << rung.name.toStdString();
    names.insert(rung.name);

    EXPECT_TRUE(static_cast<bool>(rung.apply))
        << "rung has no work: " << rung.name.toStdString();
  }

  EXPECT_EQ(AllProfileMigrationNames().size(), ladder.size());
}

TEST(ProfileMigrationPlanTest, OlderSchemaUpgrades) {
  const auto plan = PlanProfileMigration(MakeMarker(1), true, 3, {});
  EXPECT_EQ(plan.verdict, ProfileMigrationVerdict::kUPGRADE);
  EXPECT_EQ(plan.from, 1);
  EXPECT_EQ(plan.to, 3);
}

TEST(ProfileMigrationPlanTest, NewerSchemaIsTooNewAndNamesItsWriter) {
  const auto plan = PlanProfileMigration(MakeMarker(5), true, 2, {});
  EXPECT_EQ(plan.verdict, ProfileMigrationVerdict::kTOO_NEW);
  EXPECT_EQ(plan.writer_version, QString("9.9.9"));
  EXPECT_FALSE(plan.reason.isEmpty());
}

TEST(ProfileMigrationPlanTest, HigherMinReaderIsRefused) {
  // the writer explicitly declared this layout unreadable to us
  const auto plan = PlanProfileMigration(MakeMarker(2, 4), true, 2, {});
  EXPECT_EQ(plan.verdict, ProfileMigrationVerdict::kTOO_NEW);
}

TEST(ProfileMigrationPlanTest, UnknownFutureRungIsRefused) {
  auto marker = MakeMarker(2);
  ProfileMigrationRecord record;
  record.from = 2;
  record.to = 7;
  record.name = "from_a_fork";
  marker.migrations.append(record);

  const auto plan = PlanProfileMigration(marker, true, 2, {"known_rung"});
  EXPECT_EQ(plan.verdict, ProfileMigrationVerdict::kREFUSE);
  EXPECT_TRUE(plan.reason.contains("from_a_fork"));
}

TEST(ProfileMigrationPlanTest, ASkippedRecordDoesNotTriggerARefusal) {
  auto marker = MakeMarker(2);
  ProfileMigrationRecord record;
  record.from = 2;
  record.to = 7;
  record.name = "skipped_elsewhere";
  record.skipped = true;
  marker.migrations.append(record);

  EXPECT_EQ(PlanProfileMigration(marker, true, 2, {}).verdict,
            ProfileMigrationVerdict::kNONE);
}

TEST(ProfileMigrationPlanTest, TooOldToUpgradeIsRefusedNotGuessedAt) {
  const auto plan = PlanProfileMigration(MakeMarker(0), true, 2, {});
  EXPECT_EQ(plan.verdict, ProfileMigrationVerdict::kREFUSE);
}

// ------------------------------------------------------------- the ladder

TEST(ProfileMigrationLadderTest, ReturnsConsecutiveRungsInOrder) {
  const QList<ProfileMigration> ladder = {
      NoopRung(4, 5, ProfileMigrationStage::kPRE_KEY),
      NoopRung(2, 3, ProfileMigrationStage::kPRE_KEY),
      NoopRung(3, 4, ProfileMigrationStage::kPRE_KEY),
  };

  const auto rungs =
      ProfileMigrationsFor(2, 5, ProfileMigrationStage::kPRE_KEY, ladder);

  ASSERT_EQ(rungs.size(), 3);
  EXPECT_EQ(rungs[0].from, 2);
  EXPECT_EQ(rungs[1].from, 3);
  EXPECT_EQ(rungs[2].from, 4);
}

TEST(ProfileMigrationLadderTest, StagesAreSeparate) {
  const QList<ProfileMigration> ladder = {
      NoopRung(2, 3, ProfileMigrationStage::kPRE_KEY),
      NoopRung(3, 4, ProfileMigrationStage::kPOST_KEY),
  };

  EXPECT_EQ(ProfileMigrationsFor(2, 4, ProfileMigrationStage::kPRE_KEY, ladder)
                .size(),
            1);
  EXPECT_EQ(ProfileMigrationsFor(2, 4, ProfileMigrationStage::kPOST_KEY, ladder)
                .size(),
            1);
}

TEST(ProfileMigrationLadderTest, EqualOrInvertedRangeYieldsNothing) {
  const QList<ProfileMigration> ladder = {
      NoopRung(2, 3, ProfileMigrationStage::kPRE_KEY)};
  EXPECT_TRUE(
      ProfileMigrationsFor(3, 3, ProfileMigrationStage::kPRE_KEY, ladder)
          .isEmpty());
  EXPECT_TRUE(
      ProfileMigrationsFor(4, 2, ProfileMigrationStage::kPRE_KEY, ladder)
          .isEmpty());
}

// -------------------------------------------------------------- the commit

class ProfileMigrationRunTest : public ::testing::Test {
 protected:
  void SetUp() override {
    ASSERT_TRUE(dir_.isValid());
    root_ = dir_.path();
    marker_path_ = root_ + "/profile.json";
  }

  QTemporaryDir dir_;
  QString root_;
  QString marker_path_;
};

TEST_F(ProfileMigrationRunTest, EachRungIsCommittedBeforeTheNextOneStarts) {
  ASSERT_TRUE(WriteMarker(marker_path_, MakeMarker(1)));

  QStringList trace;
  const QList<ProfileMigration> ladder = {
      NoopRung(1, 2, ProfileMigrationStage::kPRE_KEY, &trace),
      NoopRung(2, 3, ProfileMigrationStage::kPRE_KEY, &trace),
  };

  const auto plan =
      PlanProfileMigration(MakeMarker(1), true, 3, {"noop_1_2", "noop_2_3"});
  const auto result =
      RunProfileMigration(root_, marker_path_, plan,
                          ProfileMigrationStage::kPRE_KEY, false, kNow, ladder);

  EXPECT_TRUE(result.ok);
  EXPECT_EQ(result.reached, 3);
  EXPECT_EQ(trace, QStringList({"noop_1_2", "noop_2_3"}));

  const auto after = ReadProfileMarker(marker_path_);
  ASSERT_TRUE(after.has_value());
  EXPECT_EQ(after->schema_version, 3);
  ASSERT_EQ(after->migrations.size(), 2);
  EXPECT_EQ(after->migrations[0].name, QString("noop_1_2"));
  EXPECT_EQ(after->migrations[0].at, QString(kNow));
}

TEST_F(ProfileMigrationRunTest, AFailingRungLeavesTheLastCommittedVersion) {
  ASSERT_TRUE(WriteMarker(marker_path_, MakeMarker(1)));

  const QList<ProfileMigration> ladder = {
      NoopRung(1, 2, ProfileMigrationStage::kPRE_KEY),
      FailingRung(2, 3, ProfileMigrationStage::kPRE_KEY),
      NoopRung(3, 4, ProfileMigrationStage::kPRE_KEY),
  };

  const auto plan = PlanProfileMigration(MakeMarker(1), true, 4, {});
  const auto result =
      RunProfileMigration(root_, marker_path_, plan,
                          ProfileMigrationStage::kPRE_KEY, false, kNow, ladder);

  EXPECT_FALSE(result.ok);
  EXPECT_EQ(result.failed_rung, QString("fail_2_3"));
  EXPECT_EQ(result.reached, 2);

  // on disk, not just in the return value: that is what the next start reads
  const auto after = ReadProfileMarker(marker_path_);
  ASSERT_TRUE(after.has_value());
  EXPECT_EQ(after->schema_version, 2);
  ASSERT_EQ(after->migrations.size(), 1);
  EXPECT_EQ(after->migrations[0].name, QString("noop_1_2"));
}

TEST_F(ProfileMigrationRunTest, AnInterruptedLadderResumesWhereItStopped) {
  ASSERT_TRUE(WriteMarker(marker_path_, MakeMarker(1)));

  // first attempt dies on the second rung
  {
    const QList<ProfileMigration> ladder = {
        NoopRung(1, 2, ProfileMigrationStage::kPRE_KEY),
        FailingRung(2, 3, ProfileMigrationStage::kPRE_KEY),
    };
    const auto plan = PlanProfileMigration(MakeMarker(1), true, 3, {});
    RunProfileMigration(root_, marker_path_, plan,
                        ProfileMigrationStage::kPRE_KEY, false, kNow, ladder);
  }

  // the next start re-plans from what is actually on disk
  QStringList trace;
  const QList<ProfileMigration> fixed = {
      NoopRung(1, 2, ProfileMigrationStage::kPRE_KEY, &trace),
      NoopRung(2, 3, ProfileMigrationStage::kPRE_KEY, &trace),
  };
  const auto marker = ReadProfileMarker(marker_path_);
  ASSERT_TRUE(marker.has_value());
  const auto plan = PlanProfileMigration(*marker, true, 3, {});
  const auto result =
      RunProfileMigration(root_, marker_path_, plan,
                          ProfileMigrationStage::kPRE_KEY, false, kNow, fixed);

  EXPECT_TRUE(result.ok);
  EXPECT_EQ(result.reached, 3);
  // rung 1 already ran and committed; re-running it would work on migrated data
  EXPECT_EQ(trace, QStringList({"noop_2_3"}));
}

TEST_F(ProfileMigrationRunTest, ClassicSkipsAndRecordsFileLevelRungs) {
  ASSERT_TRUE(WriteMarker(marker_path_, MakeMarker(1)));

  bool ran = false;
  auto rung = NoopRung(1, 2, ProfileMigrationStage::kPRE_KEY);
  rung.runs_on_classic = false;
  rung.apply = [&ran](const QString&) {
    ran = true;
    return ProfileMigrationOutcome{};
  };

  const auto plan = PlanProfileMigration(MakeMarker(1), true, 2, {});
  const auto result = RunProfileMigration(root_, marker_path_, plan,
                                          ProfileMigrationStage::kPRE_KEY,
                                          /*is_classic=*/true, kNow, {rung});

  EXPECT_TRUE(result.ok);
  EXPECT_FALSE(ran);
  EXPECT_EQ(result.reached, 2);

  const auto after = ReadProfileMarker(marker_path_);
  ASSERT_TRUE(after.has_value());
  ASSERT_EQ(after->migrations.size(), 1);
  // recorded, so a later audit cannot mistake a skip for a run
  EXPECT_TRUE(after->migrations[0].skipped);
  EXPECT_EQ(after->migrations[0].reason, QString("classic"));
}

TEST_F(ProfileMigrationRunTest, RaisingTheMinimumReaderIsOptOut) {
  ASSERT_TRUE(WriteMarker(marker_path_, MakeMarker(1, 1)));

  auto plain = NoopRung(1, 2, ProfileMigrationStage::kPRE_KEY);
  auto breaking = NoopRung(2, 3, ProfileMigrationStage::kPRE_KEY);
  breaking.raises_min_reader_to = 3;

  const auto plan = PlanProfileMigration(MakeMarker(1, 1), true, 3, {});
  RunProfileMigration(root_, marker_path_, plan,
                      ProfileMigrationStage::kPRE_KEY, false, kNow,
                      {plain, breaking});

  const auto after = ReadProfileMarker(marker_path_);
  ASSERT_TRUE(after.has_value());
  EXPECT_EQ(after->min_reader_version, 3);
}

// ------------------------------------------------- nothing is touched first

TEST_F(ProfileMigrationRunTest, AnIncompatibleProfileIsNotModifiedAtAll) {
  // a profile from a newer build, with a file of its own alongside the marker
  ASSERT_TRUE(WriteMarker(marker_path_, MakeMarker(9, 9)));
  {
    QFile f(root_ + "/from_the_future.bin");
    ASSERT_TRUE(f.open(QIODevice::WriteOnly));
    f.write("newer build's data");
  }

  const auto before = HashTree(root_);

  const auto marker = ReadProfileMarker(marker_path_);
  ASSERT_TRUE(marker.has_value());
  const auto plan = PlanProfileMigration(*marker, true, 2, {});
  ASSERT_EQ(plan.verdict, ProfileMigrationVerdict::kTOO_NEW);

  // the run is a no-op for anything that is not kUPGRADE
  RunProfileMigration(root_, marker_path_, plan,
                      ProfileMigrationStage::kPRE_KEY, false, kNow,
                      {NoopRung(1, 2, ProfileMigrationStage::kPRE_KEY)});

  // byte-identical: opening a newer profile with an older build must not be
  // what corrupts it
  EXPECT_EQ(HashTree(root_), before);
}

// -------------------------------------------------------- marker durability

TEST_F(ProfileMigrationRunTest, UnknownFieldsSurviveARewrite) {
  // a newer build's extra keys must not be dropped by an older one
  {
    QJsonObject obj;
    obj["schema_version"] = 2;
    obj["min_reader_version"] = 1;
    obj["profile"] = "GpgFrontend";
    obj["a_field_from_the_future"] = "keep me";
    QJsonObject nested;
    nested["deep"] = 42;
    obj["future_object"] = nested;

    QFile f(marker_path_);
    ASSERT_TRUE(f.open(QIODevice::WriteOnly));
    f.write(QJsonDocument(obj).toJson());
  }

  auto marker = ReadProfileMarker(marker_path_);
  ASSERT_TRUE(marker.has_value());
  marker->last_writer_version = "2.2.2";
  ASSERT_TRUE(WriteProfileMarker(marker_path_, *marker));

  QFile f(marker_path_);
  ASSERT_TRUE(f.open(QIODevice::ReadOnly));
  const auto obj = QJsonDocument::fromJson(f.readAll()).object();

  EXPECT_EQ(obj.value("a_field_from_the_future").toString(),
            QString("keep me"));
  EXPECT_EQ(obj.value("future_object").toObject().value("deep").toInt(), 42);
  EXPECT_EQ(obj.value("last_writer_version").toString(), QString("2.2.2"));
}

TEST_F(ProfileMigrationRunTest, LegacyFiveFieldMarkersStillParse) {
  {
    QJsonObject obj;
    obj["schema_version"] = 2;
    obj["min_reader_version"] = 2;
    obj["profile"] = "GpgFrontend";
    obj["last_writer_version"] = "2.1.6";
    obj["last_writer_stable"] = true;

    QFile f(marker_path_);
    ASSERT_TRUE(f.open(QIODevice::WriteOnly));
    f.write(QJsonDocument(obj).toJson());
  }

  const auto marker = ReadProfileMarker(marker_path_);
  ASSERT_TRUE(marker.has_value());
  EXPECT_EQ(marker->schema_version, 2);
  EXPECT_EQ(marker->last_writer_version, QString("2.1.6"));

  // everything new simply defaults; an existing installation needs no
  // conversion pass to keep working
  EXPECT_TRUE(marker->profile_uuid.isEmpty());
  EXPECT_TRUE(marker->migrations.isEmpty());
  EXPECT_FALSE(marker->self_contained);

  // and it needs no migration either
  EXPECT_EQ(PlanProfileMigration(*marker, true, 2, {}).verdict,
            ProfileMigrationVerdict::kNONE);
}

TEST_F(ProfileMigrationRunTest, TheFullMarkerRoundTrips) {
  ProfileMarker marker = MakeMarker(2, 2);
  marker.profile_uuid = "6b1e0000";
  marker.profile_id = "work";
  marker.display_name = "Work";
  marker.created = kNow;
  marker.created_by_version = "2.2.2";
  marker.kind = "named";
  marker.package_id = "9f3c";
  marker.credential_account = "acct";
  marker.self_contained = true;

  ProfileMigrationRecord record;
  record.from = 1;
  record.to = 2;
  record.name = "a_rung";
  record.at = kNow;
  record.by = "2.2.2";
  marker.migrations.append(record);

  ASSERT_TRUE(WriteProfileMarker(marker_path_, marker));
  const auto back = ReadProfileMarker(marker_path_);
  ASSERT_TRUE(back.has_value());

  EXPECT_EQ(back->profile_uuid, marker.profile_uuid);
  EXPECT_EQ(back->profile_id, marker.profile_id);
  EXPECT_EQ(back->display_name, marker.display_name);
  EXPECT_EQ(back->package_id, marker.package_id);
  EXPECT_EQ(back->credential_account, marker.credential_account);
  EXPECT_TRUE(back->self_contained);
  ASSERT_EQ(back->migrations.size(), 1);
  EXPECT_EQ(back->migrations[0].name, QString("a_rung"));
}

}  // namespace GpgFrontend::Test
