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

#include "core/profile/FscryptStorage.h"
#include "core/profile/ProfilePackage.h"
#include "core/profile/ProfileStoragePlan.h"
#include "core/profile/ProtectedFsProfileAccessor.h"

namespace GpgFrontend::Test {

// Two tiers here, and the split matters.
//
// Everything up to the live suite is pure or fails closed: it asserts what the
// code decides, never what the machine underneath happens to offer. This
// container is overlayfs, so fscrypt is genuinely unavailable on it — which is
// not a gap in the coverage but the most important case, because it is the case
// on nearly every machine this will ever run on, and the thing being tested is
// that the fallback is honest rather than silent.
//
// The live suite provisions real encrypted storage and is skipped unless
// GF_FSCRYPT_TEST_DIR names a directory on a filesystem that can carry a
// policy. Nothing in the default run mounts anything or adds a key.

namespace {

constexpr auto kLiveDirEnv = "GF_FSCRYPT_TEST_DIR";

auto Identifier(char fill) -> QByteArray {
  return QByteArray(kFscryptKeyIdentifierSize, fill);
}

}  // namespace

// ------------------------------------------------------------- the identifier

TEST(FscryptStorageTest, AnIdentifierSurvivesTheAnchorPointer) {
  // The round trip the sweep depends on: what Release() could not finish, a
  // later process finishes from this string and nothing else.
  const auto id = Identifier('\x7f');
  const auto hex = FscryptIdentifierToHex(id);

  EXPECT_EQ(hex.size(), kFscryptKeyIdentifierSize * 2);
  EXPECT_EQ(FscryptIdentifierFromHex(hex), id);
}

TEST(FscryptStorageTest, OnlyARealIdentifierIsSpelled) {
  // A short one would be spelled as a valid-looking string that names no key.
  EXPECT_TRUE(FscryptIdentifierToHex({}).isEmpty());
  EXPECT_TRUE(FscryptIdentifierToHex(QByteArray(8, '\x01')).isEmpty());
  EXPECT_TRUE(FscryptIdentifierToHex(QByteArray(32, '\x01')).isEmpty());
}

TEST(FscryptStorageTest, AnythingThatIsNotAnIdentifierReadsAsNone) {
  // The pointer is a file on disk that this process did not necessarily write,
  // and what is done with the result is an eviction. Anything that is not
  // exactly one identifier has to read as none rather than as something to try.
  EXPECT_TRUE(FscryptIdentifierFromHex({}).isEmpty());
  EXPECT_TRUE(FscryptIdentifierFromHex("00").isEmpty());
  EXPECT_TRUE(FscryptIdentifierFromHex(QString(31, '0')).isEmpty());
  EXPECT_TRUE(FscryptIdentifierFromHex(QString(33, '0')).isEmpty());
  EXPECT_TRUE(FscryptIdentifierFromHex(QString(64, '0')).isEmpty());

  // fromHex() skips what it cannot read rather than refusing it, so a value of
  // the right length made of the wrong characters would otherwise come back
  // short instead of empty.
  EXPECT_TRUE(FscryptIdentifierFromHex(QString(32, 'z')).isEmpty());
  EXPECT_TRUE(FscryptIdentifierFromHex(QString(31, '0') + "z").isEmpty());
}

// ----------------------------------------------------------- the search paths

TEST(FscryptStorageTest, EncryptableSearchPathsAreOrderedMostSpecificFirst) {
  // The override first for the same reason it is first everywhere else, then
  // the temporary folder the unprotected rung would have used, then /var/tmp.
  const auto paths = EncryptableStoreSearchPaths("/mnt/enc", "/tmp", 1000);

  ASSERT_EQ(paths.size(), 3);
  EXPECT_EQ(paths.at(0), "/mnt/enc");
  EXPECT_EQ(paths.at(1), "/tmp/gpgfrontend-1000");
  EXPECT_EQ(paths.at(2), "/var/tmp/gpgfrontend-1000");
}

TEST(FscryptStorageTest, VarTmpIsAlwaysThere) {
  // Because /tmp is a tmpfs on most systemd distributions and a tmpfs cannot
  // carry a policy at all — so without this entry the rung would almost never
  // fire on exactly the machines it exists for.
  const auto paths = EncryptableStoreSearchPaths({}, {}, 501);

  ASSERT_EQ(paths.size(), 1);
  EXPECT_EQ(paths.at(0), "/var/tmp/gpgfrontend-501");
}

TEST(FscryptStorageTest, EncryptableSearchPathsDeduplicate) {
  const auto paths =
      EncryptableStoreSearchPaths("/var/tmp/gpgfrontend-1000", {}, 1000);

  ASSERT_EQ(paths.size(), 1);
  EXPECT_EQ(paths.at(0), "/var/tmp/gpgfrontend-1000");
}

TEST(FscryptStorageTest, EncryptableSearchPathsIgnoreRelativeInput) {
  // A relative value would name whatever directory the process happens to sit
  // in, which for a GUI launched from a file manager is anyone's guess.
  const auto paths = EncryptableStoreSearchPaths("../enc", "tmp", 1000);

  ASSERT_EQ(paths.size(), 1);
  EXPECT_EQ(paths.at(0), "/var/tmp/gpgfrontend-1000");
}

TEST(FscryptStorageTest, EncryptableSearchPathsCleanTheirInput) {
  const auto paths = EncryptableStoreSearchPaths("/mnt/enc/", "/tmp/", 1000);
  EXPECT_EQ(paths.at(0), "/mnt/enc");
  EXPECT_EQ(paths.at(1), "/tmp/gpgfrontend-1000");
}

TEST(FscryptStorageTest, CapableMagicAcceptsOnlyTheFilesystemsThatEncrypt) {
  EXPECT_TRUE(IsFscryptCapableMagic(0xEF53));      // ext4
  EXPECT_TRUE(IsFscryptCapableMagic(0xF2F52010));  // f2fs

  // A tmpfs is where a temporary folder usually lives and is exactly what this
  // has to refuse: it is memory-backed, which is a different protection, and it
  // cannot carry a policy.
  EXPECT_FALSE(IsFscryptCapableMagic(0x01021994));  // tmpfs
  EXPECT_FALSE(IsFscryptCapableMagic(0x858458f6));  // ramfs
  EXPECT_FALSE(IsFscryptCapableMagic(0x794c7630));  // overlayfs
  EXPECT_FALSE(IsFscryptCapableMagic(0));
}

// ------------------------------------------------------------- the honest "no"

TEST(FscryptStorageTest, ADirectoryThatCannotEncryptSaysWhy) {
  // A temporary directory in this container is on overlayfs, which cannot
  // encrypt. The contract is not that this fails — it is that it says why, in
  // words that end up in a refusal dialog.
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  QString reason;
  if (FscryptDirectoryIsUsable(dir.path(), reason)) {
    // A machine that really can encrypt its temporary directory. Nothing was
    // provisioned, so there is nothing to clean up.
    EXPECT_TRUE(reason.isEmpty());
    SUCCEED();
    return;
  }

  EXPECT_FALSE(reason.isEmpty());
}

TEST(FscryptStorageTest,
     ProvisioningAPathThatIsNotThereRefusesRatherThanCrash) {
  QByteArray id;
  QString reason;

  EXPECT_FALSE(
      FscryptProvisionDirectory("/nonexistent/gf-fscrypt-test", id, reason));
  EXPECT_TRUE(id.isEmpty());
  EXPECT_FALSE(reason.isEmpty());
}

TEST(FscryptStorageTest, RemovingAKeyThatIsNotAnIdentifierIsRefused) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  QString reason;
  EXPECT_FALSE(FscryptRemoveKey(dir.path(), {}, reason));
  EXPECT_FALSE(FscryptRemoveKey(dir.path(), QByteArray(8, '\x01'), reason));
  EXPECT_FALSE(reason.isEmpty());
}

