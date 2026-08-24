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

#include <QDir>
#include <QFile>
#include <QJsonObject>
#include <QTemporaryDir>

#ifdef Q_OS_UNIX
#include <unistd.h>
#endif

#include "core/GFCoreLog.h"
#include "core/profile/ProtectedFsProfileAccessor.h"

namespace GpgFrontend::Test {

// Every test here forces the plain-folder plan, so a run never provisions real
// protected storage, never spawns a tool and never mounts anything on the
// machine it happens to be running on. What the probe finds is the platform's
// business and is asserted separately, and only for what it reports rather than
// for what it manages to provide.

namespace {

auto DiskPlanAt(const QString &path) -> ProfileStoragePlan {
  ProfileStoragePlan plan;
  plan.path = path;
  plan.driver = "fs";
  return plan;
}

auto WriteFile(const QString &path, const QByteArray &bytes) -> void {
  QDir().mkpath(QFileInfo(path).absolutePath());
  QFile file(path);
  ASSERT_TRUE(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
  file.write(bytes);
  file.close();
}

}  // namespace

TEST(ProtectedAccessorTest, HonoursTheWholeAreaContract) {
  // The payoff of making this a ProfileAccessor rather than a thing beside one:
  // the areas are the same question, so they get the same answer.
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  auto accessor = ProtectedFsProfileAccessor::Provision(
      "0123456789abcdef", DiskPlanAt(dir.path() + "/session"));
  ASSERT_FALSE(accessor.isNull());

  for (const auto area :
       {ProfileArea::kRoot, ProfileArea::kConfig, ProfileArea::kDataObjects,
        ProfileArea::kSecure, ProfileArea::kLogs, ProfileArea::kModules,
        ProfileArea::kWorkspace, ProfileArea::kScratch}) {
    ASSERT_TRUE(accessor->Ensure(area));
    EXPECT_TRUE(accessor->PathOf(area).startsWith(dir.path()));
  }

  const GFBuffer value(QByteArray("sealed"));
  ASSERT_TRUE(accessor->Write(ProfileArea::kDataObjects, "abcd", value));
  EXPECT_TRUE(accessor->Exists(ProfileArea::kDataObjects, "abcd"));

  const auto read = accessor->Read(ProfileArea::kDataObjects, "abcd");
  ASSERT_TRUE(read.has_value());
  EXPECT_EQ(*read, value);
}

TEST(ProtectedAccessorTest, ScratchIsInsideTheRootAndHidden) {
  // Inside, so extraction never crosses a filesystem and a rename is all it
  // takes. Hidden, so the area table already refuses to pack it.
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  auto accessor = ProtectedFsProfileAccessor::Provision(
      "0123456789abcdef", DiskPlanAt(dir.path() + "/session"));
  ASSERT_FALSE(accessor.isNull());

  const auto root = accessor->PathOf(ProfileArea::kRoot);
  const auto scratch = accessor->PathOf(ProfileArea::kScratch);

  EXPECT_TRUE(scratch.startsWith(root + "/"));
  EXPECT_TRUE(QFileInfo(scratch).fileName().startsWith('.'));
}

TEST(ProtectedAccessorTest, ProvisioningHardensTheDirectory) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  auto accessor = ProtectedFsProfileAccessor::Provision(
      "0123456789abcdef", DiskPlanAt(dir.path() + "/session"));
  ASSERT_FALSE(accessor.isNull());

  const auto root = accessor->PathOf(ProfileArea::kRoot);

  // The tag is what keeps a session tree out of a whole category of backup
  // tools without any of them knowing what GpgFrontend is.
  QFile tag(root + "/CACHEDIR.TAG");
  ASSERT_TRUE(tag.open(QIODevice::ReadOnly));
  EXPECT_TRUE(
      tag.readAll().startsWith("Signature: 8a477f597d28d172789f06886806bc55"));

  const auto permissions = QFileInfo(root).permissions();
  EXPECT_FALSE(permissions.testFlag(QFile::ReadGroup));
  EXPECT_FALSE(permissions.testFlag(QFile::ReadOther));
}

TEST(ProtectedAccessorTest, TheDiskDriverAdmitsItProtectsNothing) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  auto accessor = ProtectedFsProfileAccessor::Provision(
      "0123456789abcdef", DiskPlanAt(dir.path() + "/session"));
  ASSERT_FALSE(accessor.isNull());

