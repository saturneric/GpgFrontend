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

// A key listing gpg refuses to run is an error to report, not an assertion:
// a homedir whose gpg.conf holds an option this gpg does not know makes
// every listing fail, and a debug build used to abort on the spot.

#include <gtest/gtest.h>

#include <QDir>
#include <QFile>
#include <QProcess>

#include "GpgFrontendTest.h"
#include "core/GFCoreInit.h"
#include "core/function/openpgp/AbstractKeyRepository.h"
#include "core/function/openpgp/ChannelHandle.h"
#include "core/module/ModuleManager.h"

namespace GpgFrontend::Test {

TEST(GpgCoreTestKeyListingFailure, RefusedGpgConfFailsTheListingCleanly) {
  const auto gpgconf = Module::RetrieveRTValueTypedOrDefault<>(
      "core", "gpgme.ctx.gpgconf_path", QString{});
  if (gpgconf.isEmpty()) GTEST_SKIP() << "GnuPG is not available";

  // Short on purpose: gpg-agent's socket may live in the homedir.
  const auto home = QDir::tempPath() + "/gfl-" + GenerateRandomString(6);
  ASSERT_TRUE(QDir().mkpath(home));
  QFile::setPermissions(home, QFileDevice::ReadOwner | QFileDevice::WriteOwner |
                                  QFileDevice::ExeOwner);
  {
    QFile conf(QDir(home).filePath("gpg.conf"));
    ASSERT_TRUE(conf.open(QIODevice::WriteOnly));
    conf.write("no-such-option-for-this-test\n");
  }

  const auto channel = AllocateChannelHandle() + 0x300000;
  BuildOpenPGPContext(channel,
                      OpenPGPContextInitArgs{.engine = OpenPGPEngine::kGNUPG,
                                             .db_name = "refused-conf",
                                             .db_path = home,
                                             .offline_mode = true,
                                             .auto_import_missing_key = false});

  auto& repo = AbstractKeyRepository::GetInstance(channel);
  repo.FlushCache();
  EXPECT_EQ(repo.GetKey("0000000000000000"), nullptr);

  QProcess::execute(gpgconf, {"--homedir", home, "--kill", "all"});
  QDir(home).removeRecursively();
}

}  // namespace GpgFrontend::Test