TEST(ProtectedAccessorTest, FscryptDowngradesHonestlyWhenTheKernelSaysNo) {
  // The case on nearly every machine, and the one that must never lie. A plan
  // that asked for encryption and did not get it comes back as an ordinary
  // temporary folder, says so through IsEncryptedAtRest(), and leaves no key in
  // the anchor for a sweep to chase.
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  ProfileStoragePlan plan;
  plan.path = dir.path() + "/base";
  plan.driver = "fs-fscrypt";
  plan.is_encrypted_at_rest = true;
  ASSERT_TRUE(QDir().mkpath(plan.path));

  auto accessor =
      ProtectedFsProfileAccessor::Provision("0123456789abcdef", plan);
  ASSERT_FALSE(accessor.isNull());

  QString reason;
  const auto encrypted = FscryptDirectoryIsUsable(plan.path, reason);

  if (!encrypted) {
    EXPECT_EQ(accessor->Driver(), "fs-temp");
    EXPECT_FALSE(accessor->IsEncryptedAtRest());
    EXPECT_FALSE(accessor->AnchorState().contains("fscrypt_key"));
  } else {
    // A machine that really can. Then the claim must be backed by a key.
    EXPECT_EQ(accessor->Driver(), "fs-fscrypt");
    EXPECT_TRUE(accessor->IsEncryptedAtRest());
    EXPECT_TRUE(accessor->AnchorState().contains("fscrypt_key"));
  }

  EXPECT_FALSE(accessor->IsVolatile());
  accessor->Release(ProfileStorageRelease::kSCRUB);
}

