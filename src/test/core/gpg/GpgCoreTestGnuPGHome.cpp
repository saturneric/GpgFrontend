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

// Where gpg-agent may put its sockets, and the short links that get a deep key
// database there.

#include "GpgFrontendTest.h"
#include "core/GFCoreTest.h"
#include "core/function/gpg/GnuPGHome.h"
#include "core/module/ModuleManager.h"

namespace GpgFrontend::Test {

// gpg-agent could not create its sockets because the key database path was
// longer than a unix socket address can hold, and nothing checked. These pin
// the arithmetic that now catches it.
TEST_F(GFCoreTest, GnuPGSocketBudgetAcceptsAPathExactlyAtTheBudget) {
  const auto budget = GnuPGSocketBudget::Bytes();
  if (budget < 0) GTEST_SKIP() << "no socket path limit on this platform";

  const QString path = "/" + QString(budget - 1, QLatin1Char('a'));
  ASSERT_EQ(path.toUtf8().size(), budget);

  EXPECT_TRUE(GnuPGSocketBudget::Fits(path));
}

TEST_F(GFCoreTest, GnuPGSocketBudgetRejectsAPathOneByteOverTheBudget) {
  const auto budget = GnuPGSocketBudget::Bytes();
  if (budget < 0) GTEST_SKIP() << "no socket path limit on this platform";

  const QString path = "/" + QString(budget, QLatin1Char('a'));
  ASSERT_EQ(path.toUtf8().size(), budget + 1);

  EXPECT_FALSE(GnuPGSocketBudget::Fits(path));
}

// sun_path is a byte buffer, so a non-ASCII user name costs more room than its
// character count suggests. Measuring QChars would let exactly these paths
// through and reproduce the original failure.
TEST_F(GFCoreTest, GnuPGSocketBudgetIsMeasuredInBytesNotCharacters) {
  const auto budget = GnuPGSocketBudget::Bytes();
  if (budget < 0) GTEST_SKIP() << "no socket path limit on this platform";

  // Each of these is one QChar but two UTF-8 bytes, so a path of budget
  // characters is twice the budget in bytes.
  const QString path = "/" + QString(budget, QChar{0x00E4});
  ASSERT_LT(path.size(), path.toUtf8().size());

  EXPECT_FALSE(GnuPGSocketBudget::Fits(path));
}

// The tests above are all relative to Bytes(), so they cannot catch a wrong
// constant. One was: the budget only reserved the NUL, gpg-agent wants one byte
// more, and an 83-byte ephemeral profile home on macOS passed here and then
// made gpg-agent exit with "socket name ... is too long".
TEST_F(GFCoreTest, GnuPGSocketBudgetMatchesGnuPGsOwnCheck) {
#if defined(Q_OS_WINDOWS)
  EXPECT_EQ(GnuPGSocketBudget::Bytes(), -1);
#elif defined(Q_OS_MACOS)
  EXPECT_EQ(GnuPGSocketBudget::Bytes(), 82);
#else
  EXPECT_EQ(GnuPGSocketBudget::Bytes(), 86);
#endif
}

#ifndef Q_OS_WINDOWS
namespace {

// A directory whose path is exactly `bytes` long, under the short `base`.
auto MakeDirOfLength(const QString& base, int bytes) -> QString {
  const auto pad = bytes - static_cast<int>(base.toUtf8().size()) - 1;
  if (pad < 1) return {};

  const auto path = base + "/" + QString(pad, QLatin1Char('a'));
  return QDir().mkpath(path) ? path : QString{};
}

// gpg-agent checks the socket name before it forks, so the exit code of a
// --daemon start alone says whether the home was short enough.
auto AgentStartsIn(const QString& agent, const QString& gpgconf,
                   const QString& home, QString& output) -> bool {
  QProcess proc;
  proc.setProcessChannelMode(QProcess::MergedChannels);
  proc.start(agent, {"--homedir", home, "--daemon"});
  const auto finished = proc.waitForFinished(10000);
  if (!finished) proc.kill();
  output = QString::fromUtf8(proc.readAll());

  QProcess::execute(gpgconf, {"--homedir", home, "--kill", "gpg-agent"});
  return finished && proc.exitStatus() == QProcess::NormalExit &&
         proc.exitCode() == 0;
}

}  // namespace

// The budget checked against GnuPG itself rather than against a copy of its
// arithmetic: a home exactly at the budget starts, one byte more does not.
TEST_F(GFCoreTest, GnuPGSocketBudgetIsAcceptedByARealAgent) {
  const auto agent = Module::RetrieveRTValueTypedOrDefault<>(
      "core", "gnupg.components.gpg-agent.path", QString{});
  const auto gpgconf = Module::RetrieveRTValueTypedOrDefault<>(
      "core", "gpgme.ctx.gpgconf_path", QString{});
  if (agent.isEmpty() || gpgconf.isEmpty()) {
    GTEST_SKIP() << "GnuPG is not available";
  }

  // Short and fixed, so both lengths are reachable whatever TMPDIR is.
  QTemporaryDir base("/tmp/gfsb-XXXXXX");
  ASSERT_TRUE(base.isValid());

  const auto budget = GnuPGSocketBudget::Bytes();
  const auto at = MakeDirOfLength(base.path() + "/at", budget);
  const auto over = MakeDirOfLength(base.path() + "/over", budget + 1);
  ASSERT_FALSE(at.isEmpty());
  ASSERT_FALSE(over.isEmpty());

  // With /run/user/<uid> present GnuPG keeps the sockets there instead, and
  // the home length no longer matters.
  QProcess dirs;
  dirs.start(gpgconf, {"--homedir", at, "--list-dirs", "socketdir"});
  ASSERT_TRUE(dirs.waitForFinished(10000));
  if (QString::fromUtf8(dirs.readAllStandardOutput()).trimmed() != at) {
    GTEST_SKIP() << "GnuPG keeps its sockets outside the home directory here";
  }

  QString output;
  EXPECT_TRUE(AgentStartsIn(agent, gpgconf, at, output))
      << output.toStdString();
  EXPECT_FALSE(AgentStartsIn(agent, gpgconf, over, output))
      << output.toStdString();
}
#endif

namespace {

// A directory whose path is comfortably past the budget, so the resolver has to
// fall back to a link. Built under `base` so the test owns everything it makes.
auto MakeOverBudgetDir(const QString& base) -> QString {
  auto path = base;
  while (GnuPGSocketBudget::Fits(path)) path += "/0123456789";

  return QDir().mkpath(path) ? path : QString{};
}

// A store of this test's own, so nothing here touches the links the running
// application is using. Channels are per test to keep them independent.
auto StoreOn(int channel, const QString& root) -> GnuPGHomeLinkStore& {
  auto& store = GnuPGHomeLinkStore::GetInstance(channel);
  store.ReleaseAll();
  store.UseRoot(root);
  return store;
}

auto SkipWhenUnlimited() -> bool { return GnuPGSocketBudget::Bytes() < 0; }

}  // namespace

// A path that already fits must be handed to GnuPG untouched: the link only
// exists to rescue paths that cannot work, and creating one anyway would leave
// a trail of them on every installation that never had the problem.
TEST_F(GFCoreTest, GnuPGHomeIsUnchangedWhenItAlreadyFits) {
  QTemporaryDir root;
  ASSERT_TRUE(root.isValid());
  ASSERT_TRUE(GnuPGSocketBudget::Fits(root.path()));

  auto& store = StoreOn(5101, root.path());
  const auto home = GnuPGHomeResolver(store).Provision(root.path());

  EXPECT_TRUE(home.IsUsable());
  EXPECT_FALSE(home.IsRedirected());
  EXPECT_EQ(home.engine_path, root.path());
  EXPECT_EQ(home.key_db_path, root.path());
}

TEST_F(GFCoreTest, GnuPGHomeRedirectsAnOverBudgetPath) {
  if (SkipWhenUnlimited()) GTEST_SKIP() << "no socket path limit here";

  QTemporaryDir base;
  ASSERT_TRUE(base.isValid());
  const auto deep = MakeOverBudgetDir(base.path());
  ASSERT_FALSE(deep.isEmpty());

  auto& store = StoreOn(5102, base.path());
  const auto home = GnuPGHomeResolver(store).Provision(deep);

  ASSERT_TRUE(home.IsUsable());
  EXPECT_TRUE(home.IsRedirected());

  // The whole point: short enough for the sockets, and still the same
  // directory. The real files stay where key_db_path says.
  EXPECT_TRUE(GnuPGSocketBudget::Fits(home.engine_path));
  EXPECT_EQ(home.key_db_path, deep);
  EXPECT_EQ(QFileInfo(home.engine_path).symLinkTarget(), deep);

  store.ReleaseAll();
}

// Inspect() is what the settings dialog asks while a path is still being
// chosen. Answering it must not leave anything behind on a path the user is
// only considering.
TEST_F(GFCoreTest, GnuPGHomeInspectCreatesNothing) {
  if (SkipWhenUnlimited()) GTEST_SKIP() << "no socket path limit here";

  QTemporaryDir base;
  ASSERT_TRUE(base.isValid());
  const auto deep = MakeOverBudgetDir(base.path());
  ASSERT_FALSE(deep.isEmpty());

  const auto links = base.path() + "/l";
  auto& store = StoreOn(5103, links);
  ASSERT_FALSE(store.Root().isEmpty());

  const auto home = GnuPGHomeResolver(store).Inspect(deep);

  EXPECT_TRUE(home.IsUsable());
  // Usable, but no link exists yet -- that is Provision()'s job.
  EXPECT_TRUE(home.engine_path.isEmpty());
  EXPECT_FALSE(home.IsRedirected());
  EXPECT_FALSE(QFileInfo::exists(links));
}

// The macOS sandbox case: the container is deep enough that even a link inside
// it is over the limit, and there is then nothing honest to do but refuse.
TEST_F(GFCoreTest, GnuPGHomeIsUnusableWhenTheRootIsItselfOverBudget) {
  if (SkipWhenUnlimited()) GTEST_SKIP() << "no socket path limit here";

  QTemporaryDir base;
  ASSERT_TRUE(base.isValid());
  const auto deep = MakeOverBudgetDir(base.path());
  ASSERT_FALSE(deep.isEmpty());

  // A root no link can fit inside reads back as no root at all.
  auto& store = StoreOn(5104, deep);
  EXPECT_TRUE(store.Root().isEmpty());

  const auto home = GnuPGHomeResolver(store).Provision(deep);

  EXPECT_FALSE(home.IsUsable());
  EXPECT_FALSE(home.unusable_reason.isEmpty());

  // Untranslated and carrying the arithmetic, never the user's path.
  EXPECT_TRUE(home.unusable_reason.contains("max"));
  EXPECT_FALSE(home.unusable_reason.contains(deep));
}

// The defect this whole change exists for: the settings dialog measured the
// path itself and refused folders the engine would have rescued with a link.
// Both sides now run one predicate, and these are the three properties that
// say so.
TEST_F(GFCoreTest, GnuPGHomeInspectAndProvisionAgree) {
  if (SkipWhenUnlimited()) GTEST_SKIP() << "no socket path limit here";

  QTemporaryDir base;
  ASSERT_TRUE(base.isValid());
  const auto deep = MakeOverBudgetDir(base.path());
  ASSERT_FALSE(deep.isEmpty());

  // Fits directly, needs a link, and cannot be helped at all.
  const QStringList paths = {base.path(), deep};

  for (const auto& root : QStringList{base.path(), deep}) {
    auto& store = StoreOn(5105, root);
    GnuPGHomeResolver resolver(store);
    SCOPED_TRACE(store.Root().isEmpty() ? "no usable link root"
                                        : "link root available");

    for (const auto& path : paths) {
      SCOPED_TRACE(GnuPGSocketBudget::Fits(path) ? "path fits directly"
                                                 : "path needs a link");

      const auto inspected = resolver.Inspect(path);
      const auto provisioned = resolver.Provision(path);

      // 1. Every path Inspect() rejects, Provision() rejects too.
      if (!inspected.IsUsable()) {
        EXPECT_FALSE(provisioned.IsUsable());
        continue;
      }

      // 2. Every directly fitting path comes back identical from both, and
      //    equal to what went in.
      if (GnuPGSocketBudget::Fits(path)) {
        EXPECT_EQ(inspected.engine_path, path);
        EXPECT_EQ(provisioned.engine_path, path);
        continue;
      }

      // 3. Every alias-eligible path reaches the store: a link exists, under
      //    the store's root, resolving to the real directory.
      ASSERT_TRUE(provisioned.IsUsable());
      EXPECT_TRUE(provisioned.IsRedirected());
      EXPECT_TRUE(provisioned.engine_path.startsWith(store.Root()));
      EXPECT_EQ(QFileInfo(provisioned.engine_path).symLinkTarget(), path);
    }

    store.ReleaseAll();
  }
}

// Sibling channels opening one key database share a link rather than each
// minting its own, and two databases never collapse onto one.
TEST_F(GFCoreTest, GnuPGHomeLinksAreLeasedPerRealPath) {
  if (SkipWhenUnlimited()) GTEST_SKIP() << "no socket path limit here";

  QTemporaryDir base;
  ASSERT_TRUE(base.isValid());
  const auto first = MakeOverBudgetDir(base.path() + "/a");
  const auto second = MakeOverBudgetDir(base.path() + "/b");
  ASSERT_FALSE(first.isEmpty());
  ASSERT_FALSE(second.isEmpty());

  auto& store = StoreOn(5106, base.path());

  const auto link = store.Acquire(first);
  ASSERT_FALSE(link.isEmpty());
  EXPECT_EQ(store.Acquire(first), link);
  EXPECT_NE(store.Acquire(second), link);

  store.ReleaseAll();
}

// The links are this run's, and removing one discards no data: the key database
// it points at is untouched.
TEST_F(GFCoreTest, GnuPGHomeReleaseAllRemovesTheLinksItMade) {
  if (SkipWhenUnlimited()) GTEST_SKIP() << "no socket path limit here";

  QTemporaryDir base;
  ASSERT_TRUE(base.isValid());
  const auto deep = MakeOverBudgetDir(base.path());
  ASSERT_FALSE(deep.isEmpty());

  auto& store = StoreOn(5107, base.path());
  const auto link = store.Acquire(deep);
  ASSERT_FALSE(link.isEmpty());
  ASSERT_TRUE(QFileInfo(link).isSymLink());

  store.ReleaseAll();

  EXPECT_FALSE(QFileInfo(link).isSymLink());
  EXPECT_TRUE(QFileInfo(deep).isDir());

  // Released means released: the next Acquire mints a fresh one.
  EXPECT_NE(store.Acquire(deep), link);
  store.ReleaseAll();
}

}  // namespace GpgFrontend::Test
