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
#include <QProcess>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryDir>

#include "GpgFrontendTest.h"
#include "core/GFCoreInit.h"
#include "core/GFCoreTest.h"
#include "core/function/openpgp/ChannelHandle.h"
#include "core/module/ModuleManager.h"

namespace GpgFrontend::Test {

// v2.2.4 would not start on a GnuPG home whose gf_keydb.sqlite a newer build
// had upgraded, although GnuPG never reads that file. A GnuPG context neither
// opens it nor makes one.
TEST_F(GFCoreTest, AGnuPGContextIgnoresTheSQLiteKeyDatabase) {
  const auto gpgconf = Module::RetrieveRTValueTypedOrDefault<>(
      "core", "gpgme.ctx.gpgconf_path", QString{});
  if (gpgconf.isEmpty()) GTEST_SKIP() << "GnuPG is not available";

  for (const bool newer_db : {true, false}) {
    // Short on purpose: gpg-agent's socket may live in the homedir.
    QTemporaryDir dir(QDir::tempPath() + "/gfs-XXXXXX");
    ASSERT_TRUE(dir.isValid());
    const auto home = dir.path();
    QFile::setPermissions(home, QFileDevice::ReadOwner |
                                    QFileDevice::WriteOwner |
                                    QFileDevice::ExeOwner);
    const auto file = QDir(home).filePath("gf_keydb.sqlite");
    if (newer_db) {
      const auto connection = QStringLiteral("newer-gnupg-home");
      {
        auto sql = QSqlDatabase::addDatabase("QSQLITE", connection);
        sql.setDatabaseName(file);
        ASSERT_TRUE(sql.open());
        QSqlQuery query(sql);
        ASSERT_TRUE(query.exec("CREATE TABLE future (x INTEGER)"));
        ASSERT_TRUE(query.exec("PRAGMA user_version = 99"));
        query.finish();
        sql.close();
      }
      QSqlDatabase::removeDatabase(connection);
    }

    const auto channel = AllocateChannelHandle() + 0x310000;
    EXPECT_TRUE(BuildOpenPGPContext(
        channel, OpenPGPContextInitArgs{.engine = OpenPGPEngine::kGNUPG,
                                        .db_name = "sqlite-ignored",
                                        .db_path = home,
                                        .offline_mode = true,
                                        .auto_import_missing_key = false}))
        << "newer database: " << newer_db;
    EXPECT_EQ(QFileInfo::exists(file), newer_db)
        << "a GnuPG home gets no database file of its own";

    QProcess::execute(gpgconf, {"--homedir", home, "--kill", "all"});
  }
}

}  // namespace GpgFrontend::Test
