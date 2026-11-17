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

#include <QTemporaryDir>

#include "GpgCoreEngineTest.h"
#include "core/function/GFBufferFactory.h"
#include "core/function/KeyPackageOperator.h"
#include "core/function/openpgp/GpgKeyRepository.h"
#include "core/function/openpgp/KeyImportExportOperation.h"
#include "core/model/GpgImportInformation.h"
#include "core/model/GpgKey.h"
#include "core/utils/GpgUtils.h"
#include "core/utils/IOUtils.h"

namespace GpgFrontend::Test {

namespace {

/// Exports @p key from @p channel, armored, as the package writer does.
auto Export(int channel, const GpgKeyPtr& key, bool secret) -> GFBuffer {
  auto [err, buffer] = KeyImportExportOperation::GetInstance(channel).ExportKey(
      key, secret, true, false);
  EXPECT_TRUE(err.Ok());
  return buffer;
}

/// Writes a key package the way KeyPackageOperator::GenerateKeyPackage()
/// does: the key data sealed with a passphrase, the passphrase sealed with
/// the PIN, each in a file of its own.
void WritePackage(const QString& package_path, const QString& key_path,
                  const GFBuffer& key_data, const GFBuffer& pin) {
  const GFBuffer passphrase(QString("a-random-package-passphrase"));
  auto sealed_data = GFBufferFactory::Encrypt(passphrase, key_data);
  auto sealed_passphrase = GFBufferFactory::Encrypt(pin, passphrase);
  ASSERT_TRUE(sealed_data && sealed_passphrase);
  ASSERT_TRUE(WriteFileGFBuffer(package_path, *sealed_data));
  ASSERT_TRUE(WriteFileGFBuffer(key_path, *sealed_passphrase));
}

auto Present(int channel, const QString& fpr) -> bool {
  GpgKeyRepository::GetInstance(channel).FlushKeyCache();
  const auto key = GpgKeyRepository::GetInstance(channel).GetKeyPtr(fpr);
  return key != nullptr && key->IsGood();
}

}  // namespace

TEST_P(GpgCoreEngineTest, KeyPackageOpensOnlyWithItsPin) {
  auto key = GenerateFullKey("key_package_pin");
  ASSERT_TRUE(key != nullptr);

  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());
  const auto package_path = dir.filePath("pkg.gfpack");
  const auto key_path = dir.filePath("pkg.key");
  const auto key_data = Export(Channel(), key, false);
  WritePackage(package_path, key_path, key_data, GFBuffer(QString("1234-pin")));

  auto& opera = KeyPackageOperator::GetInstance(Channel());

  // The PIN opens the key file; a wrong one fails there, and only there, so
  // the wizard can ask for it again.
  auto [wrong_err, wrong_data] =
      opera.OpenKeyPackage(package_path, key_path, GFBuffer(QString("nope")));
  EXPECT_EQ(wrong_err, "Decrypt the Key of Key Package Failed");
  EXPECT_TRUE(wrong_data.Empty());

  auto [err, data] = opera.OpenKeyPackage(package_path, key_path,
                                          GFBuffer(QString("1234-pin")));
  EXPECT_TRUE(err.isEmpty()) << err.toStdString();
  EXPECT_EQ(data.ConvertToQByteArray(), key_data.ConvertToQByteArray());

  DeleteKey(key);
}

