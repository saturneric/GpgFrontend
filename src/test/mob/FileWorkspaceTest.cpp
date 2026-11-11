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
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <array>

#include "MobileCryptoTest.h"
#include "core/function/rpgp/PasswordFetcher.h"
#include "crypto/TextCryptoController.h"
#include "files/FileFilterModel.h"
#include "files/FileOperations.h"
#include "files/FileWorkspace.h"
#include "files/ShareInbox.h"
#include "files/ShareRetentionStore.h"
#include "files/StagingArea.h"
#include "lifecycle/AppLifecycle.h"

namespace GpgFrontend::Test::Mobile {

using GpgFrontend::SharedItems;
using GpgFrontend::Mobile::FileFilterModel;
using GpgFrontend::Mobile::FileWorkspace;
using GpgFrontend::Mobile::OperationQueue;
using GpgFrontend::Mobile::SharedFileStore;
using GpgFrontend::Mobile::ShareInbox;
using GpgFrontend::Mobile::TextCryptoController;

namespace {

constexpr auto kFilePassphrase = "erin's file passphrase";

void WriteFile(const QString& path, const QByteArray& content) {
  QFile file(path);
  ASSERT_TRUE(file.open(QIODevice::WriteOnly));
  file.write(content);
}

auto ReadFile(const QString& path) -> QByteArray {
  QFile file(path);
  return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}

/// Every file in the staging area, wherever it is.
auto StagedFiles() -> QStringList {
  QStringList files;
  const auto root =
      QStandardPaths::writableLocation(QStandardPaths::CacheLocation) +
      "/staging";
  QDirIterator it(root, QDir::Files, QDirIterator::Subdirectories);
  while (it.hasNext()) {
    const auto path = it.next();
    if (!path.endsWith("session.lock")) files << path;
  }
  return files;
}

}  // namespace

/// A workspace of the test's own, on the application's queue, as the
/// application's is.
auto NewWorkspace() -> FileWorkspace* {
  return new FileWorkspace(
      OperationQueue::Instance(),
      GpgFrontend::Mobile::AppLifecycle::create(nullptr, nullptr));
}

class FileWorkspaceTest : public ::testing::Test {
 protected:
  static inline QString erin_;  ///< has a passphrase
  static inline QString fred_;  ///< has none
  QTemporaryDir dir_;
  FileWorkspace* files_ = nullptr;

  static void SetUpTestSuite() {
    erin_ = GenerateKey("Erin", kFilePassphrase);
    fred_ = GenerateKey("Fred", {});
  }

  void SetUp() override {
    ASSERT_FALSE(erin_.isEmpty());
    ASSERT_FALSE(fred_.isEmpty());
    ClearRpgpPasswordCache();
    RunOnMainThread([this]() { files_ = NewWorkspace(); });
  }

  void TearDown() override {
    RunOnMainThread([]() { Broker()->CancelAll(); });
    RunOnMainThread([this]() { delete files_; });
  }

  void Do(const std::function<void(FileWorkspace*)>& op) {
    RunOnMainThread([this, &op]() { op(files_); });
  }

  /// The roles of the file @p id, by name.
  auto Entry(int id) -> QVariantMap {
    QVariantMap entry;
    RunOnMainThread([this, id, &entry]() { entry = files_->entry(id); });
    return entry;
  }

  auto State(int id) -> QString { return Entry(id).value("phase").toString(); }

  auto Count() -> int {
    int count = 0;
    RunOnMainThread([this, &count]() { count = files_->Count(); });
    return count;
  }

  void Finish() {
    EXPECT_TRUE(WaitFor(
        [this]() {
          bool busy = true;
          RunOnMainThread([this, &busy]() { busy = files_->Busy(); });
          return !busy;
        },
        30000));
  }

  /// Write @p content to @p name and add it, as the picker would; its id.
  auto Add(const QByteArray& content, const QString& name = "notes.txt")
      -> int {
    const auto path = dir_.filePath(name);
    WriteFile(path, content);
    int id = -1;
    RunOnMainThread([this, &path, &id]() {
      files_->addFiles({QUrl::fromLocalFile(path)});
      // The newest is first.
      id = files_->data(files_->index(0), FileWorkspace::kIdRole).toInt();
    });
    return id;
  }

  /// The output made from @p source, or -1.
  auto OutputOf(int source) -> int {
    int id = -1;
    RunOnMainThread([this, source, &id]() {
      for (int row = 0; row < files_->rowCount(); ++row) {
        const auto at = files_->index(row);
        if (files_->data(at, FileWorkspace::kSourceIdRole).toInt() == source) {
          id = files_->data(at, FileWorkspace::kIdRole).toInt();
        }
      }
    });
    return id;
  }

  /// Save the file @p id where a save picker would, and read it back.
  auto SaveAndRead(int id, const QString& name) -> QByteArray {
    const auto path = dir_.filePath(name);
    bool saved = false;
    bool done = false;
    RunOnMainThread([this, id, &path, &saved, &done]() {
      QObject::connect(
          files_, &FileWorkspace::SignalSaved, files_,
          [&saved, &done](int, bool ok) {
            saved = ok;
            done = true;
          },
          Qt::SingleShotConnection);
      files_->saveAs(id, QUrl::fromLocalFile(path));
    });
    EXPECT_TRUE(WaitFor(
        [this, &done]() {
          bool d = false;
          RunOnMainThread([&done, &d]() { d = done; });
          return d;
        },
        30000));
    EXPECT_TRUE(saved);
    return ReadFile(path);
  }