// ------------------------------------------------------------------ the sweep

TEST(FscryptStorageTest, AStrandedPointerWithoutAKeyStillRemovesTheTree) {
  // The pointer every non-encrypted driver writes, and the behaviour that was
  // there before any of this.
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  const auto stranded = dir.path() + "/stranded";
  ASSERT_TRUE(QDir().mkpath(stranded));

  QJsonObject pointer;
  pointer["driver"] = "fs-temp";
  pointer["root"] = stranded;

  EXPECT_TRUE(ReleaseStrandedSessionStorage(pointer, dir.path() + "/anchor"));
  EXPECT_FALSE(QFileInfo::exists(stranded));
}

TEST(FscryptStorageTest, AnEmptyPointerNamesNothingAndTouchesNothing) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  EXPECT_TRUE(ReleaseStrandedSessionStorage({}, dir.path()));
  EXPECT_TRUE(QFileInfo::exists(dir.path()));
}

TEST(FscryptStorageTest, TheAnchorItselfIsLeftForTheCallerToRemove) {
  // The plain-folder driver's root *is* the anchor. Removing it here would have
  // the sweep report a failure for work it had already done.
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  const auto anchor = dir.path() + "/anchor";
  ASSERT_TRUE(QDir().mkpath(anchor));

  QJsonObject pointer;
  pointer["driver"] = "fs";
  pointer["root"] = anchor;

  EXPECT_TRUE(ReleaseStrandedSessionStorage(pointer, anchor));
  EXPECT_TRUE(QFileInfo::exists(anchor));
}

TEST(FscryptStorageTest, ARelativeStrandedRootIsNeverFollowed) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  const auto kept = dir.path() + "/kept";
  ASSERT_TRUE(QDir().mkpath(kept));

  QJsonObject pointer;
  pointer["driver"] = "fs-temp";
  pointer["root"] = "kept";

  ReleaseStrandedSessionStorage(pointer, dir.path() + "/anchor");
  EXPECT_TRUE(QFileInfo::exists(kept));
}

TEST(FscryptStorageTest, ACorruptKeyInAPointerIsNotActedOn) {
  // The tree still goes; the unreadable key spelling is simply not a key.
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  const auto stranded = dir.path() + "/stranded";
  ASSERT_TRUE(QDir().mkpath(stranded));

  QJsonObject pointer;
  pointer["driver"] = "fs-fscrypt";
  pointer["root"] = stranded;
  pointer["base"] = dir.path();
  pointer["fscrypt_key"] = "not a key";

  EXPECT_TRUE(ReleaseStrandedSessionStorage(pointer, dir.path() + "/anchor"));
  EXPECT_FALSE(QFileInfo::exists(stranded));
}

// ----------------------------------------------------- the sweep's own record