TEST_P(GpgCoreEngineTest, KeyPackageListsEachKeyOnce) {
  auto with_secret = GenerateFullKey("key_package_list_a");
  auto public_only = GenerateFullKey("key_package_list_b");
  ASSERT_TRUE(with_secret != nullptr && public_only != nullptr);

  // Laid out as a package written with private keys: every key's public part,
  // then the private parts of the keys that have one.
  GFBuffer key_data = Export(Channel(), with_secret, false);
  key_data.Append(Export(Channel(), public_only, false));
  key_data.Append(Export(Channel(), with_secret, true));

  const auto keys = KeyPackageOperator::ListKeyPackageKeys(key_data);
  ASSERT_EQ(keys.size(), 2);

  const auto find = [&](const QString& fpr) {
    return std::find_if(keys.begin(), keys.end(), [&](const auto& k) {
      return k.fpr.compare(fpr, Qt::CaseInsensitive) == 0;
    });
  };
  const auto a = find(with_secret->Fingerprint());
  const auto b = find(public_only->Fingerprint());
  ASSERT_NE(a, keys.end());
  ASSERT_NE(b, keys.end());
  EXPECT_TRUE(a->has_secret);
  EXPECT_FALSE(b->has_secret);
  EXPECT_FALSE(a->key_id.isEmpty());
  EXPECT_TRUE(a->name.contains("key_package_list_a") ||
              a->email.contains("key_package_list_a"));

  DeleteKey(with_secret);
  DeleteKey(public_only);
}

TEST_P(GpgCoreEngineTest, KeyPackageImportsOnlyTheChosenKeys) {
  auto chosen = GenerateFullKey("key_package_chosen");
  auto left_out = GenerateFullKey("key_package_left_out");
  ASSERT_TRUE(chosen != nullptr && left_out != nullptr);
  const auto chosen_fpr = chosen->Fingerprint();
  const auto left_out_fpr = left_out->Fingerprint();

  GFBuffer key_data = Export(Channel(), chosen, false);
  key_data.Append(Export(Channel(), left_out, false));

  // Gone from the keyring, so the import alone decides whether they return.
  DeleteKey(chosen);
  DeleteKey(left_out);
  ASSERT_FALSE(Present(Channel(), chosen_fpr));
  ASSERT_FALSE(Present(Channel(), left_out_fpr));

  auto& opera = KeyPackageOperator::GetInstance(Channel());
  auto [err, info] = opera.ImportKeyPackageKeys(key_data, {chosen_fpr});
  EXPECT_TRUE(err.isEmpty()) << err.toStdString();
  ASSERT_TRUE(info != nullptr);

  EXPECT_TRUE(Present(Channel(), chosen_fpr));
  EXPECT_FALSE(Present(Channel(), left_out_fpr));

  // A fingerprint the package does not hold imports nothing, and says so.
  auto [none_err, none_info] =
      opera.ImportKeyPackageKeys(key_data, {QString(40, 'F')});
  EXPECT_EQ(none_err, "No Selected Key Found in the Key Package");
  EXPECT_TRUE(none_info == nullptr);

  GpgKeyRepository::GetInstance(Channel()).FlushKeyCache();
  DeleteKey(GpgKeyRepository::GetInstance(Channel()).GetKeyPtr(chosen_fpr));
}

TEST_P(GpgCoreEngineTest, KeyPackageV3WithPinOpensLikeVersion2) {
  auto key = GenerateFullKey("key_package_v3_pin");
  ASSERT_TRUE(key != nullptr);

  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());
  KeyPackageV3Request request;
  request.package_path = dir.filePath("pkg.gfpack");
  request.key_path = dir.filePath("pkg.key");
  request.keys = {key};
  request.secret = true;
  request.protection = KeyPackageProtection::kPIN_FILE;
  request.pin = GFBuffer(QString("1234-pin"));

  auto& opera = KeyPackageOperator::GetInstance(Channel());
  const auto written = opera.GenerateKeyPackageV3Sync(request);
  ASSERT_TRUE(written.error.isEmpty()) << written.error.toStdString();
  EXPECT_TRUE(written.plain_key.Empty());
  EXPECT_FALSE(written.package_id.isEmpty());

  const auto info = KeyPackageOperator::InspectKeyPackage(request.package_path);
  EXPECT_EQ(info.version, 3);
  EXPECT_EQ(info.protection, KeyPackageProtection::kPIN_FILE);
  EXPECT_EQ(info.package_id, written.package_id);

  // The same call the import wizard makes for a version 2 package.
  auto [wrong_err, wrong_data] = opera.OpenKeyPackage(
      request.package_path, request.key_path, GFBuffer(QString("nope")));
  EXPECT_FALSE(wrong_err.isEmpty());
  EXPECT_TRUE(wrong_data.Empty());

  auto [err, data] =
      opera.OpenKeyPackage(request.package_path, request.key_path, request.pin);
  ASSERT_TRUE(err.isEmpty()) << err.toStdString();
  const auto keys = KeyPackageOperator::ListKeyPackageKeys(data);
  ASSERT_EQ(keys.size(), 1);
  EXPECT_EQ(keys.front().fpr.toUpper(), key->Fingerprint().toUpper());
  EXPECT_TRUE(keys.front().has_secret);

  DeleteKey(key);
}