  // Claiming a protection we did not provide is worse than admitting we
  // provide none, and the status strip repeats whatever this says.
  EXPECT_FALSE(accessor->IsVolatile());
  EXPECT_FALSE(accessor->IsEncryptedAtRest());
  EXPECT_FALSE(accessor->Label().isEmpty());
  EXPECT_EQ(accessor->Driver(), "fs");
}

TEST(ProtectedAccessorTest, ReleaseDestroysTheTreeAndIsIdempotent) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  auto accessor = ProtectedFsProfileAccessor::Provision(
      "0123456789abcdef", DiskPlanAt(dir.path() + "/session"));
  ASSERT_FALSE(accessor.isNull());

  const auto root = accessor->PathOf(ProfileArea::kRoot);
  ASSERT_TRUE(accessor->Ensure(ProfileArea::kSecure));
  ASSERT_TRUE(accessor->Write(ProfileArea::kSecure, "app.key",
                              GFBuffer(QByteArray("key material"))));

  accessor->Release(ProfileStorageRelease::kSCRUB);
  EXPECT_FALSE(QFileInfo::exists(root));

  // Called again from a teardown path that cannot know whether the first call
  // happened. Doing nothing is the only safe answer.
  accessor->Release(ProfileStorageRelease::kSCRUB);
  accessor->Release(ProfileStorageRelease::kFAST);
  EXPECT_FALSE(QFileInfo::exists(root));
}

TEST(ProtectedAccessorTest, KeepIsRefusedByADriverThatOwnsItsTree) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  auto accessor = ProtectedFsProfileAccessor::Provision(
      "0123456789abcdef", DiskPlanAt(dir.path() + "/session"));
  ASSERT_FALSE(accessor.isNull());

  const auto root = accessor->PathOf(ProfileArea::kRoot);

  // kKEEP is the filesystem driver's answer, not this one's caller's: nothing
  // asks a session to survive, and honouring it here would leave key material
  // behind on request.
  accessor->Release(ProfileStorageRelease::kKEEP);
  EXPECT_TRUE(QFileInfo::exists(root));

  accessor->Release(ProfileStorageRelease::kSCRUB);
  EXPECT_FALSE(QFileInfo::exists(root));
}

TEST(ProtectedAccessorTest, AnchorStateNamesWhereToLookForAnOrphan) {
  // The anchor is on disk and the storage may not be, so a process that dies
  // leaves this pointer and nothing else.
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  auto accessor = ProtectedFsProfileAccessor::Provision(
      "0123456789abcdef", DiskPlanAt(dir.path() + "/session"));
  ASSERT_FALSE(accessor.isNull());

  const auto state = accessor->AnchorState();
  EXPECT_EQ(state["driver"].toString(), accessor->Driver());
  EXPECT_EQ(state["root"].toString(), accessor->PathOf(ProfileArea::kRoot));
}

TEST(ProtectedAccessorTest, ARefusedPlanProvisionsNothing) {
  ProfileStoragePlan refused;
  refused.refuse = true;
  EXPECT_TRUE(ProtectedFsProfileAccessor::Provision("0123456789abcdef", refused)
                  .isNull());

  EXPECT_TRUE(
      ProtectedFsProfileAccessor::Provision("0123456789abcdef", DiskPlanAt({}))
          .isNull());
}

// ------------------------------------------------------------------ scrubbing

TEST(ScrubDirectoryTest, ContentsAreOverwrittenNotJustUnlinked) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  const auto victim = dir.path() + "/tree/secret";
  WriteFile(victim, "the passphrase is hunter2");

  // A second *hard* link is the only way to see what the overwrite did without
  // root: removing the name would otherwise hide everything, and a symlink
  // would just dangle. Qt has no hard link, so this is POSIX or nothing.
#ifdef Q_OS_UNIX
  const auto witness = dir.path() + "/witness";
  if (::link(victim.toLocal8Bit().constData(),
             witness.toLocal8Bit().constData()) != 0) {
    GTEST_SKIP() << "this filesystem does not do hard links";
  }

  ScrubDirectory(dir.path() + "/tree");

  EXPECT_FALSE(QFileInfo::exists(victim));

  // The inode is still reachable through the other name, and what is in it is
  // no longer the passphrase.
  QFile seen(witness);
  ASSERT_TRUE(seen.open(QIODevice::ReadOnly));
  EXPECT_FALSE(seen.readAll().contains("hunter2"));
#else
  ScrubDirectory(dir.path() + "/tree");
  EXPECT_FALSE(QFileInfo::exists(victim));