  auto Errors(int id) -> std::string {
    return Entry(id)
        .value("result")
        .toMap()
        .value("errors")
        .toStringList()
        .join("; ")
        .toStdString();
  }
};

// A file's SHA-256, read a piece at a time: the known digest of "abc", why
// not for a file that is not there, and nothing once it is no longer wanted.
TEST(FileDigestTest, AFileIsHashedOrSaysWhyNot) {
  QTemporaryDir dir;
  const auto path = dir.filePath("abc.txt");
  QFile file(path);
  ASSERT_TRUE(file.open(QIODevice::WriteOnly));
  file.write("abc");
  file.close();
  const auto digest = GpgFrontend::Mobile::Sha256OfFile(path, {});
  EXPECT_EQ(digest.sha256,
            "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
  EXPECT_TRUE(digest.error.isEmpty());

  const auto missing =
      GpgFrontend::Mobile::Sha256OfFile(dir.filePath("none"), {});
  EXPECT_EQ(missing.error, "unreadable");
  EXPECT_TRUE(missing.sha256.isEmpty());

  const auto unwanted =
      GpgFrontend::Mobile::Sha256OfFile(path, []() { return false; });
  EXPECT_TRUE(unwanted.sha256.isEmpty());
  EXPECT_TRUE(unwanted.error.isEmpty());
}

TEST_F(FileWorkspaceTest, EncryptAFileToAKeyAndDecryptIt) {
  const QByteArray content = "line one\nline two\n";
  const auto source = Add(content);
  const auto staged_before = StagedFiles().size();

  Do([&](auto* f) { f->encrypt({source}, {fred_}); });
  Finish();
  EXPECT_EQ(State(source), "done") << Errors(source);
  const auto encrypted = OutputOf(source);
  ASSERT_GE(encrypted, 0) << Errors(source);
  auto output = Entry(encrypted);
  EXPECT_EQ(output.value("name").toString(), "notes.txt.gpg");
  EXPECT_EQ(output.value("kind").toString(), "output");
  EXPECT_EQ(output.value("operation").toString(), "encrypt");
  EXPECT_EQ(output.value("madeBy").toString(), "encrypt");
  // Paired with what it was made from by id; no name kept for show.
  EXPECT_EQ(output.value("sourceId").toInt(), source);
  EXPECT_FALSE(output.contains("sourceName"));
  EXPECT_FALSE(output.value("plaintext").toBool());
  // The copy of the input is gone; only the output is staged.
  EXPECT_EQ(StagedFiles().size(), staged_before + 1);

  // Saving copies it and keeps it.
  const auto saved = SaveAndRead(encrypted, "notes.txt.gpg");
  ASSERT_FALSE(saved.isEmpty());
  EXPECT_NE(saved, content);
  EXPECT_EQ(Entry(encrypted).value("kind").toString(), "output");
  EXPECT_EQ(StagedFiles().size(), staged_before + 1);

  // An output is worked on where it is, without another copy.
  Do([&](auto* f) { f->decrypt({encrypted}); });
  Finish();
  const auto decrypted = OutputOf(encrypted);
  ASSERT_GE(decrypted, 0) << Errors(encrypted);
  output = Entry(decrypted);
  // notes.txt is in the list already, the file it all started from.
  EXPECT_EQ(output.value("name").toString(), "notes (2).txt");
  EXPECT_TRUE(output.value("plaintext").toBool());
  EXPECT_EQ(output.value("result").toMap().value("status"), 1);
  EXPECT_EQ(SaveAndRead(decrypted, "decrypted.txt"), content);
  EXPECT_EQ(output.value("madeBy").toString(), "decrypt");

  // Decrypting the encrypted file is the last thing done to it, not what
  // made it.
  output = Entry(encrypted);
  EXPECT_EQ(output.value("operation").toString(), "decrypt");
  EXPECT_EQ(output.value("madeBy").toString(), "encrypt");
}

TEST_F(FileWorkspaceTest, AFileForAPassphraseRoundTrips) {
  const QByteArray content = "for a passphrase";
  const auto source = Add(content);
  Do([&](auto* f) { f->encrypt({source}, {}, {}, true); });
  ASSERT_FALSE(WaitForRequest().isEmpty());
  Answer("file passphrase");
  Finish();
  const auto encrypted = OutputOf(source);
  ASSERT_GE(encrypted, 0);

  Do([&](auto* f) { f->decrypt({encrypted}); });
  ASSERT_FALSE(WaitForRequest().isEmpty());
  Answer("file passphrase");
  Finish();
  const auto decrypted = OutputOf(encrypted);
  ASSERT_GE(decrypted, 0);
  EXPECT_EQ(SaveAndRead(decrypted, "back.txt"), content);
}

TEST_F(FileWorkspaceTest, ACancelledPromptLeavesNothingStaged) {
  const auto source = Add("secret");
  Do([&](auto* f) { f->encrypt({source}, {erin_}); });
  Finish();
  const auto encrypted = OutputOf(source);
  ASSERT_GE(encrypted, 0);
  const auto saved = SaveAndRead(encrypted, "secret.gpg");
  ASSERT_FALSE(saved.isEmpty());

  // From a document this time, so its copy is staged while it waits.
  const auto added = Add(saved, "secret.gpg");
  const auto staged_before = StagedFiles().size();
  const auto count_before = Count();
  Do([&](auto* f) { f->decrypt({added}); });
  ASSERT_FALSE(WaitForRequest().isEmpty());
  EXPECT_GT(StagedFiles().size(), staged_before);
  EXPECT_EQ(State(added), "running");
  Decline();
  Finish();

  // Neither the copy nor an output outlives the cancelled operation.
  EXPECT_EQ(State(added), "cancelled");
  EXPECT_TRUE(Entry(added).value("result").toMap().value("cancelled").toBool());
  EXPECT_EQ(Count(), count_before);
  EXPECT_EQ(StagedFiles().size(), staged_before);
}

TEST_F(FileWorkspaceTest, AFileThatCannotBeReadFailsCleanly) {
  const auto staged_before = StagedFiles().size();
  const auto gone = Add("soon gone", "gone.gpg");
  ASSERT_TRUE(QFile::remove(dir_.filePath("gone.gpg")));
  Do([&](auto* f) { f->decrypt({gone}); });
  Finish();
  EXPECT_EQ(State(gone), "failed");
  EXPECT_LT(Entry(gone).value("result").toMap().value("status").toInt(), 0);
  EXPECT_LT(OutputOf(gone), 0);
  EXPECT_EQ(StagedFiles().size(), staged_before);

  // Not an encrypted file at all.
  const auto plain = Add("plain text", "plain.txt");
  Do([&](auto* f) { f->decrypt({plain}); });
  Finish();
  EXPECT_EQ(State(plain), "failed");
  EXPECT_LT(OutputOf(plain), 0);
  EXPECT_EQ(StagedFiles().size(), staged_before);
}

// No share sheet on Linux: the output is kept, and nothing goes into the
// store.
TEST_F(FileWorkspaceTest, WithoutAShareSheetTheOutputStays) {
  const auto source = Add("to share");
  Do([&](auto* f) { f->encrypt({source}, {fred_}); });
  Finish();
  const auto output = OutputOf(source);
  ASSERT_GE(output, 0);

  ASSERT_NE(SharedFileStore(), nullptr);
  qsizetype retained = 0;
  RunOnMainThread(
      [&retained]() { retained = SharedFileStore()->Entries().size(); });

  QString outcome;
  RunOnMainThread([this, output, &outcome]() {
    QObject::connect(files_, &FileWorkspace::SignalShared,
                     [&outcome](int, const QString& o) { outcome = o; });
    files_->share(output);
  });
  EXPECT_EQ(outcome, "unsupported");
  EXPECT_EQ(Entry(output).value("kind").toString(), "output");
  RunOnMainThread([&retained]() {
    EXPECT_EQ(SharedFileStore()->Entries().size(), retained);
  });
}

TEST_F(FileWorkspaceTest, RemovingAnOutputDeletesItRemovingADocumentKeepsIt) {
  const auto staged_before = StagedFiles().size();
  const auto source = Add("drop me");
  Do([&](auto* f) { f->encrypt({source}, {fred_}); });
  Finish();
  const auto output = OutputOf(source);
  ASSERT_GE(output, 0);
  EXPECT_EQ(StagedFiles().size(), staged_before + 1);

  Do([&](auto* f) { f->remove({output, source}); });
  EXPECT_EQ(Count(), 0);
  // The application's copy is deleted; the person's document is not.
  EXPECT_EQ(StagedFiles().size(), staged_before);
  EXPECT_EQ(ReadFile(dir_.filePath("notes.txt")), "drop me");
}

TEST_F(FileWorkspaceTest, TheOutputsGoWithTheWorkspace) {
  const auto staged_before = StagedFiles().size();
  const auto source = Add("short lived");
  Do([&](auto* f) { f->decrypt({}); });  // nothing to do is nothing done
  Do([&](auto* f) { f->encrypt({source}, {fred_}); });
  Finish();
  ASSERT_GE(OutputOf(source), 0);
  EXPECT_EQ(StagedFiles().size(), staged_before + 1);

  RunOnMainThread([this]() {
    delete files_;
    files_ = NewWorkspace();
  });
  EXPECT_EQ(StagedFiles().size(), staged_before);
  EXPECT_EQ(ReadFile(dir_.filePath("notes.txt")), "short lived");
}

TEST_F(FileWorkspaceTest, ABatchGoesOnPastAFailure) {
  const auto a = Add("first", "a.txt");
  const auto b = Add("second", "b.txt");
  const auto c = Add("third", "c.txt");
  ASSERT_TRUE(QFile::remove(dir_.filePath("b.txt")));

  QStringList order;
  RunOnMainThread([this, &order]() {
    QObject::connect(files_, &FileWorkspace::SignalFinished, files_,
                     [this, &order](int id, bool) {
                       order << files_->entry(id).value("name").toString();
                     });
  });
  Do([&](auto* f) { f->encrypt({a, b, c}, {fred_}); });
  Finish();

  EXPECT_EQ(order, (QStringList{"a.txt", "b.txt", "c.txt"}));
  EXPECT_EQ(State(a), "done");
  EXPECT_EQ(State(b), "failed");
  EXPECT_EQ(State(c), "done");
  EXPECT_GE(OutputOf(a), 0);
  EXPECT_LT(OutputOf(b), 0);
  EXPECT_GE(OutputOf(c), 0);
}

TEST_F(FileWorkspaceTest, CancellingTheQueueLeavesTheRunningOne) {
  const auto a = Add("first", "a.txt");
  const auto b = Add("second", "b.txt");
  const auto c = Add("third", "c.txt");

  // To a passphrase: the first waits at its prompt while the others queue.
  Do([&](auto* f) { f->encrypt({a, b, c}, {}, {}, true); });
  ASSERT_FALSE(WaitForRequest().isEmpty());
  EXPECT_EQ(State(a), "running");
  EXPECT_EQ(State(b), "queued");
  Do([](auto* f) { f->cancelQueued(); });
  EXPECT_EQ(State(b), "cancelled");
  EXPECT_EQ(State(c), "cancelled");

  Answer("batch passphrase");
  Finish();
  EXPECT_EQ(State(a), "done");
  EXPECT_GE(OutputOf(a), 0);
  EXPECT_LT(OutputOf(b), 0);
  EXPECT_LT(OutputOf(c), 0);
}

TEST_F(FileWorkspaceTest, StoppingOneBatchLeavesTheOthersWaiting) {
  const auto a = Add("first", "a.txt");
  const auto b = Add("second", "b.txt");
  const auto c = Add("third", "c.txt");

  // Two batches: the first waits at its prompt, the second behind it.
  Do([&](auto* f) { f->encrypt({a, b}, {}, {}, true); });
  ASSERT_FALSE(WaitForRequest().isEmpty());
  Do([&](auto* f) { f->encrypt({c}, {}, {}, true); });
  const auto first = RecordOfFile(a).value("batchId").toInt();
  ASSERT_NE(RecordOfFile(c).value("batchId").toInt(), first);
  EXPECT_EQ(State(b), "queued");
  EXPECT_EQ(State(c), "queued");

  // Stopped from the first batch's sheet: only its own.
  Do([&](auto* f) { f->cancelQueued(first); });
  EXPECT_EQ(State(b), "cancelled");
  EXPECT_EQ(State(c), "queued");

  Answer("batch passphrase");
  ASSERT_FALSE(WaitForRequest().isEmpty());
  Answer("second passphrase");
  Finish();
  EXPECT_EQ(State(a), "done");
  EXPECT_EQ(State(c), "done");
}

// Which file is the signature and which the data: what is known of a
// signature made here, else what the names say; certain only when there is
// exactly one of each.
TEST_F(FileWorkspaceTest, ASignatureIsPairedWithItsFile) {
  const auto data = Add("signed content", "report.txt");
  Do([&](auto* f) { f->sign({data}, fred_); });
  Finish();
  const auto signature = OutputOf(data);
  ASSERT_GE(signature, 0) << Errors(data);

  QVariantMap from_signature;
  QVariantMap from_data;
  QVariantMap combined;
  const auto inline_signed = Add("-----BEGIN PGP MESSAGE-----", "notes.gpg");
  Do([&](auto* f) {
    from_signature = f->pairFor(signature);
    from_data = f->pairFor(data);
    combined = f->pairFor(inline_signed);
  });
  EXPECT_EQ(from_signature.value("signatureId").toInt(), signature);
  EXPECT_EQ(from_signature.value("originalId").toInt(), data);
  EXPECT_EQ(from_signature.value("originalHow").toString(), "linked");
  EXPECT_TRUE(from_signature.value("unique").toBool());
  EXPECT_EQ(from_data.value("signatureId").toInt(), signature);
  EXPECT_EQ(from_data.value("signatureHow").toString(), "linked");
  EXPECT_TRUE(combined.value("inline").toBool());

  // Two files a name points to: suggested, never taken as certain.
  const auto sig = Add("not really a signature", "letter.txt.sig");
  Add("one", "letter.txt");
  QDir().mkpath(dir_.filePath("other"));
  WriteFile(dir_.filePath("other/letter.txt"), "two");
  RunOnMainThread([&]() {
    files_->addFiles({QUrl::fromLocalFile(dir_.filePath("other/letter.txt"))});
  });
  QVariantMap ambiguous;
  Do([&](auto* f) { ambiguous = f->pairFor(sig); });
  EXPECT_EQ(ambiguous.value("originalHow").toString(), "suggested");
  EXPECT_FALSE(ambiguous.value("unique").toBool());
}

TEST_F(FileWorkspaceTest, ADetachedSignatureVerifiesOnlyOverItsFile) {
  const auto data = Add("signed content", "contract.txt");
  Do([&](auto* f) { f->sign({data}, fred_); });
  Finish();
  const auto signature = OutputOf(data);
  ASSERT_GE(signature, 0) << Errors(data);
  EXPECT_EQ(Entry(signature).value("name").toString(), "contract.txt.sig");
  EXPECT_FALSE(Entry(signature).value("plaintext").toBool());

  const auto count = Count();
  Do([&](auto* f) { f->verify(signature, data); });
  Finish();
  EXPECT_EQ(State(signature), "done") << Errors(signature);
  auto signatures =
      Entry(signature).value("result").toMap().value("signatures").toList();
  ASSERT_FALSE(signatures.isEmpty());
  EXPECT_TRUE(signatures.first().toMap().value("good").toBool());
  // And the desktop Status Panel's cards for it: the signature, its verdict
  // in the title, ok, with who signed and the key ID among its facts.
  const auto cards =
      Entry(signature).value("result").toMap().value("cards").toList();
  ASSERT_FALSE(cards.isEmpty());
  const auto card = cards.first().toMap();
  EXPECT_TRUE(card.value("title").toString().startsWith("Signature"))
      << card.value("title").toString().toStdString();
  EXPECT_EQ(card.value("status").toString(), "ok");
  QStringList labels;
  for (const auto& field : card.value("fields").toList()) {
    labels << field.toMap().value("label").toString();
    // Each fact says which run it belongs to, as the desktop sets them apart.
    EXPECT_TRUE(field.toMap().contains("group"));
  }
  EXPECT_TRUE(labels.contains("Key ID")) << labels.join(", ").toStdString();
  // A check makes nothing.
  EXPECT_EQ(Count(), count);

  const auto other = Add("other content", "other.txt");
  Do([&](auto* f) { f->verify(signature, other); });
  Finish();
  signatures =
      Entry(signature).value("result").toMap().value("signatures").toList();
  EXPECT_TRUE(State(signature) == "failed" || signatures.isEmpty() ||
              !signatures.first().toMap().value("good").toBool());
}

TEST_F(FileWorkspaceTest, ADocumentThatGoesAwayIsUnavailable) {
  const auto source = Add("here for now");
  EXPECT_TRUE(Entry(source).value("available").toBool());
  // The same document is not added twice.
  Do([this](auto* f) {
    f->addFiles({QUrl::fromLocalFile(dir_.filePath("notes.txt"))});
  });
  EXPECT_EQ(Count(), 1);

  ASSERT_TRUE(QFile::remove(dir_.filePath("notes.txt")));
  Do([](auto* f) { f->refreshAvailability(); });
  EXPECT_FALSE(Entry(source).value("available").toBool());
  // Nothing is started on it.
  Do([&](auto* f) { f->encrypt({source}, {fred_}); });
  EXPECT_EQ(State(source), "idle");
}

namespace {

/// Every file under @p root, by its path under it, and its SHA-256; and every
/// folder, by its path, with an empty digest.
auto TreeOf(const QString& root) -> QMap<QString, QByteArray> {
  QMap<QString, QByteArray> tree;
  QDirIterator walk(root,
                    QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden,
                    QDirIterator::Subdirectories);
  while (walk.hasNext()) {
    const auto info = walk.nextFileInfo();
    const auto path = QDir(root).relativeFilePath(info.absoluteFilePath());
    if (info.isDir()) {
      tree.insert(path, {});
      continue;
    }
    QFile file(info.absoluteFilePath());
    file.open(QIODevice::ReadOnly);
    tree.insert(path, QCryptographicHash::hash(file.readAll(),
                                               QCryptographicHash::Sha256));
  }
  return tree;
}

}  // namespace

// A folder is one item: everything in it counted, a folder already here not
// added twice, one that cannot be listed said so. It is only ever encrypted.
TEST_F(FileWorkspaceTest, AFolderIsOneItem) {
  QTemporaryDir parent;
  const auto root = parent.filePath("Holiday");
  ASSERT_TRUE(QDir().mkpath(root + "/photos/raw"));
  ASSERT_TRUE(QDir().mkpath(root + "/empty"));
  WriteFile(root + "/notes.txt", "one");
  WriteFile(root + "/photos/a.jpg", "two!");
  WriteFile(root + "/photos/raw/b.dng", "three");

  QVariantList outcome;
  const auto add = [&](const QUrl& folder) {
    RunOnMainThread([&]() {
      outcome.clear();
      QObject::connect(
          files_, &FileWorkspace::folderAdded, files_,
          [&outcome](int id, const QString& problem) {
            outcome = {id, problem};
          },
          Qt::SingleShotConnection);
      files_->addFolder(folder);
    });
    EXPECT_TRUE(WaitFor(
        [&outcome]() {
          bool done = false;
          RunOnMainThread([&]() { done = !outcome.isEmpty(); });
          return done;
        },
        10000));
  };

  add(QUrl::fromLocalFile(parent.filePath("missing")));
  EXPECT_EQ(outcome.value(0).toInt(), -1);
  EXPECT_EQ(outcome.value(1).toString(), "unreadable");
  EXPECT_EQ(Count(), 0);

  add(QUrl::fromLocalFile(root));
  const auto id = outcome.value(0).toInt();
  ASSERT_GE(id, 0);
  EXPECT_EQ(outcome.value(1).toString(), "");
  EXPECT_EQ(Count(), 1);
  const auto entry = Entry(id);
  EXPECT_EQ(entry.value("name").toString(), "Holiday");
  EXPECT_TRUE(entry.value("isFolder").toBool());
  EXPECT_EQ(entry.value("fileCount").toInt(), 3);
  EXPECT_EQ(entry.value("size").toLongLong(), 3 + 4 + 5);

  // Picked again: the same item.
  add(QUrl::fromLocalFile(root));
  EXPECT_EQ(outcome.value(0).toInt(), id);
  EXPECT_EQ(Count(), 1);

  QVariantMap type;
  bool encrypt = false;
  bool sign = false;
  bool decrypt = false;
  Do([&](auto* f) {
    type = f->typeOf(id);
    const QVariantList entries{f->entry(id)};
    encrypt = f->allowsEntries("encrypt", entries) &&
              f->allowsEntries("symmetric", entries);
    sign = f->allowsEntries("sign", entries);
    decrypt = f->allowsEntries("decrypt", entries) ||
              f->allowsEntries("verify", entries);
  });
  EXPECT_EQ(type.value("kind").toString(), "folder");
  EXPECT_TRUE(encrypt);
  EXPECT_FALSE(sign);
  EXPECT_FALSE(decrypt);
}

// A folder whose name ends in .tar already is not named ".tar.tar".
TEST_F(FileWorkspaceTest, AFolderCalledTarIsNotNamedTwice) {
  QTemporaryDir parent;
  const auto root = parent.filePath("backup.tar");
  ASSERT_TRUE(QDir().mkpath(root));
  WriteFile(root + "/README", "read me");
  int folder = -1;
  RunOnMainThread([&]() {
    QObject::connect(
        files_, &FileWorkspace::folderAdded, files_,
        [&folder](int id, const QString&) { folder = id; },
        Qt::SingleShotConnection);
    files_->addFolder(QUrl::fromLocalFile(root));
  });
  ASSERT_TRUE(WaitFor(
      [&folder]() {
        int id = -1;
        RunOnMainThread([&]() { id = folder; });
        return id >= 0;
      },
      10000));
  Do([&](auto* f) { f->encrypt({folder}, {fred_}); });
  Finish();
  const auto encrypted = OutputOf(folder);
  ASSERT_GE(encrypted, 0) << Errors(folder);
  EXPECT_EQ(Entry(encrypted).value("name").toString(), "backup.tar.gpg");
}

// A folder encrypted is one .tar.gpg of all in it, nested and empty folders
// too; decrypted, it is a TAR archive, which goes back into a folder only
// when asked, the same tree, file for file.
TEST_F(FileWorkspaceTest, AFolderEncryptsToOneArchiveThatExtractsBack) {
  QTemporaryDir parent;
  const auto root = parent.filePath("Project");
  ASSERT_TRUE(QDir().mkpath(root + "/src/deep/er"));
  ASSERT_TRUE(QDir().mkpath(root + "/empty/also empty"));
  WriteFile(root + "/README", "read me");
  WriteFile(root + "/src/main.c", "int main() {}");
  WriteFile(root + "/src/deep/er/Ünïcode name.txt", "unicode");
  QByteArray large(8 * 1024 * 1024, Qt::Uninitialized);
  for (qsizetype i = 0; i < large.size(); ++i) {
    large[i] = static_cast<char>((i * 7919) % 251);
  }
  WriteFile(root + "/src/large.bin", large);

  int folder = -1;
  RunOnMainThread([&]() {
    QObject::connect(
        files_, &FileWorkspace::folderAdded, files_,
        [&folder](int id, const QString&) { folder = id; },
        Qt::SingleShotConnection);
    files_->addFolder(QUrl::fromLocalFile(root));
  });
  ASSERT_TRUE(WaitFor(
      [&folder]() {
        int id = -1;
        RunOnMainThread([&]() { id = folder; });
        return id >= 0;
      },
      10000));

  Do([&](auto* f) { f->encrypt({folder}, {fred_}); });
  Finish();
  const auto encrypted = OutputOf(folder);
  ASSERT_GE(encrypted, 0) << Errors(folder);
  EXPECT_EQ(Entry(encrypted).value("name").toString(), "Project.tar.gpg");
  EXPECT_EQ(State(folder), "done");
  // Nothing of the packing is left behind but the encrypted archive.
  for (const auto& staged : StagedFiles()) {
    EXPECT_FALSE(staged.endsWith(".tar")) << staged.toStdString();
  }
  QVariantMap type;
  Do([&](auto* f) { type = f->typeOf(encrypted); });
  EXPECT_EQ(type.value("kind").toString(), "archive");
  EXPECT_TRUE(type.value("verified").toBool());

  Do([&](auto* f) { f->decrypt({encrypted}); });
  Finish();
  const auto decrypted = OutputOf(encrypted);
  ASSERT_GE(decrypted, 0) << Errors(encrypted);
  EXPECT_EQ(Entry(decrypted).value("name").toString(), "Project.tar");
  bool archive = false;
  Do([&](auto* f) { archive = f->isArchive(decrypted); });
  EXPECT_TRUE(archive);
  Do([&](auto* f) { type = f->typeOf(decrypted); });
  EXPECT_EQ(type.value("kind").toString(), "tar");
  EXPECT_TRUE(type.value("verified").toBool());

  QTemporaryDir destination;
  QVariantList outcome;
  bool started = false;
  RunOnMainThread([&]() {
    QObject::connect(
        files_, &FileWorkspace::extracted, files_,
        [&outcome](int, bool ok, const QString& name, const QString& reason,
                   int count) { outcome = {ok, name, reason, count}; },
        Qt::SingleShotConnection);
    started =
        files_->extract(decrypted, QUrl::fromLocalFile(destination.path()));
  });
  ASSERT_TRUE(started);
  ASSERT_TRUE(WaitFor(
      [&outcome]() {
        bool done = false;
        RunOnMainThread([&]() { done = !outcome.isEmpty(); });
        return done;
      },
      30000));
  ASSERT_TRUE(outcome.value(0).toBool())
      << outcome.value(2).toString().toStdString();
  EXPECT_EQ(outcome.value(1).toString(), "Project");
  EXPECT_EQ(outcome.value(3).toInt(), 4);
  EXPECT_EQ(TreeOf(destination.filePath("Project")), TreeOf(root));

  // Extracted again: into a folder of its own, never over the first.
  RunOnMainThread([&]() {
    outcome.clear();
    QObject::connect(
        files_, &FileWorkspace::extracted, files_,
        [&outcome](int, bool ok, const QString& name, const QString& reason,
                   int count) { outcome = {ok, name, reason, count}; },
        Qt::SingleShotConnection);
    files_->extract(decrypted, QUrl::fromLocalFile(destination.path()));
  });
  ASSERT_TRUE(WaitFor(
      [&outcome]() {
        bool done = false;
        RunOnMainThread([&]() { done = !outcome.isEmpty(); });
        return done;
      },
      30000));
  EXPECT_TRUE(outcome.value(0).toBool());
  EXPECT_EQ(outcome.value(1).toString(), "Project (1)");
}

// A file in the folder that cannot be read fails the whole archive, saying
// which: it is never left out without a word, and nothing is made.
TEST_F(FileWorkspaceTest, AFolderWithAFileThatCannotBeReadIsNotEncrypted) {
  QTemporaryDir parent;
  const auto root = parent.filePath("Locked");
  ASSERT_TRUE(QDir().mkpath(root + "/inner"));
  WriteFile(root + "/fine.txt", "fine");
  WriteFile(root + "/inner/secret.txt", "secret");
  QFile::setPermissions(root + "/inner/secret.txt", QFileDevice::Permissions());
  if (QFile(root + "/inner/secret.txt").open(QIODevice::ReadOnly)) {
    GTEST_SKIP() << "running as a user who reads anything";
  }
  int folder = -1;
  RunOnMainThread([&]() {
    QObject::connect(
        files_, &FileWorkspace::folderAdded, files_,
        [&folder](int id, const QString&) { folder = id; },
        Qt::SingleShotConnection);
    files_->addFolder(QUrl::fromLocalFile(root));
  });
  ASSERT_TRUE(WaitFor(
      [&folder]() {
        int id = -1;
        RunOnMainThread([&]() { id = folder; });
        return id >= 0;
      },
      10000));
  Do([&](auto* f) { f->encrypt({folder}, {fred_}); });
  Finish();
  EXPECT_EQ(State(folder), "failed");
  EXPECT_LT(OutputOf(folder), 0);
  // Said in the result, by its path in the folder.
  const auto said = QJsonDocument(QJsonObject::fromVariantMap(
                                      Entry(folder).value("result").toMap()))
                        .toJson();
  EXPECT_TRUE(said.contains("inner/secret.txt")) << said.toStdString();
  for (const auto& staged : StagedFiles()) {
    EXPECT_FALSE(staged.endsWith(".tar")) << staged.toStdString();
  }
  QFile::setPermissions(root + "/inner/secret.txt",
                        QFileDevice::ReadOwner | QFileDevice::WriteOwner);
}

TEST_F(FileWorkspaceTest, SortedByNameSizeOrWhenAdded) {
  Add("bb", "b.txt");
  Add("a", "a.txt");
  Add("ccc", "c.txt");
  const auto names = [this]() {
    QStringList names;
    RunOnMainThread([this, &names]() {
      for (int row = 0; row < files_->rowCount(); ++row) {
        names << files_->data(files_->index(row), FileWorkspace::kNameRole)
                     .toString();
      }
    });
    return names;
  };
  EXPECT_EQ(names(), (QStringList{"c.txt", "a.txt", "b.txt"}));
  Do([](auto* f) { f->SetSortKey("name"); });
  EXPECT_EQ(names(), (QStringList{"a.txt", "b.txt", "c.txt"}));
  Do([](auto* f) { f->SetSortKey("size"); });
  EXPECT_EQ(names(), (QStringList{"c.txt", "b.txt", "a.txt"}));
}

TEST_F(FileWorkspaceTest, SelectionIsKeptPerFile) {
  const auto a = Add("first", "a.txt");
  const auto b = Add("second", "b.txt");
  QSignalSpy* spy = nullptr;
  RunOnMainThread([this, &spy]() {
    spy = new QSignalSpy(files_, &FileWorkspace::SignalSelectionChanged);
  });
  Do([&](auto* f) { f->setSelected(a, true); });
  RunOnMainThread([this, a]() {
    EXPECT_EQ(files_->SelectedCount(), 1);
    EXPECT_EQ(files_->selectedIds(), QVariantList{a});
  });
  Do([](auto* f) { f->selectAll(); });
  RunOnMainThread([this]() { EXPECT_EQ(files_->SelectedCount(), 2); });
  // A file that is removed is no longer selected.
  Do([&](auto* f) { f->remove({b}); });
  RunOnMainThread([this]() { EXPECT_EQ(files_->SelectedCount(), 1); });
  Do([](auto* f) { f->clearSelection(); });
  RunOnMainThread([this, &spy]() {
    EXPECT_EQ(files_->SelectedCount(), 0);
    EXPECT_GE(spy->count(), 3);
    delete spy;
  });
}

// On the return to the foreground every picked file is looked at again --
// off the interface thread, a cloud document may be fetched to open it.
TEST_F(FileWorkspaceTest, AFileGoneWhileAwayIsNoticedOnTheReturn) {
  const auto a = Add("first", "a.txt");
  ASSERT_TRUE(Entry(a).value("available").toBool());
  ASSERT_TRUE(QFile::remove(dir_.filePath("a.txt")));
  RunOnMainThread([]() {
    auto* lifecycle =
        GpgFrontend::Mobile::AppLifecycle::create(nullptr, nullptr);
    lifecycle->OnApplicationStateChanged(Qt::ApplicationSuspended);
    lifecycle->OnApplicationStateChanged(Qt::ApplicationActive);
  });
  EXPECT_TRUE(
      WaitFor([&]() { return !Entry(a).value("available").toBool(); }, 10000));
}

// Returning to the foreground is one of the store's chances to clean up.
TEST(SharedFilesTest, ReturningToTheForegroundCleansWhatIsDue) {
  RunOnMainThread([]() {
    auto* store = SharedFileStore();
    auto* lifecycle =
        GpgFrontend::Mobile::AppLifecycle::create(nullptr, nullptr);
    ASSERT_NE(store, nullptr);
    ASSERT_NE(lifecycle, nullptr);

    const auto staged =
        QStandardPaths::writableLocation(QStandardPaths::CacheLocation) +
        "/due-staged.txt";
    WriteFile(staged, "shared two days ago");
    const auto due = store->Retain(staged, "due.txt", true,
                                   QDateTime::currentDateTimeUtc().addDays(-2));
    ASSERT_TRUE(due.has_value());
    ASSERT_TRUE(QFile::exists(due->path));

    lifecycle->OnApplicationStateChanged(Qt::ApplicationSuspended);
    EXPECT_TRUE(QFile::exists(due->path));  // nothing runs while away
    lifecycle->OnApplicationStateChanged(Qt::ApplicationActive);
    EXPECT_FALSE(QFile::exists(due->path));
  });
}

TEST_F(FileWorkspaceTest, EachFileIsARecordOfOneBatch) {
  const auto a = Add("first", "a.txt");
  const auto b = Add("second", "b.txt");
  Do([&](auto* f) { f->encrypt({a, b}, {fred_}); });
  Finish();

  const auto first = RecordOfFile(a);
  const auto second = RecordOfFile(b);
  ASSERT_FALSE(first.isEmpty());
  ASSERT_FALSE(second.isEmpty());
  EXPECT_NE(first.value("operationId"), second.value("operationId"));
  EXPECT_EQ(first.value("batchId"), first.value("operationId"));
  EXPECT_EQ(second.value("batchId"), first.value("operationId"));
  EXPECT_EQ(first.value("surface").toString(), "files");
  EXPECT_EQ(first.value("phase").toString(), "done");
  EXPECT_EQ(first.value("recipients").toStringList(), QStringList{fred_});
  EXPECT_FALSE(first.value("symmetric").toBool());
  EXPECT_EQ(first.value("targets").toList().first().toMap().value("name"),
            "a.txt");
  // Its output, by the id the page shows it under.
  EXPECT_EQ(first.value("outputs").toList(), (QVariantList{OutputOf(a)}));
}

TEST_F(FileWorkspaceTest, APromptIsForTheOperationThatRuns) {
  const auto a = Add("first", "a.txt");
  Do([&](auto* f) { f->encrypt({a}, {}, {}, true); });
  const auto request = WaitForRequest();
  ASSERT_FALSE(request.isEmpty());
  const auto record = RecordOfFile(a);
  EXPECT_EQ(request.value("operationId"), record.value("operationId"));
  EXPECT_EQ(request.value("operation").toString(), "encrypt");
  EXPECT_EQ(record.value("phase").toString(), "waiting_passphrase");
  EXPECT_TRUE(record.value("symmetric").toBool());
  Decline();
  Finish();
  EXPECT_EQ(RecordOfFile(a).value("phase").toString(), "cancelled");
}

TEST_F(FileWorkspaceTest, TextWaitsForTheRunningFileThenGoesFirst) {
  const auto a = Add("first", "a.txt");
  const auto b = Add("second", "b.txt");
  QList<int> started;
  TextCryptoController* text = nullptr;
  RunOnMainThread([&]() {
    text = new TextCryptoController();
    QObject::connect(OperationQueue::Instance(), &OperationQueue::SignalStarted,
                     text, [&started](int id) { started << id; });
  });

  // To a passphrase: the first file waits at its prompt, the second behind it.
  Do([&](auto* f) { f->encrypt({a, b}, {}, {}, true); });
  ASSERT_FALSE(WaitForRequest().isEmpty());
  RunOnMainThread([&]() { text->encrypt("for fred", {fred_}); });
  const auto message = FindRecord([](const QVariantMap& record) {
    return record.value("surface").toString() == "text";
  });
  EXPECT_EQ(message.value("phase").toString(), "queued");
  EXPECT_EQ(message.value("targets").toList().first().toMap().value("fileId"),
            -1);

  // Every prompt answered as it comes, until all is done.
  EXPECT_TRUE(WaitFor(
      [&]() {
        if (!CurrentRequest().isEmpty()) Answer("batch passphrase");
        bool busy = true;
        RunOnMainThread([&]() { busy = files_->Busy() || text->Busy(); });
        return !busy;
      },
      30000));

  int text_id = message.value("operationId").toInt();
  QList<int> expected = {RecordOfFile(a).value("operationId").toInt(), text_id,
                         RecordOfFile(b).value("operationId").toInt()};
  RunOnMainThread([&]() {
    EXPECT_EQ(started, expected);
    EXPECT_EQ(text->Result().value("status").toInt(), 1);
    delete text;
  });
}

TEST_F(FileWorkspaceTest, NoRecipientsIsNeverAPassphrase) {
  const auto a = Add("first", "a.txt");
  bool started = true;
  Do([&](auto* f) { started = f->encrypt({a}, {}); });
  EXPECT_FALSE(started);
  EXPECT_EQ(State(a), "idle");
  Do([&](auto* f) { EXPECT_FALSE(f->Busy()); });
}

TEST_F(FileWorkspaceTest, RetryingTheFailedOfABatchRunsOnlyThose) {
  const auto a = Add("first", "a.txt");
  const auto b = Add("second", "b.txt");
  ASSERT_TRUE(QFile::remove(dir_.filePath("b.txt")));
  Do([&](auto* f) { f->encrypt({a, b}, {fred_}); });
  Finish();
  ASSERT_EQ(State(a), "done");
  ASSERT_EQ(State(b), "failed");
  const auto batch = RecordOfFile(a).value("batchId").toInt();
  const auto failed = RecordOfFile(b).value("operationId").toInt();

  // Gone from the device: picked again, it is the same entry.
  QString why;
  Do([&](auto* f) {
    f->refreshAvailability();
    why = f->retry(failed);
  });
  EXPECT_EQ(why, "unavailable");
  WriteFile(dir_.filePath("b2.txt"), "second, again");
  bool relinked = false;
  Do([&](auto* f) {
    relinked = f->relink(b, QUrl::fromLocalFile(dir_.filePath("b2.txt")));
  });
  ASSERT_TRUE(relinked);
  EXPECT_EQ(Entry(b).value("name").toString(), "b2.txt");

  int queued = 0;
  Do([&](auto* f) { queued = f->retryFailed(batch); });
  EXPECT_EQ(queued, 1);
  Finish();
  EXPECT_EQ(State(b), "done") << Errors(b);
  const auto again = RecordOfFile(b);
  EXPECT_NE(again.value("operationId").toInt(), failed);
  EXPECT_EQ(again.value("recipients").toStringList(), QStringList{fred_});
  // a was not run again.
  EXPECT_EQ(RecordOfFile(a).value("batchId").toInt(), batch);
}

// Run again together, the failed files are one batch again: the status sheet
// follows that batch, not only the last of them.
TEST_F(FileWorkspaceTest, TheFailedRunAgainAsOneBatch) {
  const auto a = Add("first", "a.txt");
  const auto b = Add("second", "b.txt");
  // Gone from the device before they are read, unnoticed: both fail.
  ASSERT_TRUE(QFile::remove(dir_.filePath("a.txt")));
  ASSERT_TRUE(QFile::remove(dir_.filePath("b.txt")));
  Do([&](auto* f) { f->encrypt({a, b}, {fred_}); });
  Finish();
  ASSERT_EQ(State(a), "failed");
  ASSERT_EQ(State(b), "failed");
  const auto failed_batch = RecordOfFile(a).value("batchId").toInt();
  ASSERT_EQ(RecordOfFile(b).value("batchId").toInt(), failed_batch);
  WriteFile(dir_.filePath("a.txt"), "first");
  WriteFile(dir_.filePath("b.txt"), "second");

  int queued = 0;
  Do([&](auto* f) { queued = f->retryFailed(failed_batch); });
  EXPECT_EQ(queued, 2);
  Finish();
  const auto again_a = RecordOfFile(a).value("batchId").toInt();
  const auto again_b = RecordOfFile(b).value("batchId").toInt();
  EXPECT_NE(again_a, failed_batch);
  EXPECT_EQ(again_a, again_b);
}

// Nothing to queue (each file waits or runs already): said so, never taken
// for an operation that started.
TEST_F(FileWorkspaceTest, AStartThatQueuesNothingSaysSo) {
  const auto a = Add("first", "a.txt");
  ASSERT_TRUE(QFile::remove(dir_.filePath("a.txt")));
  bool started = true;
  Do([&](auto* f) {
    f->refreshAvailability();
    started = f->decrypt({a});
  });
  EXPECT_FALSE(started);
  Do([&](auto* f) { started = f->sign({a}, fred_); });
  EXPECT_FALSE(started);
  Do([&](auto* f) { started = f->verify(a); });
  EXPECT_FALSE(started);
  Do([&](auto* f) { started = f->encrypt({a}, {fred_}); });
  EXPECT_FALSE(started);
}

TEST_F(FileWorkspaceTest, ARetryWhoseFileIsGoneSaysSo) {
  const auto a = Add("first", "a.txt");
  Do([&](auto* f) { f->sign({a}, fred_); });
  Finish();
  const auto signed_op = RecordOfFile(a).value("operationId").toInt();
  QString why;
  Do([&](auto* f) {
    f->remove({a});
    why = f->retry(signed_op);
  });
  EXPECT_EQ(why, "missing");
  Do([&](auto* f) { why = f->retry(424242); });
  EXPECT_EQ(why, "unknown");
}

TEST_F(FileWorkspaceTest, NamesInTheListAreUniqueAndSavesAreNamed) {
  const auto source = Add("twice");
  Do([&](auto* f) { f->encrypt({source}, {fred_}); });
  Finish();
  Do([&](auto* f) { f->encrypt({source}, {fred_}); });
  Finish();
  QStringList names;
  RunOnMainThread([&]() {
    for (int row = 0; row < files_->rowCount(); ++row) {
      names << files_->data(files_->index(row), FileWorkspace::kNameRole)
                   .toString();
    }
  });
  EXPECT_TRUE(names.contains("notes.txt.gpg"))
      << names.join(", ").toStdString();
  EXPECT_TRUE(names.contains("notes.txt (2).gpg"))
      << names.join(", ").toStdString();
  EXPECT_EQ(Entry(source).value("origin").toString(), "added");

  const auto output = OutputOf(source);
  EXPECT_EQ(Entry(output).value("origin").toString(), "made");
  EXPECT_TRUE(Entry(output).value("savedName").toString().isEmpty());
  ASSERT_FALSE(SaveAndRead(output, "kept.gpg").isEmpty());
  EXPECT_EQ(Entry(output).value("savedName").toString(), "kept.gpg");
}

TEST_F(FileWorkspaceTest, FilesAreFoundByRowNameAndWhatTheyMade) {
  const auto data = Add("signed content", "contract.txt");
  Do([&](auto* f) {
    EXPECT_EQ(f->idAt(0), data);
    EXPECT_EQ(f->idAt(5), -1);
    EXPECT_EQ(f->rowOf(data), 0);
    EXPECT_EQ(f->idNamed("contract.txt"), data);
    EXPECT_EQ(f->idNamed("nothing"), -1);
    EXPECT_EQ(f->signatureOf(data), -1);
    EXPECT_EQ(f->UnsavedCount(), 0);
  });
  Do([&](auto* f) { f->sign({data}, fred_); });
  Finish();
  const auto signature = OutputOf(data);
  ASSERT_GE(signature, 0) << Errors(data);
  Do([&](auto* f) {
    EXPECT_EQ(f->signatureOf(data), signature);
    EXPECT_EQ(f->idNamed("contract.txt.sig"), signature);
    // Made here and not saved: goes with the session.
    EXPECT_EQ(f->UnsavedCount(), 1);
  });
  ASSERT_FALSE(SaveAndRead(signature, "kept.sig").isEmpty());
  Do([&](auto* f) { EXPECT_EQ(f->UnsavedCount(), 0); });
}

// Two files of one name, from two folders, are told apart in the list; what
// may be the file a signature is named after is each of them, never only the
// first. Picked again, a listed file is looked at again.
TEST_F(FileWorkspaceTest, FilesOfOneNameAreToldApart) {
  ASSERT_TRUE(QDir(dir_.path()).mkpath("other"));
  const auto first = Add("first notes", "notes.txt");
  const auto second = Add("other notes", "other/notes.txt");
  ASSERT_NE(first, second);
  EXPECT_EQ(Entry(first).value("name").toString(), "notes.txt");
  EXPECT_EQ(Entry(second).value("name").toString(), "notes (2).txt");
  Do([&](auto* f) {
    EXPECT_EQ(f->idNamed("notes.txt"), first);
    EXPECT_EQ(f->idsNamed("notes.txt"), (QVariantList{first, second}));
    EXPECT_EQ(f->idsNamed("notes (2).txt"), QVariantList{second});
    EXPECT_TRUE(f->idsNamed("nothing.txt").isEmpty());
  });

  // Gone, noticed, back: added again, it can be opened once more.
  const auto path = dir_.filePath("notes.txt");
  ASSERT_TRUE(QFile::remove(path));
  Do([&](auto* f) { f->refreshAvailability(); });
  EXPECT_FALSE(Entry(first).value("available").toBool());
  WriteFile(path, "first notes, again");
  QVariantList again;
  Do([&](auto* f) { again = f->addFiles({QUrl::fromLocalFile(path)}); });
  EXPECT_EQ(again, QVariantList{first});
  EXPECT_TRUE(Entry(first).value("available").toBool());
  EXPECT_EQ(Entry(first).value("name").toString(), "notes.txt");
}

// As the desktop tells files apart: by the name, nothing read.
// What a file is: its name a hint, its first bytes the proof, whether it
// was added or made here.
TEST_F(FileWorkspaceTest, WhatAFileIsComesFromItsNameAndItsBytes) {
  // Armored, it says what it is under any name.
  const auto letter =
      Add("-----BEGIN PGP MESSAGE-----\n\nhQ==\n", "letter.txt");
  EXPECT_EQ(Entry(letter).value("content").toString(), "message");
  // Named as OpenPGP data and not: nothing in particular.
  const auto named = Add("only text", "named.gpg");
  EXPECT_EQ(Entry(named).value("content").toString(), "");

  const auto source = Add("binary out", "data.txt");
  Do([&](auto* f) { f->encrypt({source}, {fred_}); });
  Finish();
  const auto encrypted = OutputOf(source);
  ASSERT_GE(encrypted, 0) << Errors(source);
  EXPECT_EQ(Entry(encrypted).value("content").toString(), "message");

  Do([&](auto* f) { f->sign({source}, fred_); });
  Finish();
  int signature = -1;
  Do([&](auto* f) { signature = f->signatureOf(source); });
  ASSERT_GE(signature, 0) << Errors(source);
  EXPECT_EQ(Entry(signature).value("content").toString(), "signature");
}

// What the details say a file is: what its bytes said, or what its name
// suggests, never more. An archive or a message is never vouched for by a
// name.
TEST_F(FileWorkspaceTest, AFilesTypeSaysHowItIsKnown) {
  const auto type = [this](int id) {
    QVariantMap map;
    Do([&](auto* f) { map = f->typeOf(id); });
    return std::make_pair(map.value("kind").toString(),
                          map.value("verified").toBool());
  };
  using Kind = std::pair<QString, bool>;
  // Armored text: its header says it, whatever it is called.
  EXPECT_EQ(type(Add("-----BEGIN PGP MESSAGE-----\n\nhQ==\n", "letter.txt")),
            Kind("message", true));
  // Named as OpenPGP data, and not: said so, not a type by its name.
  EXPECT_EQ(type(Add("only text", "named.gpg")), Kind("not_openpgp", false));
  // A picture called .gpg is not taken for OpenPGP data either.
  EXPECT_EQ(type(Add(QByteArray("\x89PNG\r\n\x1a\n", 8), "photo.gpg")).second,
            false);
  // An archive by its name, and only by its name.
  EXPECT_EQ(type(Add("not even OpenPGP", "photos.tar.gpg")),
            Kind("archive", false));
  EXPECT_EQ(type(Add("plain", "photos.tar")), Kind("tar", false));
  // Anything else: its type by its name, never verified.
  const auto pdf = Add("%PDF-1.7", "paper.pdf");
  QVariantMap map;
  Do([&](auto* f) { map = f->typeOf(pdf); });
  EXPECT_FALSE(map.value("verified").toBool());
  EXPECT_FALSE(map.value("mime").toString().isEmpty());

  // Made here: confirmed by what was made, an archive still only by name.
  const auto source = Add("binary out", "data.txt");
  Do([&](auto* f) { f->encrypt({source}, {fred_}); });
  Finish();
  const auto encrypted = OutputOf(source);
  ASSERT_GE(encrypted, 0) << Errors(source);
  EXPECT_EQ(type(encrypted), Kind("message", true));
  Do([&](auto* f) { f->sign({source}, fred_); });
  Finish();
  int signature = -1;
  Do([&](auto* f) { signature = f->signatureOf(source); });
  ASSERT_GE(signature, 0) << Errors(source);
  EXPECT_EQ(type(signature), Kind("signature", true));
}

TEST_F(FileWorkspaceTest, ASharedFileIsCopiedInAndIsTheApplicationsOwn) {
  // More than one piece of the copy, so it reports how far it is.
  const QByteArray content(3 * 1024 * 1024 + 17, 'x');
  const auto path = dir_.filePath("shared.bin");
  WriteFile(path, content);
  const auto staged_before = StagedFiles().size();

  QList<double> steps;
  int id = -1;
  RunOnMainThread([&]() {
    QObject::connect(
        files_, &QAbstractItemModel::dataChanged, files_,
        [&](const QModelIndex& at, const QModelIndex&,
            const QList<int>& roles) {
          if (roles.contains(FileWorkspace::kProgressRole)) {
            steps << at.data(FileWorkspace::kProgressRole).toDouble();
          }
        });
    const auto ids = files_->receiveFiles({QUrl::fromLocalFile(path)});
    ASSERT_EQ(ids.size(), 1);
    id = ids.first().toInt();
    const auto entry = files_->entry(id);
    EXPECT_EQ(entry.value("origin").toString(), "shared");
    EXPECT_EQ(entry.value("kind").toString(), "external");
    EXPECT_FALSE(entry.value("available").toBool());
    EXPECT_EQ(entry.value("progress").toDouble(), 0.0);
  });
  EXPECT_TRUE(
      WaitFor([&]() { return Entry(id).value("available").toBool(); }, 30000));
  const auto entry = Entry(id);
  EXPECT_EQ(entry.value("progress").toDouble(), -1.0);
  EXPECT_EQ(entry.value("size").toLongLong(), content.size());
  EXPECT_EQ(entry.value("content").toString(), "");
  EXPECT_TRUE(std::any_of(steps.begin(), steps.end(),
                          [](double step) { return step > 0 && step < 1; }));
  // Not a file made here: nothing to warn about when the session ends.
  Do([](auto* f) { EXPECT_EQ(f->UnsavedCount(), 0); });
  EXPECT_EQ(StagedFiles().size(), staged_before + 1);

  // It is worked on like any other.
  Do([&](auto* f) { f->encrypt({id}, {fred_}); });
  Finish();
  EXPECT_EQ(State(id), "done") << Errors(id);

  // Removed, the copy is deleted; the sender's document is not touched.
  Do([&](auto* f) { f->remove({id, OutputOf(id)}); });
  EXPECT_EQ(StagedFiles().size(), staged_before);
  EXPECT_TRUE(QFile::exists(path));
}

TEST_F(FileWorkspaceTest, ASharedFileThatCannotBeCopiedIsTakenOffAgain) {
  QString failed;
  RunOnMainThread([&]() {
    QObject::connect(files_, &FileWorkspace::SignalReceiveFailed, files_,
                     [&failed](const QString& name) { failed = name; });
    files_->receiveFiles({QUrl::fromLocalFile(dir_.filePath("gone.txt"))});
    EXPECT_EQ(files_->Count(), 1);
  });
  EXPECT_TRUE(WaitFor([&]() { return Count() == 0; }, 30000));
  EXPECT_EQ(failed, "gone.txt");
}

TEST_F(FileWorkspaceTest, TheListIsSearchedAndNarrowedByWhatFilesAre) {
  const auto plain = Add("plain words", "Report.txt");
  const auto armored =
      Add("-----BEGIN PGP MESSAGE-----\n\nhQ==\n", "mail.txt.asc");
  const auto signature =
      Add("-----BEGIN PGP SIGNATURE-----\n\niQ==\n", "report.txt.sig");
  Do([&](auto* f) { f->encrypt({plain}, {fred_}); });
  Finish();
  const auto made = OutputOf(plain);
  ASSERT_GE(made, 0);

  RunOnMainThread([&]() {
    FileFilterModel model;
    model.setSourceModel(files_);
    EXPECT_FALSE(model.Filtering());
    EXPECT_EQ(model.rowCount(), 4);

    model.SetSearchText("  report ");
    EXPECT_TRUE(model.Filtering());
    EXPECT_EQ(model.ids(), (QVariantList{made, signature, plain}));
    EXPECT_EQ(model.rowOf(armored), -1);

    // Searching is the only narrowing: no kinds of file to choose from.
    EXPECT_EQ(model.metaObject()->indexOfProperty("category"), -1);

    model.reset();
    EXPECT_FALSE(model.Filtering());
    EXPECT_EQ(model.rowCount(), 4);
    EXPECT_EQ(model.rowOf(made), 0);
  });

  // Saved, it is no longer counted among the unsaved.
  int unsaved_before = 0;
  RunOnMainThread([&]() { unsaved_before = files_->UnsavedCount(); });
  EXPECT_EQ(unsaved_before, 1);
  ASSERT_FALSE(SaveAndRead(made, "kept.gpg").isEmpty());
  RunOnMainThread([&]() { EXPECT_EQ(files_->UnsavedCount(), 0); });
}

// A file's SHA-256, worked out off the interface thread, read where the file
// is; an unknown file says so.
TEST_F(FileWorkspaceTest, AFilesHashIsWorkedOutAndSaid) {
  const QByteArray content = "hash me\n";
  const auto id = Add(content);
  QString hex;
  QString error = QStringLiteral("unset");
  int calls = 0;
  RunOnMainThread([&]() {
    QObject::connect(files_, &GpgFrontend::Mobile::FileWorkspace::hashReady,
                     files_,
                     [&](int got, const QString& sha256, const QString& why) {
                       if (got != id && got != 999999) return;
                       ++calls;
                       if (got == id) {
                         hex = sha256;
                         error = why;
                       } else {
                         EXPECT_EQ(why, "missing");
                         EXPECT_TRUE(sha256.isEmpty());
                       }
                     });
    // The unknown one is said at once; the file's then worked out.
    files_->hash(999999);
    files_->hash(id);
  });
  ASSERT_TRUE(WaitFor(
      [&]() {
        int n = 0;
        RunOnMainThread([&]() { n = calls; });
        return n >= 2;
      },
      10000));
  RunOnMainThread([&]() {
    EXPECT_TRUE(error.isEmpty());
    EXPECT_EQ(hex, QString::fromLatin1(QCryptographicHash::hash(
                                           content, QCryptographicHash::Sha256)
                                           .toHex()));
  });
}

// One hash at a time: asking again stops the one under way, which reports
// nothing, and so does cancelHash() (the sheet that asked is gone).
TEST_F(FileWorkspaceTest, ALaterHashOrACancelStopsTheOneUnderWay) {
  const QByteArray big(64 << 20, 'x');
  const auto big_id = Add(big, "big.bin");
  const auto small_id = Add("small\n", "small.txt");
  QList<int> reported;
  RunOnMainThread([&]() {
    QObject::connect(
        files_, &GpgFrontend::Mobile::FileWorkspace::hashReady, files_,
        [&](int got, const QString&, const QString&) { reported.append(got); });
    files_->hash(big_id);
    files_->hash(small_id);
  });
  ASSERT_TRUE(WaitFor(
      [&]() {
        bool done = false;
        RunOnMainThread([&]() { done = reported.contains(small_id); });
        return done;
      },
      20000));
  RunOnMainThread([&]() {
    EXPECT_FALSE(reported.contains(big_id));
    reported.clear();
    files_->hash(big_id);
    files_->cancelHash();
  });
  // Hashes run one at a time, in turn: had the cancelled one gone on, it
  // would report before this later one.
  RunOnMainThread([&]() { files_->hash(small_id); });
  ASSERT_TRUE(WaitFor(
      [&]() {
        bool done = false;
        RunOnMainThread([&]() { done = reported.contains(small_id); });
        return done;
      },
      20000));
  RunOnMainThread([&]() { EXPECT_EQ(reported, QList<int>{small_id}); });
}

TEST_F(FileWorkspaceTest, WhatIsSharedIsTakenOnceIntoTheApplicationsList) {
  const auto path = dir_.filePath("from-mail.asc");
  WriteFile(path, "-----BEGIN PGP MESSAGE-----\n\nhQ==\n");
  int calls = 0;
  ShareInbox::SetSourceForTesting([&]() {
    SharedItems items;
    if (calls++ == 0) {
      items.files = {QUrl::fromLocalFile(path)};
      items.text = "shared words";
      items.refused = 1;
    }
    return items;
  });

  QVariantMap got;
  RunOnMainThread([&]() {
    ShareInbox inbox;
    got = inbox.take();
    // Handed out once.
    EXPECT_TRUE(inbox.take().isEmpty());
  });
  ShareInbox::SetSourceForTesting({});
  EXPECT_EQ(got.value("text").toString(), "shared words");
  EXPECT_EQ(got.value("refused").toInt(), 1);
  const auto ids = got.value("files").toList();
  ASSERT_EQ(ids.size(), 1);

  // Into the application's workspace, as a copy.
  const auto id = ids.first().toInt();
  auto* files = FileWorkspace::Instance();
  ASSERT_NE(files, nullptr);
  EXPECT_TRUE(WaitFor(
      [&]() {
        bool available = false;
        RunOnMainThread([&]() {
          available = files->entry(id).value("available").toBool();
        });
        return available;
      },
      30000));
  RunOnMainThread([&]() {
    const auto entry = files->entry(id);
    EXPECT_EQ(entry.value("origin").toString(), "shared");
    EXPECT_EQ(entry.value("name").toString(), "from-mail.asc");
    EXPECT_EQ(entry.value("content").toString(), "message");
    files->remove({id});
  });
}

TEST_F(FileWorkspaceTest, SavingIntoAFolderAsksNothingButWhatIsTaken) {
  const auto a = Add("first", "a.txt");
  const auto b = Add("second", "b.txt");
  Do([&](auto* f) { f->encrypt({a, b}, {fred_}); });
  Finish();
  const auto out_a = OutputOf(a);
  const auto out_b = OutputOf(b);
  ASSERT_GE(out_a, 0);
  ASSERT_GE(out_b, 0);
  QTemporaryDir folder;
  const auto url = QUrl::fromLocalFile(folder.path());

  // Saves every one, in turn, and reports once.
  const auto save = [&](const QString& taken) {
    std::array<int, 3> counts{-1, -1, -1};
    RunOnMainThread([&]() {
      QObject::connect(
          files_, &FileWorkspace::SignalSavedAll, files_,
          [&counts](int saved, int skipped, int failed) {
            counts = {saved, skipped, failed};
          },
          Qt::SingleShotConnection);
      files_->saveAll({out_a, out_b}, url, taken);
    });
    EXPECT_TRUE(WaitFor(
        [&]() {
          int saved = -1;
          RunOnMainThread([&]() { saved = counts[0]; });
          return saved >= 0;
        },
        30000));
    return counts;
  };

  QStringList taken;
  Do([&](auto* f) { taken = f->takenIn({out_a, out_b}, url); });
  EXPECT_TRUE(taken.isEmpty());
  EXPECT_EQ(save("skip"), (std::array<int, 3>{2, 0, 0}));
  EXPECT_TRUE(QFile::exists(folder.filePath("a.txt.gpg")));
  EXPECT_TRUE(QFile::exists(folder.filePath("b.txt.gpg")));
  EXPECT_EQ(Entry(out_a).value("savedName").toString(), "a.txt.gpg");
  Do([](auto* f) { EXPECT_EQ(f->UnsavedCount(), 0); });

  // The names are there now: what to do with them is the caller's to say.
  Do([&](auto* f) { taken = f->takenIn({out_a, out_b, a}, url); });
  EXPECT_EQ(taken, (QStringList{"a.txt.gpg", "b.txt.gpg"}));
  EXPECT_EQ(save("skip"), (std::array<int, 3>{0, 2, 0}));
  EXPECT_EQ(save("keep_both"), (std::array<int, 3>{2, 0, 0}));
  EXPECT_TRUE(QFile::exists(folder.filePath("a.txt (1).gpg")));
  EXPECT_EQ(Entry(out_a).value("savedName").toString(), "a.txt (1).gpg");

  WriteFile(folder.filePath("b.txt.gpg"), "something else, and longer");
  EXPECT_EQ(save("replace"), (std::array<int, 3>{2, 0, 0}));
  EXPECT_EQ(ReadFile(folder.filePath("b.txt.gpg")),
            ReadFile(QFileInfo(folder.filePath("b.txt (1).gpg")).filePath()));
  EXPECT_EQ(Entry(out_b).value("savedName").toString(), "b.txt.gpg");
  // Replaced in place: no new number left beside it.
  EXPECT_EQ(QDir(folder.path()).entryList(QDir::Files).size(), 4);
}

TEST_F(FileWorkspaceTest, AnOutputBeingSavedIsNotRemovedFromUnderTheCopy) {
  const auto a = Add("first", "a.txt");
  Do([&](auto* f) { f->encrypt({a}, {fred_}); });
  Finish();
  const auto out = OutputOf(a);
  ASSERT_GE(out, 0);
  QTemporaryDir folder;
  const auto to = QUrl::fromLocalFile(folder.filePath("a.txt.gpg"));

  int saved = -1;
  RunOnMainThread([&]() {
    QObject::connect(
        files_, &FileWorkspace::SignalSaved, files_,
        [&saved](int, bool ok) { saved = ok ? 1 : 0; },
        Qt::SingleShotConnection);
    files_->saveAs(out, to);
    // At once, before the copy has run: it stays for the copy.
    files_->remove({out});
    EXPECT_FALSE(files_->entry(out).isEmpty());
  });
  ASSERT_TRUE(WaitFor(
      [&]() {
        int s = -1;
        RunOnMainThread([&]() { s = saved; });
        return s >= 0;
      },
      30000));
  EXPECT_EQ(saved, 1);
  EXPECT_EQ(ReadFile(folder.filePath("a.txt.gpg")).isEmpty(), false);
  // Done: it goes now.
  RunOnMainThread([&]() {
    files_->remove({out});
    EXPECT_TRUE(files_->entry(out).isEmpty());
  });
}

TEST_F(FileWorkspaceTest, AFileCanBeReadWhereItIs) {
  const auto a = Add("first", "a.txt");
  Do([&](auto* f) { f->encrypt({a}, {fred_}); });
  Finish();
  const auto out = OutputOf(a);
  ASSERT_GE(out, 0);
  QUrl added;
  QUrl made;
  RunOnMainThread([&]() {
    added = files_->readableUrl(a);
    made = files_->readableUrl(out);
    EXPECT_TRUE(files_->readableUrl(-1).isEmpty());
  });
  EXPECT_EQ(added, QUrl::fromLocalFile(dir_.filePath("a.txt")));
  // An output is the copy here: read from it, as a key import would.
  ASSERT_TRUE(made.isLocalFile());
  QFile file(made.toLocalFile());
  ASSERT_TRUE(file.open(QIODevice::ReadOnly));
  EXPECT_FALSE(file.readAll().isEmpty());
}

TEST(CopyLimitedTest, ACopyThatFailsLeavesTheFileThereAsItWas) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());
  const auto from = dir.filePath("from");
  const auto to = dir.filePath("to");
  {
    QFile in(from);
    ASSERT_TRUE(in.open(QIODevice::WriteOnly));
    in.write(QByteArray(64, 'n'));
    QFile out(to);
    ASSERT_TRUE(out.open(QIODevice::WriteOnly));
    out.write("kept");
  }
  // Over the limit part way: what was there stays, whole.
  EXPECT_FALSE(GpgFrontend::Mobile::CopyLimited(from, to, 16));
  QFile kept(to);
  ASSERT_TRUE(kept.open(QIODevice::ReadOnly));
  EXPECT_EQ(kept.readAll(), "kept");
  kept.close();
  EXPECT_EQ(QDir(dir.path()).entryList(QDir::Files).size(), 2);