TEST_P(GpgCoreEngineTest, KeyPackageV3WithAPlainKeyOpensWithItsText) {
  auto key = GenerateFullKey("key_package_v3_plain");
  ASSERT_TRUE(key != nullptr);

  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());
  KeyPackageV3Request request;
  request.package_path = dir.filePath("pkg.gfpack");
  request.keys = {key};
  request.secret = false;
  request.protection = KeyPackageProtection::kPLAIN;

  auto& opera = KeyPackageOperator::GetInstance(Channel());
  const auto written = opera.GenerateKeyPackageV3Sync(request);
  ASSERT_TRUE(written.error.isEmpty()) << written.error.toStdString();
  ASSERT_FALSE(written.plain_key.Empty());
  EXPECT_TRUE(written.plain_key.ConvertToQString().startsWith("gfkp3:"));
  EXPECT_FALSE(QFileInfo::exists(dir.filePath("pkg.key")));

  const auto info = KeyPackageOperator::InspectKeyPackage(request.package_path);
  EXPECT_EQ(info.version, 3);
  EXPECT_EQ(info.protection, KeyPackageProtection::kPLAIN);
  EXPECT_FALSE(info.secrets_unprotected);

  auto [err, data] = KeyPackageOperator::OpenKeyPackageWithSecret(
      request.package_path, written.plain_key);
  ASSERT_TRUE(err.isEmpty()) << err.toStdString();
  const auto keys = KeyPackageOperator::ListKeyPackageKeys(data);
  ASSERT_EQ(keys.size(), 1);
  EXPECT_FALSE(keys.front().has_secret);

  auto [wrong_err, wrong_data] = KeyPackageOperator::OpenKeyPackageWithSecret(
      request.package_path, GFBuffer(QString("gfkp3:") + QString(64, '0')));
  EXPECT_FALSE(wrong_err.isEmpty());
  EXPECT_TRUE(wrong_data.Empty());

  DeleteKey(key);
}

TEST_P(GpgCoreEngineTest, KeyPackageInspectionTellsTheVersionsApart) {
  auto key = GenerateFullKey("key_package_inspect");
  ASSERT_TRUE(key != nullptr);

  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());
  const auto v2_path = dir.filePath("v2.gfpack");
  WritePackage(v2_path, dir.filePath("v2.key"), Export(Channel(), key, false),
               GFBuffer(QString("pin")));
  EXPECT_EQ(KeyPackageOperator::InspectKeyPackage(v2_path).version, 2);

  const auto other = dir.filePath("other.txt");
  ASSERT_TRUE(WriteFileGFBuffer(other, GFBuffer(QString("hello"))));
  EXPECT_EQ(KeyPackageOperator::InspectKeyPackage(other).version, 0);
  EXPECT_EQ(
      KeyPackageOperator::InspectKeyPackage(dir.filePath("missing")).version,
      0);

  DeleteKey(key);
}

#ifdef GF_TEST_GFPIPE