TEST(FscryptStorageTest, AnAnchorOutlivesAReleaseThatCouldNotFinish) {
  // The pointer is the only record of what a dead session left behind, and for
  // an encrypted driver one of those things -- a key in the kernel -- cannot be
  // found by looking around: no filesystem names it. So an anchor removed after
  // a release that did not finish takes the last thing that could name that key
  // with it.
  //
  // Forced here through the tree rather than the key, because that is the half
  // a machine without fscrypt can still fail at, and it is the same branch: a
  // directory whose parent this process cannot write to cannot be removed.
  if (::geteuid() == 0)
    GTEST_SKIP() << "root ignores the permissions this uses";

  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  const auto locked = dir.path() + "/locked";
  const auto stranded = locked + "/gf-abcd";
  ASSERT_TRUE(QDir().mkpath(stranded));

  const auto anchor = dir.path() + "/.abcd";
  QJsonObject state;
  state["driver"] = "fs-fscrypt";
  state["root"] = stranded;
  state["base"] = locked;
  state["fscrypt_key"] = QString(kFscryptKeyIdentifierSize * 2, 'a');
  ASSERT_TRUE(WriteSessionPointer(anchor, state));

  ASSERT_TRUE(QFile::setPermissions(
      locked, QFileDevice::ReadOwner | QFileDevice::ExeOwner));

  const auto swept = SweepTransientProfileRoots(dir.path(), {});

  // Restored first, so that a failed expectation below cannot leave a directory
  // the harness is unable to clean up.
  QFile::setPermissions(locked, QFileDevice::ReadOwner |
                                    QFileDevice::WriteOwner |
                                    QFileDevice::ExeOwner);

  EXPECT_EQ(swept, 0);
  EXPECT_TRUE(QFileInfo::exists(anchor));
  EXPECT_FALSE(ReadSessionPointer(anchor).isEmpty())
      << "the pointer is the only thing that can name what was left behind";
}

TEST(FscryptStorageTest, AKeyOnAFilesystemThatIsGoneDoesNotPinTheAnchor) {
  // The other side of it. A key exists only in a running kernel's keyring for
  // one superblock, so a base that is not there names nothing that could still
  // be evicted -- and a later sweep would be handed the same pointer and reach
  // the same dead end. Keeping the anchor for that retry would keep it forever.
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  const auto anchor = dir.path() + "/.abcd";

  QJsonObject state;
  state["driver"] = "fs-fscrypt";
  state["root"] = "/nonexistent/gf-abcd";
  state["base"] = "/nonexistent/base";
  state["fscrypt_key"] = QString(kFscryptKeyIdentifierSize * 2, 'b');
  ASSERT_TRUE(WriteSessionPointer(anchor, state));

  EXPECT_EQ(SweepTransientProfileRoots(dir.path(), {}), 1);
  EXPECT_FALSE(QFileInfo::exists(anchor));
}

TEST(FscryptStorageTest, AKeyThatIsNotThereIsReportedAsAlreadyGone) {
  // What lets the sweep be correct on every machine. An identifier that names
  // no key -- because nothing minted one, or because this kernel or filesystem
  // cannot hold one at all -- is gone, not a removal that failed. Only a key
  // that may still be there is worth keeping a pointer for.
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  QString reason;
  EXPECT_TRUE(FscryptRemoveKey(dir.path(), Identifier('\x01'), reason))
      << reason.toStdString();
}

// ------------------------------------------------------------- the platform

TEST(FscryptStorageTest, WhatThisMachineActuallyOffersIsReported) {
  // Not an assertion about the platform — an assertion that every candidate
  // path answers *something*, printed so that a run on an unfamiliar machine
  // says why an encrypted session was or was not available there.
  std::cerr << "  fscrypt available: " << (FscryptAvailable() ? "yes" : "no")
            << std::endl;

  for (const auto &base : EncryptableStoreSearchPaths(
           {}, QDir::tempPath(), static_cast<uint>(::geteuid()))) {
    // Asked of the parent, which exists, rather than of the base, which is
    // created only once a plan has chosen it. That is also the order the probe
    // itself uses, so this reports what a real session would find.
    const auto origin = QFileInfo(base).absolutePath();

    QString reason;
    const auto usable = FscryptDirectoryIsUsable(origin, reason);
    std::cerr << "  base " << base.toStdString() << " (on "
              << origin.toStdString() << ")"
              << (usable ? "  [usable]"
                         : "  [no: " + reason.toStdString() + "]")
              << std::endl;
  }
  SUCCEED();
}