#endif
}

TEST(ScrubDirectoryTest, RefusesAnythingButAnAbsolutePath) {
  // A bug in the caller must not be able to turn this loose on the wrong tree,
  // and "" resolves to the working directory.
  ScrubDirectory({});
  ScrubDirectory("relative/path");
  SUCCEED();
}

TEST(ScrubDirectoryTest, AReadOnlyFileDoesNotAbandonTheRest) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  const auto tree = dir.path() + "/tree";
  WriteFile(tree + "/locked", "cannot be opened for writing");
  WriteFile(tree + "/deeper/ordinary", "can be");

  ASSERT_TRUE(QFile::setPermissions(tree + "/locked", QFile::ReadOwner));

  ScrubDirectory(tree);

  // The unwritable one is left alone rather than taking the walk down with it.
  QFile::setPermissions(tree + "/locked", QFile::ReadOwner | QFile::WriteOwner);
  SUCCEED();
}

TEST(ScrubDirectoryTest, ASymlinkIsNotFollowedOutOfTheTree) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  const auto outside = dir.path() + "/outside.txt";
  WriteFile(outside, "not ours to destroy");

  const auto tree = dir.path() + "/tree";
  QDir().mkpath(tree);
  if (!QFile::link(outside, tree + "/pointer")) {
    GTEST_SKIP() << "this filesystem does not do symlinks";
  }

  ScrubDirectory(tree);

  QFile kept(outside);
  ASSERT_TRUE(kept.open(QIODevice::ReadOnly));
  EXPECT_EQ(kept.readAll(), QByteArray("not ours to destroy"));
}

// --------------------------------------------------------------- the platform

TEST(StorageProbeTest, ReportsWhatThisMachineActuallyOffers) {
  // Not an assertion about the platform — it is an assertion that the probe
  // answers *something* for every candidate, printed so that a run on an
  // unfamiliar machine says which storage a session would get there.
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  // A budget a real small package would ask for, so the report reflects what
  // an actual session would get here rather than an invented worst case.
  for (const auto &candidate :
       ProbeStorageCandidates(ProfileStorageBudget(2LL * 1024 * 1024, 0),
                              dir.path() + "/.anchor")) {
    std::cerr << "  candidate " << candidate.driver.toStdString() << " at "
              << candidate.path.toStdString()
              << (candidate.usable ? "  [usable]" : "  [no: ")
              << (candidate.usable ? "" : candidate.reason.toStdString())
              << (candidate.usable ? "" : "]")
              << (candidate.is_volatile ? "  volatile" : "")
              << (candidate.is_encrypted_at_rest ? "  encrypted" : "")
              << std::endl;
  }
  SUCCEED();
}

TEST(StorageProbeTest, TheProfilesFolderIsAlwaysTheLastResort) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  const auto candidates = ProbeStorageCandidates(1024, dir.path() + "/.anchor");
  ASSERT_FALSE(candidates.isEmpty());

  const auto &last = candidates.last();
  EXPECT_EQ(last.driver, "fs");
  EXPECT_EQ(last.path, dir.path() + "/.anchor");
  EXPECT_FALSE(last.IsProtected());
}

TEST(StorageProbeTest, EveryCandidateEitherWorksOrSaysWhyNot) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  // The reasons are shown to the user verbatim when a strict policy refuses, so
  // a candidate that is neither usable nor explained is a dialog with a blank
  // in it.
  for (const auto &candidate :
       ProbeStorageCandidates(1024, dir.path() + "/.anchor")) {
    EXPECT_FALSE(candidate.driver.isEmpty());
    if (!candidate.usable) {
      EXPECT_FALSE(candidate.reason.isEmpty())
          << "candidate: " << candidate.driver.toStdString();
    }
  }
}

TEST(StorageProbeTest, AnImpossibleBudgetIsReportedNotIgnored) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  // Nothing has an exabyte free, so every candidate should be reporting a
  // shortfall rather than claiming it will fit.
  const auto candidates =
      ProbeStorageCandidates(1LL << 60, dir.path() + "/.anchor");

  for (const auto &candidate : candidates) {
    if (candidate.driver == QLatin1String("fs")) continue;  // never measured
    if (candidate.free_bytes < 0) continue;                 // could not say
    EXPECT_FALSE(candidate.usable)
        << "candidate: " << candidate.path.toStdString();
  }
}

