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

#include <QDir>
#include <QFile>
#include <QLockFile>
#include <QTemporaryDir>

#include "core/function/ProfileLock.h"

namespace GpgFrontend::Test {

// Acquire()/Release() are deliberately not exercised here: the lock is
// process-wide and this test binary is a running GpgFrontend that already holds
// one, so an Acquire() in a test would report "already ours" and assert
// nothing. Probe() takes no process-wide state and is what the UI actually asks
// before offering to open a profile, so that is what is tested.

TEST(ProfileLockTest, AFreeRootProbesAsFree) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  EXPECT_TRUE(ProfileLock::Probe(dir.path()).Ok());
}

TEST(ProfileLockTest, ARootThatDoesNotExistProbesAsFree) {
  // nothing can be holding a profile that was never created; the alternative
  // would be reporting every not-yet-created profile as busy
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  EXPECT_TRUE(ProfileLock::Probe(dir.path() + "/never-created").Ok());
}

TEST(ProfileLockTest, ProbingLeavesNothingBehind) {
  // Probe() works by taking the lock and giving it straight back. If it left
  // the file behind, the next probe of the same root would report it busy and
  // the profile could never be opened again.
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  ASSERT_TRUE(ProfileLock::Probe(dir.path()).Ok());

  EXPECT_FALSE(QFileInfo::exists(ProfileLock::PathFor(dir.path())));
  EXPECT_TRUE(ProfileLock::Probe(dir.path()).Ok());
}

TEST(ProfileLockTest, AHeldRootIsReportedWithItsHolder) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  QLockFile holder(ProfileLock::PathFor(dir.path()));
  ASSERT_TRUE(holder.tryLock(0));

  const auto result = ProfileLock::Probe(dir.path());
  EXPECT_FALSE(result.Ok());
  EXPECT_EQ(result.status, ProfileLockStatus::kHELD_ELSEWHERE);

  // pid and host are the whole reason for probing rather than checking whether
  // the file exists: they are what the refusal message is built from
  EXPECT_EQ(result.pid, QCoreApplication::applicationPid());
  EXPECT_FALSE(result.host.isEmpty());

  holder.unlock();
  EXPECT_TRUE(ProfileLock::Probe(dir.path()).Ok());
}

TEST(ProfileLockTest, ALockFileLeftByADeadProcessIsNotAHolder) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  // What a crashed process leaves behind: a lock file naming a pid that is not
  // running. Reporting it as busy would strand the profile forever.
  //
  // Aged deliberately. A lock is only stale once it is older than the stale
  // timeout, so a file written a moment ago counts as live no matter which pid
  // it names — which is also why a real crash keeps the profile shut for that
  // long, and why the probe is not a substitute for the launched process
  // taking the lock itself.
  const auto path = ProfileLock::PathFor(dir.path());
  {
    QFile stale(path);
    ASSERT_TRUE(stale.open(QIODevice::WriteOnly));
    stale.write("2147483646\nghost\nghost\n");
  }

  // Aged after the handle is closed: closing flushes, and the flush would put
  // the modification time back to now.
  {
    QFile aged(path);
    ASSERT_TRUE(aged.open(QIODevice::ReadWrite));
    ASSERT_TRUE(aged.setFileTime(QDateTime::currentDateTime().addSecs(-600),
                                 QFileDevice::FileModificationTime));
  }

  EXPECT_TRUE(ProfileLock::Probe(dir.path()).Ok());
}

TEST(ProfileLockTest, TheLockLivesInsideTheRootItProtects) {
  EXPECT_EQ(ProfileLock::PathFor("/srv/p/work"), "/srv/p/work/profile.lock");
}

}  // namespace GpgFrontend::Test