namespace {

/// A program's exit code and everything it printed.
struct Ran {
  int code = -1;
  QString output;
};

auto RunIn(const QString& dir, const QProcessEnvironment& env,
           const QString& program, const QStringList& args,
           const QByteArray& input = {}) -> Ran {
  QProcess process;
  process.setWorkingDirectory(dir);
  process.setProcessEnvironment(env);
  process.setProcessChannelMode(QProcess::MergedChannels);
  process.start(program, args);
  process.write(input);
  process.closeWriteChannel();
  if (!process.waitForFinished(60000)) return {};
  return {
      process.exitStatus() == QProcess::NormalExit ? process.exitCode() : -1,
      QString::fromUtf8(process.readAll())};
}

/// The environment a user's shell has, without anything naming a package or
/// key: what gfpipe finds beside itself is all it gets.
auto WithoutGfPipeVariables() -> QProcessEnvironment {
  auto env = QProcessEnvironment::systemEnvironment();
  for (const auto& name :
       {u"GFPIPE_KEYPACK"_s, u"GFPIPE_KEY"_s, u"GFPIPE_KEY_FILE"_s}) {
    env.remove(name);
  }
  return env;
}

/// Export `key` with gfpipe into `folder`, as the wizard's "Include gfpipe".
auto ExportWithGfPipe(int channel, const GpgAbstractKeyPtr& key,
                      const QString& folder) -> KeyPackageV3Written {
  KeyPackageV3Request request;
  request.package_path = QDir(folder).filePath(u"default.gfpack"_s);
  request.keys = {key};
  request.secret = true;
  request.remove_passphrase = true;
  request.protection = KeyPackageProtection::kPLAIN;
  request.gfpipe_program = QString(GF_TEST_GFPIPE);
  return KeyPackageOperator::GetInstance(channel).GenerateKeyPackageV3Sync(
      request);
}

}  // namespace

/// GpgFrontend carries gfpipe beside itself (Contents/Helpers on macOS); a
/// test program in the build folder sits beside the built gfpipe.
TEST(KeyPackageGfPipeTest, TheCarriedGfPipeIsFound) {
#ifndef Q_OS_MACOS
  EXPECT_EQ(KeyPackageOperator::BundledGfPipe(),
            QFileInfo(QString(GF_TEST_GFPIPE)).canonicalFilePath());
#endif
}

/// What "Include gfpipe" writes is ready to use: gfpipe, default.gfpack and
/// default.gfkey in one folder, and gfpipe run from there signs with nothing
/// set.
TEST_P(GpgCoreEngineTest, AnExportWithGfPipeRunsAsItIs) {
  if (GetParam().engine != OpenPGPEngine::kRPGP) {
    GTEST_SKIP() << "covered by the rPGP run";
  }
  auto key = GenerateFullKey("gfpipe_folder");
  ASSERT_TRUE(key != nullptr);
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  const auto written = ExportWithGfPipe(Channel(), key, dir.path());
  ASSERT_TRUE(written.error.isEmpty()) << written.error.toStdString();
  EXPECT_EQ(written.gfpipe_path,
            QDir(dir.path()).filePath(QFileInfo(GF_TEST_GFPIPE).fileName()));
  EXPECT_EQ(written.key_text_path, dir.filePath(u"default.gfkey"_s));
  EXPECT_TRUE(QFileInfo(written.gfpipe_path).isExecutable());
#ifndef Q_OS_WINDOWS
  EXPECT_EQ(QFile::permissions(written.key_text_path) &
                (QFile::ReadGroup | QFile::ReadOther),
            QFileDevice::Permissions{});
#endif

  const auto r = RunIn(dir.path(), WithoutGfPipeVariables(),
                       written.gfpipe_path, {u"-bsa"_s}, "data\n");
  EXPECT_EQ(r.code, 0) << r.output.toStdString();
  EXPECT_TRUE(r.output.contains(u"BEGIN PGP SIGNATURE"_s))
      << r.output.toStdString();

  DeleteKey(key);
}

