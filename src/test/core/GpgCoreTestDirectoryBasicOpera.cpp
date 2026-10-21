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

#include "GpgCoreTest.h"
#include <QDir>
#include <QFile>
#include <QTemporaryDir>

#include "core/function/gpg/GpgFileOpera.h"
#include "core/function/gpg/GpgKeyGetter.h"
#include "core/utils/GpgUtils.h"

namespace GpgFrontend::Test {

// Regression for a race in the archive decrypt path: the extraction task
// (which drains the decrypted plaintext from the exchanger and writes the files
// to disk) runs on a separate task runner from gpgme. The opera must not report
// completion until extraction has fully finished, otherwise the trailing files
// of the archive can be lost. Several files are packed and every one must
// survive the round-trip with identical content.
TEST_F(GpgCoreTest, CoreDirectoryEncryptDecryptRoundTrip) {
  auto key = GpgKeyGetter::GetInstance().GetPubkeyPtr(
      "E87C6A2D8D95C818DE93B3AE6A2764F8298DEB29");
  ASSERT_TRUE(key != nullptr);

  QTemporaryDir work_dir;
  ASSERT_TRUE(work_dir.isValid());

  const QString src_dir = work_dir.path() + "/test";
  ASSERT_TRUE(QDir().mkpath(src_dir));

  // Keep the total decrypted size below the exchanger queue capacity so gpgme
  // can push the whole plaintext archive into the queue and return without
  // waiting for the extractor. That maximizes the window in which the extractor
  // is still writing files after the gpg opera reports completion.
  const QContainer<QPair<QString, QByteArray>> files = {
      {"A.txt", "plain content"},
      {"A.txt.gpg", "pretend ciphertext"},
      {"A.txt.sig", "pretend signature"},
      {"B.bin", QByteArray(3 * 1024 * 1024, '\x42')},
  };

  for (const auto& [name, content] : files) {
    QFile f(src_dir + "/" + name);
    ASSERT_TRUE(f.open(QIODevice::WriteOnly));
    ASSERT_EQ(f.write(content), content.size());
    f.close();
  }

  const QString archive_path = work_dir.path() + "/test.tar.gpg";

  bool encrypt_done = false;
  GpgError encrypt_err = GPG_ERR_GENERAL;
  GpgFileOpera::GetInstance().EncryptDirectory(
      {key}, src_dir, false, archive_path,
      [&](GpgError err, const DataObjectPtr&) {
        encrypt_err = err;
        encrypt_done = true;
      });
  WAIT_FOR_TRUE(encrypt_done, 10000);
  ASSERT_EQ(CheckGpgError(encrypt_err), GPG_ERR_NO_ERROR);
  ASSERT_TRUE(QFileInfo::exists(archive_path));

  const QString out_dir = work_dir.path() + "/out";
  ASSERT_TRUE(QDir().mkpath(out_dir));

  bool decrypt_done = false;
  GpgError decrypt_err = GPG_ERR_GENERAL;
  GpgFileOpera::GetInstance().DecryptArchive(
      archive_path, out_dir, [&](GpgError err, const DataObjectPtr&) {
        decrypt_err = err;
        decrypt_done = true;
      });
  WAIT_FOR_TRUE(decrypt_done, 10000);
  ASSERT_EQ(CheckGpgError(decrypt_err), GPG_ERR_NO_ERROR);

  // Every packed file must reappear with identical content. A lost trailing
  // file (the original bug) fails here.
  for (const auto& [name, content] : files) {
    QFile f(out_dir + "/" + name);
    ASSERT_TRUE(f.open(QIODevice::ReadOnly))
        << "missing extracted file: " << name.toStdString();
    EXPECT_EQ(f.readAll(), content)
        << "content mismatch for file: " << name.toStdString();
    f.close();
  }
}

}  // namespace GpgFrontend::Test