TEST(StorageProbeTest, TheEncryptedRungSitsBetweenMemoryAndPlainTemp) {
  // The ladder: memory, then encrypted, then a plain temporary folder, then the
  // profiles folder. PlanProfileStorage() takes the first usable protected
  // candidate in order, so this order *is* the preference.
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  const auto candidates = ProbeStorageCandidates(1024, dir.path() + "/.anchor");

  auto first_of = [&candidates](const QString &driver) {
    for (int i = 0; i < candidates.size(); ++i) {
      if (candidates.at(i).driver == driver) return i;
    }
    return -1;
  };

  const auto tmpfs = first_of("fs-tmpfs");
  const auto fscrypt = first_of("fs-fscrypt");
  const auto temp = first_of("fs-temp");
  const auto disk = first_of("fs");

  // The two that exist everywhere.
  ASSERT_GE(temp, 0);
  ASSERT_GE(disk, 0);
  EXPECT_LT(temp, disk);

  // The two that do not. Absent is a fine answer on a platform without them;
  // present in the wrong place is not.
  if (fscrypt >= 0) {
    EXPECT_LT(fscrypt, temp);
    if (tmpfs >= 0) {
      EXPECT_LT(tmpfs, fscrypt);
    }
  }
}

// --------------------------------------------------------------------- live

// Everything below provisions real encrypted storage and is skipped unless
// GF_FSCRYPT_TEST_DIR names somewhere that can carry a policy. Create one with:
//
//   truncate -s 512M img && mkfs.ext4 -O encrypt img
//   sudo mount -o loop img /mnt/fsc && sudo chown $USER /mnt/fsc
//   GF_FSCRYPT_TEST_DIR=/mnt/fsc scripts/run_tests.sh -f 'LiveFscryptTest.*'

namespace {

/// A fresh empty directory under the live base, or a skip.
auto LiveDir(const QString &name) -> QString {
  const auto base = qEnvironmentVariable(kLiveDirEnv);
  if (base.isEmpty()) return {};

  const auto path = base + "/gf-fscrypt-test-" + name;
  QDir(path).removeRecursively();
  if (!QDir().mkpath(path)) return {};

  QString reason;
  if (!FscryptDirectoryIsUsable(path, reason)) {
    std::cerr << "  " << kLiveDirEnv
              << " is not usable: " << reason.toStdString() << std::endl;
    QDir(path).removeRecursively();
    return {};
  }

  return path;
}

}  // namespace

TEST(LiveFscryptTest, ProvisionEncryptsAndRemovingTheKeyLocksItOut) {
  const auto dir = LiveDir("roundtrip");
  if (dir.isEmpty()) GTEST_SKIP() << "no usable " << kLiveDirEnv;

  QByteArray id;
  QString reason;
  ASSERT_TRUE(FscryptProvisionDirectory(dir, id, reason))
      << reason.toStdString();
  ASSERT_EQ(id.size(), kFscryptKeyIdentifierSize);

  const auto file = dir + "/secret";
  {
    QFile out(file);
    ASSERT_TRUE(out.open(QIODevice::WriteOnly));
    out.write("app.key");
  }
  {
    QFile in(file);
    ASSERT_TRUE(in.open(QIODevice::ReadOnly));
    EXPECT_EQ(in.readAll(), QByteArray("app.key"));
  }

  ASSERT_TRUE(FscryptRemoveKey(dir, id, reason)) << reason.toStdString();

  // The whole promise: the bytes are still on the medium and this process can
  // no longer read them.
  QFile locked(file);
  EXPECT_FALSE(locked.open(QIODevice::ReadOnly));

  // And the tree can still be destroyed afterwards, which is what makes it safe
  // for Release() to evict the key last.
  EXPECT_TRUE(QDir(dir).removeRecursively());
}

TEST(LiveFscryptTest, APolicyIsRefusedOnADirectoryThatIsNotEmpty) {
  // Pins the ordering in Provision(): the policy has to go on before anything
  // is written, or what is already there stays plaintext.
  const auto dir = LiveDir("nonempty");
  if (dir.isEmpty()) GTEST_SKIP() << "no usable " << kLiveDirEnv;

  QFile squatter(dir + "/already-here");
  ASSERT_TRUE(squatter.open(QIODevice::WriteOnly));
  squatter.close();

  QByteArray id;
  QString reason;
  EXPECT_FALSE(FscryptProvisionDirectory(dir, id, reason));
  EXPECT_TRUE(id.isEmpty());
  EXPECT_FALSE(reason.isEmpty());

  QDir(dir).removeRecursively();
}