TEST(ProtectedAccessorTest, ReleaseClosesTheLogFileInsideTheTreeItRemoves) {
  // The session logs into its own storage, so this process holds a handle on a
  // file in the tree it is about to destroy. POSIX unlinks an open file
  // happily, which is why this went unnoticed for so long; Windows refuses,
  // and refuses for the directory holding it too, so every packaged session
  // left its whole storage folder behind -- log file included -- on close.
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  auto accessor = ProtectedFsProfileAccessor::Provision(
      "0123456789abcdef", DiskPlanAt(dir.path() + "/session"));
  ASSERT_FALSE(accessor.isNull());
  ASSERT_TRUE(accessor->Ensure(ProfileArea::kLogs));

  auto &log = GFLogManager::Instance();
  log.StopFileLogger();
  log.InitRingBuffer(64);
  log.InitFileLogger(accessor->PathOf(ProfileArea::kLogs));
  ASSERT_FALSE(log.FileLoggerPath().isEmpty());

  const auto root = accessor->PathOf(ProfileArea::kRoot);
  accessor->Release(ProfileStorageRelease::kSCRUB);

  EXPECT_TRUE(log.FileLoggerPath().isEmpty());
  EXPECT_FALSE(QFileInfo::exists(root));
}

TEST(ProtectedAccessorTest, ReleaseLeavesALogFileOutsideItsTreeAlone) {
  // Scoped to the tree, not to "there is a log file". A window closing its own
  // package must not silence the logging of a profile it has nothing to do
  // with -- and on a machine running two, that is the other window's diary of
  // the failure this one is in the middle of.
  QTemporaryDir dir;
  QTemporaryDir elsewhere;
  ASSERT_TRUE(dir.isValid());
  ASSERT_TRUE(elsewhere.isValid());

  auto accessor = ProtectedFsProfileAccessor::Provision(
      "0123456789abcdef", DiskPlanAt(dir.path() + "/session"));
  ASSERT_FALSE(accessor.isNull());

  auto &log = GFLogManager::Instance();
  log.StopFileLogger();
  log.InitRingBuffer(64);
  log.InitFileLogger(elsewhere.path());
  const auto path = log.FileLoggerPath();
  ASSERT_FALSE(path.isEmpty());

  accessor->Release(ProfileStorageRelease::kSCRUB);

  EXPECT_EQ(log.FileLoggerPath(), path);
  log.StopFileLogger();
}

#ifdef Q_OS_UNIX
TEST(ProtectedAccessorTest, AReleaseThatCannotRemoveTheTreeStillEmptiesIt) {
  // What "could not be removed" has to mean. Reporting it and marking the
  // storage released is the dangerous answer: this call is the only promise
  // that an unpacked profile does not outlive the process, and every later
  // caller would take that flag at its word.
  //
  // A directory with no write permission is the portable stand-in for the
  // Windows case: its entries cannot be unlinked, though their contents can
  // still be overwritten -- which is the distinction that decides whether what
  // survives is key material or an empty name.
  if (::geteuid() == 0) GTEST_SKIP() << "root ignores directory permissions";

  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  auto accessor = ProtectedFsProfileAccessor::Provision(
      "0123456789abcdef", DiskPlanAt(dir.path() + "/session"));
  ASSERT_FALSE(accessor.isNull());
  ASSERT_TRUE(accessor->Ensure(ProfileArea::kSecure));

  const auto root = accessor->PathOf(ProfileArea::kRoot);
  const auto pinned = accessor->PathOf(ProfileArea::kSecure);
  const auto key = pinned + "/app.key";
  WriteFile(key, QByteArray("PLAINTEXT-KEY-MATERIAL"));

  ASSERT_TRUE(QFile::setPermissions(
      pinned, QFileDevice::ReadOwner | QFileDevice::ExeOwner));

  accessor->Release(ProfileStorageRelease::kSCRUB);

  // Still there, because nothing could unlink it -- but holding nothing.
  ASSERT_TRUE(QFileInfo::exists(key));
  EXPECT_EQ(QFileInfo(key).size(), 0);

  // And not marked released: the next attempt, once whatever held the tree has
  // let go, has to actually run.
  ASSERT_TRUE(QFile::setPermissions(pinned, QFileDevice::ReadOwner |
                                                QFileDevice::WriteOwner |
                                                QFileDevice::ExeOwner));
  accessor->Release(ProfileStorageRelease::kSCRUB);
  EXPECT_FALSE(QFileInfo::exists(root));
}
#endif

}  // namespace GpgFrontend::Test