/// What a user does: export a key package with gfpipe in GpgFrontend, point
/// git at the exported gfpipe, and sign and verify commits and tags with the
/// real git.
TEST_P(GpgCoreEngineTest, GitSignsAndVerifiesThroughGfPipe) {
  const auto git = QStandardPaths::findExecutable(u"git"_s);
  if (git.isEmpty()) GTEST_SKIP() << "git is not installed";
  // The package and gfpipe do not depend on the engine that exported the
  // key; the rPGP run keeps rPGP's rare rejection of a GnuPG-made secret key
  // ("Invalid checksum") out of a test about git.
  if (GetParam().engine != OpenPGPEngine::kRPGP) {
    GTEST_SKIP() << "covered by the rPGP run";
  }

  auto key = GenerateFullKey("gfpipe_git");
  ASSERT_TRUE(key != nullptr);

  // gfpipe as the export writes it: in a folder of its own with the package
  // and its key, and nothing set to say where they are.
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());
  const auto tools = dir.filePath(u"tools"_s);
  ASSERT_TRUE(QDir().mkpath(tools));
  const auto written = ExportWithGfPipe(Channel(), key, tools);
  ASSERT_TRUE(written.error.isEmpty()) << written.error.toStdString();

  // An isolated git: no system or user configuration, its own HOME.
  const auto repo = dir.filePath("repo");
  ASSERT_TRUE(QDir().mkpath(repo));
  auto env = WithoutGfPipeVariables();
  env.insert(u"HOME"_s, dir.path());
  env.insert(u"GIT_CONFIG_NOSYSTEM"_s, u"1"_s);
  env.insert(u"GIT_CONFIG_GLOBAL"_s, dir.filePath("gitconfig"));
  const auto run = [&](const QStringList& args) {
    return RunIn(repo, env, git, args);
  };

  for (const auto& args : QList<QStringList>{
           {u"init"_s, u"-q"_s, u"-b"_s, u"main"_s},
           {u"config"_s, u"user.name"_s, key->Name()},
           {u"config"_s, u"user.email"_s, key->Email()},
           {u"config"_s, u"gpg.program"_s, written.gfpipe_path}}) {
    const auto r = run(args);
    ASSERT_EQ(r.code, 0) << r.output.toStdString();
  }

  // No user.signingKey: git passes the committer ident.
  auto r = run({u"commit"_s, u"-q"_s, u"--allow-empty"_s, u"-S"_s, u"-m"_s,
                u"signed by ident"_s});
  ASSERT_EQ(r.code, 0) << r.output.toStdString();
  r = run({u"verify-commit"_s, u"HEAD"_s});
  EXPECT_EQ(r.code, 0) << r.output.toStdString();
  r = run({u"log"_s, u"-1"_s, u"--format=%G? %GP"_s});
  EXPECT_EQ(r.output.trimmed(), u"G "_s + key->Fingerprint())
      << r.output.toStdString();

  // With user.signingKey, on a branch merged with --verify-signatures.
  ASSERT_EQ(run({u"config"_s, u"user.signingKey"_s, key->Fingerprint()}).code,
            0);
  ASSERT_EQ(run({u"switch"_s, u"-q"_s, u"-c"_s, u"side"_s}).code, 0);
  r = run({u"commit"_s, u"-q"_s, u"--allow-empty"_s, u"-S"_s, u"-m"_s,
           u"signed by key"_s});
  ASSERT_EQ(r.code, 0) << r.output.toStdString();
  ASSERT_EQ(run({u"switch"_s, u"-q"_s, u"main"_s}).code, 0);
  r = run({u"merge"_s, u"-q"_s, u"--no-edit"_s, u"--verify-signatures"_s,
           u"side"_s});
  EXPECT_EQ(r.code, 0) << r.output.toStdString();

  // A signed tag.
  r = run({u"tag"_s, u"-s"_s, u"v1"_s, u"-m"_s, u"release"_s});
  ASSERT_EQ(r.code, 0) << r.output.toStdString();
  r = run({u"verify-tag"_s, u"v1"_s});
  EXPECT_EQ(r.code, 0) << r.output.toStdString();

  // A commit signed by a key the package does not hold is not good.
  r = run({u"-c"_s,
           u"user.signingKey=0123456789ABCDEF0123456789ABCDEF01234567"_s,
           u"commit"_s, u"-q"_s, u"--allow-empty"_s, u"-S"_s, u"-m"_s,
           u"no such key"_s});
  EXPECT_NE(r.code, 0);

  DeleteKey(key);
}

#endif

}  // namespace GpgFrontend::Test