  EXPECT_TRUE(GpgFrontend::Mobile::CopyLimited(from, to, 64));
  ASSERT_TRUE(kept.open(QIODevice::ReadOnly));
  EXPECT_EQ(kept.readAll(), QByteArray(64, 'n'));
}

TEST_F(FileWorkspaceTest, ASaveThatFailsLeavesWhatWasThereAndNothingEmpty) {
  const auto a = Add("first", "a.txt");
  Do([&](auto* f) { f->encrypt({a}, {fred_}); });
  Finish();
  const auto out = OutputOf(a);
  ASSERT_GE(out, 0);
  QTemporaryDir folder;
  const auto url = QUrl::fromLocalFile(folder.path());
  WriteFile(folder.filePath("a.txt.gpg"), "the one kept before");

  // The output can no longer be read: the copy fails part way.
  QString path;
  RunOnMainThread([&]() {
    QDirIterator walk(GpgFrontend::Mobile::FileStagingArea()->Directory(),
                      {"a.txt.gpg"}, QDir::Files, QDirIterator::Subdirectories);
    if (walk.hasNext()) path = walk.next();
  });
  ASSERT_FALSE(path.isEmpty());
  ASSERT_TRUE(QFile::setPermissions(path, QFileDevice::Permissions()));

  for (const auto* taken : {"replace", "keep_both"}) {
    std::array<int, 3> counts{-1, -1, -1};
    RunOnMainThread([&]() {
      QObject::connect(
          files_, &FileWorkspace::SignalSavedAll, files_,
          [&counts](int saved, int skipped, int failed) {
            counts = {saved, skipped, failed};
          },
          Qt::SingleShotConnection);
      files_->saveAll({out}, url, QString::fromLatin1(taken));
    });
    ASSERT_TRUE(WaitFor(
        [&]() {
          int saved = -1;
          RunOnMainThread([&]() { saved = counts[0]; });
          return saved >= 0;
        },
        30000));
    EXPECT_EQ(counts, (std::array<int, 3>{0, 0, 1})) << taken;
    EXPECT_EQ(ReadFile(folder.filePath("a.txt.gpg")), "the one kept before")
        << taken;
    EXPECT_EQ(QDir(folder.path()).entryList(QDir::Files),
              QStringList{"a.txt.gpg"})
        << taken;
  }
  QFile::setPermissions(path, QFileDevice::ReadOwner | QFileDevice::WriteOwner);
}

TEST_F(FileWorkspaceTest, OpeningWithoutAnotherAppSaysSo) {
  const auto source = Add("open me");
  Do([&](auto* f) { f->encrypt({source}, {fred_}); });
  Finish();
  const auto output = OutputOf(source);
  ASSERT_GE(output, 0);
  QString outcome;
  RunOnMainThread([&]() {
    QObject::connect(files_, &FileWorkspace::SignalOpened, files_,
                     [&outcome](int, const QString& o) { outcome = o; });
    QObject::connect(files_, &FileWorkspace::SignalShared, files_,
                     [](int, const QString&) { ADD_FAILURE(); });
    files_->openWith(output);
  });
  EXPECT_TRUE(WaitFor(
      [&]() {
        QString o;
        RunOnMainThread([&]() { o = outcome; });
        return !o.isEmpty();
      },
      30000));
  EXPECT_EQ(outcome, "unsupported");
}

}  // namespace GpgFrontend::Test::Mobile