TEST(LiveFscryptTest, ADirectoryAlreadyUnderAPolicyIsRefused) {
  const auto dir = LiveDir("nested");
  if (dir.isEmpty()) GTEST_SKIP() << "no usable " << kLiveDirEnv;

  QByteArray id;
  QString reason;
  ASSERT_TRUE(FscryptProvisionDirectory(dir, id, reason))
      << reason.toStdString();

  const auto inner = dir + "/inner";
  ASSERT_TRUE(QDir().mkpath(inner));
  EXPECT_FALSE(FscryptDirectoryIsUsable(inner, reason));
  EXPECT_FALSE(reason.isEmpty());

  QString ignored;
  FscryptRemoveKey(dir, id, ignored);
  QDir(dir).removeRecursively();
}

TEST(LiveFscryptTest, AKeyStillHeldOpenKeepsThePointerThatNamesIt) {
  // The retention case a machine without fscrypt cannot reach: the master
  // secret is dropped but files somebody still holds open keep their derived
  // keys, so the tree is not yet unreadable and the key is not yet gone. That
  // is the one outcome where a later sweep has real work to do, and it can only
  // find it through the pointer.
  const auto base = LiveDir("busy");
  if (base.isEmpty()) GTEST_SKIP() << "no usable " << kLiveDirEnv;

  const auto stranded = base + "/gf-abcd";
  ASSERT_TRUE(QDir().mkpath(stranded));

  QByteArray id;
  QString reason;
  ASSERT_TRUE(FscryptProvisionDirectory(stranded, id, reason))
      << reason.toStdString();

  // Held open across the release, which is what produces FILES_BUSY.
  QFile busy(stranded + "/held");
  ASSERT_TRUE(busy.open(QIODevice::WriteOnly));
  busy.write("still open");
  busy.flush();

  QJsonObject pointer;
  pointer["driver"] = "fs-fscrypt";
  pointer["root"] = stranded;
  pointer["base"] = base;
  pointer["fscrypt_key"] = FscryptIdentifierToHex(id);

  EXPECT_FALSE(ReleaseStrandedSessionStorage(pointer, base + "/.anchor"))
      << "a key still held open is a key a later sweep must be able to find";

  busy.close();

  // And once the handle is gone the retry the pointer was kept for succeeds.
  EXPECT_TRUE(ReleaseStrandedSessionStorage(pointer, base + "/.anchor"));

  QString ignored;
  FscryptRemoveKey(base, id, ignored);
  QDir(base).removeRecursively();
}

TEST(LiveFscryptTest, TheDriverProvisionsAndReleasesRealEncryptedStorage) {
  const auto base = LiveDir("driver");
  if (base.isEmpty()) GTEST_SKIP() << "no usable " << kLiveDirEnv;

  ProfileStoragePlan plan;
  plan.path = base;
  plan.driver = "fs-fscrypt";
  plan.is_encrypted_at_rest = true;

  auto accessor =
      ProtectedFsProfileAccessor::Provision("0123456789abcdef", plan);
  ASSERT_FALSE(accessor.isNull());
  ASSERT_EQ(accessor->Driver(), "fs-fscrypt");
  EXPECT_TRUE(accessor->IsEncryptedAtRest());

  const auto state = accessor->AnchorState();
  EXPECT_EQ(state.value("base").toString(), base);
  EXPECT_EQ(
      FscryptIdentifierFromHex(state.value("fscrypt_key").toString()).size(),
      kFscryptKeyIdentifierSize);

  ASSERT_TRUE(accessor->Ensure(ProfileArea::kDataObjects));
  ASSERT_TRUE(accessor->Write(ProfileArea::kDataObjects, "abcd",
                              GFBuffer(QByteArray("sealed"))));

  const auto root = accessor->PathOf(ProfileArea::kRoot);
  accessor->Release(ProfileStorageRelease::kSCRUB);
  EXPECT_FALSE(QFileInfo::exists(root));

  QDir(base).removeRecursively();
}

}  // namespace GpgFrontend::Test
