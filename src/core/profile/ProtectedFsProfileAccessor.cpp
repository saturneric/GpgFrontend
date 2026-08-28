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

#include "core/profile/ProtectedFsProfileAccessor.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonObject>
#include <QStorageInfo>
#include <array>

#include "core/GFCoreLog.h"
#include "core/profile/FscryptStorage.h"
#include "core/profile/MemoryAreaProfileAccessor.h"
#include "core/profile/ProfileAreaTraits.h"

#if defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)
#include <sys/stat.h>
#include <sys/statfs.h>
#include <unistd.h>
#endif

#ifdef Q_OS_MACOS
#include <sys/xattr.h>
#endif

#ifdef Q_OS_WINDOWS
#include <windows.h>
#endif

namespace GpgFrontend {

namespace {

/// Driver tokens. Stable, because they end up in logs, in the anchor pointer a
/// later process reads back, and in the status strip.
constexpr auto kDriverTmpfs = "fs-tmpfs";
constexpr auto kDriverFscrypt = "fs-fscrypt";
constexpr auto kDriverEfs = "fs-efs";
constexpr auto kDriverTemp = "fs-temp";
constexpr auto kDriverDisk = "fs";

/// Keys in the anchor pointer. Read by a later process out of a file this one
/// wrote, so they are spelled once here rather than at each end.
constexpr auto kPointerDriver = "driver";
constexpr auto kPointerRoot = "root";
constexpr auto kPointerBase = "base";
constexpr auto kPointerFscryptKey = "fscrypt_key";

/// The standard cache-directory tag, honoured by tar --exclude-caches, borg,
/// restic and rsnapshot. One line, and it keeps a session tree out of a whole
/// category of backups without any of them knowing what GpgFrontend is.
constexpr auto kCacheDirSignature =
    "Signature: 8a477f597d28d172789f06886806bc55\n"
    "# This directory holds a temporary GpgFrontend profile session.\n"
    "# It is deliberately not backed up: it contains key material that\n"
    "# belongs to a profile package, not to this machine.\n";

auto OwnerOnly() -> QFile::Permissions {
  return QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner;
}

/// A session directory within a chosen base. Named by the digest so that a
/// sweep can recognise one, and so two open packages cannot collide.
auto SessionDirIn(const QString &base, const QString &digest) -> QString {
  return QString("%1/gf-%2").arg(base, digest);
}

#if defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)

/// Whether a path is on a filesystem that lives in memory. Reported rather than
/// assumed: $XDG_RUNTIME_DIR is a tmpfs on every systemd distribution and on
/// none of them is that a guarantee.
auto IsRamBackedPath(const QString &path, QString &reason) -> bool {
  struct statfs info{};
  if (statfs(path.toLocal8Bit().constData(), &info) != 0) {
    reason = "this folder could not be examined";
    return false;
  }

#ifdef Q_OS_FREEBSD
  // No magic number here; the name is what there is.
  if (QLatin1String(info.f_fstypename) != QLatin1String("tmpfs")) {
    reason = QString("not a memory-backed filesystem (%1)")
                 .arg(QLatin1String(info.f_fstypename));
    return false;
  }
#else
  if (!IsRamBackedMagic(static_cast<quint64>(info.f_type))) {
    reason = "not a memory-backed filesystem";
    return false;
  }
#endif

  return true;
}

/// Make a base directory private to this user, or refuse one that is not.
auto ClaimBaseDirectory(const QString &path, QString &reason) -> bool {
  if (mkdir(path.toLocal8Bit().constData(), 0700) == 0) return true;

  struct stat info{};
  if (lstat(path.toLocal8Bit().constData(), &info) != 0) {
    // Not there and not creatable. Usually the parent is root-owned and this
    // login has no per-user runtime directory at all, which is ordinary inside
    // a container and worth saying rather than reporting as a mystery.
    reason = "this folder is not there and could not be created";
    return false;
  }

  if (!IsAcceptableOwnership(info.st_uid, info.st_mode,
                             S_ISLNK(info.st_mode) != 0, geteuid())) {
    // /dev/shm is world-writable, so something else may already be sitting at
    // the name this build would pick. Adopting it would hand another local
    // account a live view of somebody's keys.
    reason = "a folder of this name is already there and is not private to you";
    return false;
  }

  return S_ISDIR(info.st_mode) != 0;
}

#endif

#ifdef Q_OS_LINUX

/// The nearest ancestor of a path that exists. The interesting candidates have
/// not been created yet, and the question being asked -- what filesystem is
/// this -- is answerable about the place they would land.
auto NearestExistingAncestor(const QString &path) -> QString {
  auto current = QDir::cleanPath(path);

  while (!current.isEmpty()) {
    if (QFileInfo::exists(current)) return current;

    const auto cut = current.lastIndexOf('/');
    if (cut <= 0) return QStringLiteral("/");
    current.truncate(cut);
  }

  return {};
}

/// Whether the filesystem a path would land on is one that implements fscrypt.
///
/// A pre-filter and not the answer: the feature is per-superblock, so an ext4
/// here still has to be asked properly. What it settles cheaply is the common
/// case -- a temporary directory on a tmpfs, which cannot carry a policy at all
/// -- without opening or creating anything.
auto IsFscryptCapableOrigin(const QString &path, QString &reason) -> bool {
  const auto origin = NearestExistingAncestor(path);
  if (origin.isEmpty()) {
    reason = "this folder could not be examined";
    return false;
  }

  struct statfs info{};
  if (statfs(origin.toLocal8Bit().constData(), &info) != 0) {
    reason = "this folder could not be examined";
    return false;
  }

  if (!IsFscryptCapableMagic(static_cast<quint64>(info.f_type))) {
    reason = "this filesystem cannot encrypt folders";
    return false;
  }

  return true;
}

#endif

#ifdef Q_OS_WINDOWS

/// Ask NTFS to encrypt everything created inside a directory from here on.
///
/// Applied to the directory while it is still empty, which is exactly the
/// provision-then-extract order: a directory-level encrypt marks it so newly
/// created files inherit encryption.
///
/// Verified by reading the attribute back rather than trusting the return
/// value, and then by creating a file and asking about that. A driver that
/// claims an encryption it did not get is worse than one that admits it has
/// none.
auto EncryptDirectory(const QString &path, QString &reason) -> bool {
  const auto native = QDir::toNativeSeparators(path).toStdWString();

  if (EncryptFileW(native.c_str()) == 0) {
    reason = "this volume or edition does not support file encryption";
    return false;
  }

  const auto attributes = GetFileAttributesW(native.c_str());
  if (attributes == INVALID_FILE_ATTRIBUTES ||
      (attributes & FILE_ATTRIBUTE_ENCRYPTED) == 0) {
    reason = "file encryption was accepted but did not take effect";
    return false;
  }

  // The directory's own attribute says only that files created here should
  // inherit encryption. It is not a statement about any file, and the claim
  // this makes to the user -- that what lands here is unreadable to anyone
  // else -- is entirely about the files. So one is created and asked.
  //
  // The case this catches is not hypothetical: inheritance is what fails when
  // the account has no usable EFS certificate, and the directory keeps its
  // attribute either way. Without this, that machine would be told its keys
  // were in an encrypted folder while every file in it was plaintext.
  const auto probe = path + "/.gf-encryption-probe";

  QFile file(probe);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    reason = "this folder could not be written to";
    return false;
  }
  file.write("probe");
  file.close();

  const auto probe_native = QDir::toNativeSeparators(probe).toStdWString();
  const auto probe_attributes = GetFileAttributesW(probe_native.c_str());

  // Removed whatever the answer: it has served its purpose, and leaving a file
  // behind in a directory whose emptiness the caller relies on is its own bug.
  QFile::remove(probe);

  if (probe_attributes == INVALID_FILE_ATTRIBUTES ||
      (probe_attributes & FILE_ATTRIBUTE_ENCRYPTED) == 0) {
    reason = "files created in this folder are not encrypted";
    return false;
  }

  return true;
}

void MarkTemporaryAndUnindexed(const QString &path) {
  const auto native = QDir::toNativeSeparators(path).toStdWString();
  auto attributes = GetFileAttributesW(native.c_str());
  if (attributes == INVALID_FILE_ATTRIBUTES) return;

  // TEMPORARY is a hint to the cache manager to avoid flushing, not a promise.
  attributes |= FILE_ATTRIBUTE_TEMPORARY | FILE_ATTRIBUTE_NOT_CONTENT_INDEXED;
  SetFileAttributesW(native.c_str(), attributes);
}

#endif  // Q_OS_WINDOWS

auto ProbeFreeBytes(const QString &path) -> qint64 {
  QStorageInfo storage(path);
  if (!storage.isValid() || !storage.isReady()) return -1;
  return storage.bytesAvailable();
}

auto DescribeShortfall(qint64 free_bytes, qint64 budget_bytes) -> QString {
  const auto mb = [](qint64 bytes) { return bytes / (1024 * 1024); };
  return QString("%1 MB free, %2 MB needed")
      .arg(mb(free_bytes))
      .arg(mb(budget_bytes));
}

/**
 * @brief Close this process's log file when it is inside the tree being freed.
 *
 * A session logs into its own storage, so the handle points at the directory
 * about to be destroyed. POSIX unlinks an open file without complaint; Windows
 * refuses, and refuses for the directory holding it too -- so every packaged
 * session left its whole storage tree behind in the user's temporary folder,
 * log file included, and the sweep on the next start could not remove it
 * either while the same handle was held.
 *
 * Scoped to the tree: a session that logs somewhere else keeps logging there,
 * and the entries this teardown still emits reach the ring buffer and stderr
 * regardless.
 *
 * @param root storage root about to be removed
 */
void ReleaseLogFileUnder(const QString &root) {
  const auto log_path = GFLogManager::Instance().FileLoggerPath();
  if (log_path.isEmpty()) return;

  const auto inside = QFileInfo(log_path).absoluteFilePath();
  const auto tree = QDir(root).absolutePath();
  if (inside != tree && !inside.startsWith(tree + "/")) return;

  GFLogManager::Instance().StopFileLogger();
}

}  // namespace

// ------------------------------------------------------------------ the driver

ProtectedFsProfileAccessor::ProtectedFsProfileAccessor(QString root,
                                                       QString settings_file,
                                                       ProfileStoragePlan plan)
    : FsProfileAccessor(std::move(root), std::move(settings_file)),
      plan_(std::move(plan)) {}

ProtectedFsProfileAccessor::~ProtectedFsProfileAccessor() {
  // The tree holds somebody else's key material. If it is still here the owning
  // profile never got to unmount, which is exactly when leaving it behind would
  // be worst.
  Release(ProfileStorageRelease::kFAST);
}

auto ProtectedFsProfileAccessor::Provision(const QString &digest,
                                           const ProfileStoragePlan &plan)
    -> QSharedPointer<ProtectedFsProfileAccessor> {
  if (plan.refuse || plan.path.isEmpty()) return {};

  auto settled = plan;

  // The plain-folder candidate is the anchor itself, unchanged from every
  // earlier build; every other candidate gets a directory of its own inside the
  // base that was chosen.
  QString root = plan.driver == QLatin1String(kDriverDisk)
                     ? plan.path
                     : SessionDirIn(plan.path, digest);

  if (!QDir().mkpath(root)) {
    LOG_E() << "cannot create session storage:" << root;
    return {};
  }

#ifdef Q_OS_WINDOWS
  if (plan.driver == QLatin1String(kDriverEfs)) {
    QString reason;
    if (!EncryptDirectory(root, reason)) {
      // The protection did not happen, so the claim must not be made. Reported
      // as an ordinary temporary folder rather than refused outright: it is
      // still better than the profiles folder, and saying so honestly is the
      // whole contract.
      LOG_W() << "file encryption unavailable for the session folder:"
              << reason;
      settled.driver = kDriverTemp;
      settled.is_encrypted_at_rest = false;
    }
  }
#endif

  QByteArray fscrypt_key_id;

#ifdef Q_OS_LINUX
  if (plan.driver == QLatin1String(kDriverFscrypt)) {
    // A policy can only be set on an empty directory, and mkpath() above
    // succeeds on one that was already there. A tree the sweep did not reach --
    // same package, same digest, a process killed outright -- would otherwise
    // make this fail for as long as it sits there, and fail as an ordinary
    // downgrade that says nothing about why.
    //
    // Emptied rather than emptied unconditionally: the usual case is a
    // directory this call just created, and destroying and recreating that
    // would be one more way for the whole provision to fail for nothing.
    QDir root_dir(root);
    if (!root_dir.isEmpty(QDir::AllEntries | QDir::NoDotAndDotDot |
                          QDir::Hidden | QDir::System)) {
      root_dir.removeRecursively();

      // Refused rather than carried on with: every later step, including the
      // fallback to a plain folder, needs this directory to be there.
      if (!QDir().mkpath(root)) {
        LOG_E() << "cannot reclaim session storage:" << root;
        return {};
      }
    }

    QString reason;
    if (!FscryptProvisionDirectory(root, fscrypt_key_id, reason)) {
      // Reported as an ordinary temporary folder rather than refused: it is
      // still better than the profiles folder, and saying so honestly is the
      // whole contract. The same answer the EFS branch gives, for the same
      // reason.
      LOG_W() << "filesystem encryption unavailable for the session folder:"
              << reason;
      settled.driver = kDriverTemp;
      settled.is_encrypted_at_rest = false;
    }
  }
#endif

  HardenStorageDirectory(root);

  auto accessor = QSharedPointer<ProtectedFsProfileAccessor>::create(
      root, root + "/config/config.ini", settled);

  // Set here rather than passed through the plan: the plan is what a pure
  // function decided, and this is what actually happened.
  accessor->fscrypt_key_id_ = fscrypt_key_id;
  accessor->fscrypt_base_ = plan.path;

  return accessor;
}

auto ProtectedFsProfileAccessor::Driver() const -> QString {
  return plan_.driver;
}

auto ProtectedFsProfileAccessor::Label() const -> QString {
  if (plan_.is_volatile) {
    // Deliberately not "never touches your disk". A tmpfs page can be swapped,
    // we cannot lock the tree because GnuPG writes into it rather than us, and
    // `tmpfs noswap` is a mount option nobody here controls.
    return QCoreApplication::translate(
        "ProfileAccessor",
        "memory only; not written to your disk in the normal course of things");
  }

  if (plan_.is_encrypted_at_rest) {
    return QCoreApplication::translate(
        "ProfileAccessor", "an encrypted folder this session alone can read");
  }

  return QCoreApplication::translate("ProfileAccessor",
                                     "an ordinary temporary folder on this "
                                     "disk");
}

auto ProtectedFsProfileAccessor::IsVolatile() const -> bool {
  return plan_.is_volatile;
}

auto ProtectedFsProfileAccessor::IsEncryptedAtRest() const -> bool {
  return plan_.is_encrypted_at_rest;
}

auto ProtectedFsProfileAccessor::AnchorState() const -> QJsonObject {
  QJsonObject state;
  state[QLatin1String(kPointerDriver)] = plan_.driver;
  state[QLatin1String(kPointerRoot)] = PathOf(ProfileArea::kRoot);

  if (!fscrypt_key_id_.isEmpty()) {
    // The base rather than the root, because a sweep needs an open directory on
    // the filesystem holding the key and the root is the thing it just deleted.
    state[QLatin1String(kPointerBase)] = fscrypt_base_;
    state[QLatin1String(kPointerFscryptKey)] =
        FscryptIdentifierToHex(fscrypt_key_id_);
  }

  return state;
}

void ProtectedFsProfileAccessor::Release(ProfileStorageRelease mode) {
  if (released_ || mode == ProfileStorageRelease::kKEEP) return;

  RemoveTree(mode);

  if (fscrypt_key_id_.isEmpty()) return;

  // After the tree, never before. With the key gone a file cannot be opened,
  // so evicting first would leave a tree that could no longer be scrubbed or,
  // on the failure path above, retried. Unlink and rmdir keep working without
  // it, which is what makes this order safe.
  QString reason;
  if (FscryptRemoveKey(fscrypt_base_, fscrypt_key_id_, reason)) {
    fscrypt_key_id_.clear();
    return;
  }

  // The one outcome where the tree is not yet unreadable: the master secret is
  // wiped but files somebody still holds open keep their derived keys. Worth
  // saying, and not worth waiting on -- kFAST runs against the shutdown
  // watchdog's deadline, and the key is recorded next to the anchor precisely
  // so a later sweep can finish this.
  LOG_W() << "session key not fully evicted:" << reason;
}

void ProtectedFsProfileAccessor::RemoveTree(ProfileStorageRelease mode) {
  const auto root = PathOf(ProfileArea::kRoot);
  if (root.isEmpty()) {
    released_ = true;
    return;
  }

  // Skipped where it buys nothing: unlinking a tmpfs file already frees the
  // page, ciphertext whose key is gone is already unreadable, and kFAST runs
  // from the shutdown watchdog against a deadline it must not miss.
  const bool worth_scrubbing = mode == ProfileStorageRelease::kSCRUB &&
                               !plan_.is_volatile &&
                               !plan_.is_encrypted_at_rest;

  ReleaseLogFileUnder(root);

  if (worth_scrubbing) ScrubDirectory(root);

  if (QDir(root).removeRecursively()) {
    released_ = true;
    return;
  }

  // Something in the tree would not go. On Windows that means a handle is
  // still open on a file inside it, and the directory holding an open file
  // cannot be removed either.
  //
  // Reporting that and marking the storage released would be the dangerous
  // answer: this call is the only promise that an unpacked profile does not
  // outlive the process that unpacked it, and every later caller -- the
  // destructor included -- would take the flag at its word and never come
  // back. So the contents are destroyed whatever the mode said, which leaves
  // empty names rather than key material behind, and the flag stays down so
  // the destructor tries the removal again once the rest of the shutdown has
  // let go of whatever was holding it.
  ScrubDirectory(root);
  released_ = QDir(root).removeRecursively();
}

// ------------------------------------------------------------------- hardening

void HardenStorageDirectory(const QString &path) {
  if (path.isEmpty()) return;

  QFile::setPermissions(path, OwnerOnly());

  QFile tag(path + "/CACHEDIR.TAG");
  if (tag.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    tag.write(kCacheDirSignature);
    tag.close();
  }

  QFile nobackup(path + "/.nobackup");
  if (nobackup.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    nobackup.close();
  }

#ifdef Q_OS_MACOS
  // Spotlight reads this one; Time Machine reads the extended attribute. Not
  // CSBackupSetItemExcluded(), which would pull in CoreServices for no gain.
  QFile never_index(path + "/.metadata_never_index");
  if (never_index.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    never_index.close();
  }

  const auto native = path.toLocal8Bit();
  const char kExcluded = 1;
  setxattr(native.constData(),
           "com.apple.metadata:com_apple_backup_excludeItem", &kExcluded,
           sizeof(kExcluded), 0, 0);
#endif

#ifdef Q_OS_WINDOWS
  // POSIX owner-only permissions mean nothing on NTFS, so the attributes carry
  // what they can and the ACL is left to the directory's inherited default.
  MarkTemporaryAndUnindexed(path);
#endif
}

void ScrubDirectory(const QString &path) {
  // A bug in the caller must not be able to turn this loose on the wrong tree.
  if (path.isEmpty() || !QDir::isAbsolutePath(path)) {
    LOG_W() << "refusing to scrub a path that is not absolute:" << path;
    return;
  }

  QDir dir(path);
  if (!dir.exists()) return;

  for (const auto &entry :
       dir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot |
                         QDir::Hidden | QDir::System)) {
    // Never followed. A symlink out of the tree would have this overwriting
    // something that was never ours.
    if (entry.isSymLink()) continue;

    if (entry.isDir()) {
      ScrubDirectory(entry.absoluteFilePath());
      continue;
    }

    QFile file(entry.absoluteFilePath());
    const auto size = file.size();
    if (size <= 0) continue;

    // A read-only file is not a reason to abandon the rest of the tree.
    if (!file.open(QIODevice::ReadWrite)) continue;

    // One pass. Gutmann's thirty-five were about the encoding of 1990s MFM
    // drives and buy nothing on anything sold this century.
    const QByteArray zeros(qMin<qint64>(size, 64 * 1024), '\0');
    qint64 written = 0;
    while (written < size) {
      const auto chunk = qMin<qint64>(zeros.size(), size - written);
      const auto put = file.write(zeros.constData(), chunk);
      if (put <= 0) break;
      written += put;
    }

    file.flush();
    file.resize(0);
    file.close();

    // Renamed before it is removed, so the name itself does not survive in the
    // directory entry for an undelete to read back. Best effort like the rest
    // of this: a failed rename is not a reason to leave the file.
    const auto scrubbed = entry.absolutePath() + "/" +
                          QString::number(entry.fileName().size())
                              .rightJustified(entry.fileName().size(), 'x');

    if (QFile::rename(entry.absoluteFilePath(), scrubbed)) {
      QFile::remove(scrubbed);
    } else {
      QFile::remove(entry.absoluteFilePath());
    }
  }

  // The directories themselves, now that nothing is left in them. Left to the
  // caller's removeRecursively() would work too, but a scrub that leaves the
  // shape of the tree behind has told only half the truth.
  QDir(path).removeRecursively();
}

auto ReleaseStrandedSessionStorage(const QJsonObject &pointer,
                                   const QString &anchor) -> bool {
  if (pointer.isEmpty()) return true;

  auto done = true;

  const auto root = pointer.value(QLatin1String(kPointerRoot)).toString();

  // Absolute or nothing. This deletes what the pointer names, and the pointer
  // is a file on disk that some other process wrote.
  if (!root.isEmpty() && QDir::isAbsolutePath(root) && root != anchor &&
      QFileInfo::exists(root)) {
    if (QDir(root).removeRecursively()) {
      LOG_I() << "removed session storage left behind by a process that is"
              << "gone:" << root;
    } else {
      LOG_W() << "could not remove stranded session storage:" << root;
      done = false;
    }
  }

  const auto key = FscryptIdentifierFromHex(
      pointer.value(QLatin1String(kPointerFscryptKey)).toString());
  if (key.isEmpty()) return done;

  // Outside the tree's guard on purpose. A key lives in the kernel, not in the
  // directory, so it can perfectly well still be there when the tree is
  // already gone — and that is precisely the case worth cleaning, because
  // nothing else will ever come back for it.
  auto base = pointer.value(QLatin1String(kPointerBase)).toString();
  if (base.isEmpty() && !root.isEmpty()) {
    base = QFileInfo(root).absolutePath();
  }

  if (base.isEmpty() || !QDir::isAbsolutePath(base) ||
      !QFileInfo::exists(base)) {
    LOG_W() << "cannot reach the filesystem holding a stranded session key";
    return false;
  }

  QString reason;
  if (!FscryptRemoveKey(base, key, reason)) {
    LOG_W() << "could not remove a stranded session key:" << reason;
    return false;
  }

  LOG_I() << "removed a session key left behind by a process that is gone";
  return done;
}

// ------------------------------------------------------------------- the probe

auto ProbeStorageCandidates(qint64 budget_bytes, const QString &anchor)
    -> QList<StorageCandidate> {
  QList<StorageCandidate> candidates;

#if defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)
  const auto bases = VolatileStoreSearchPaths(
      qEnvironmentVariable("XDG_RUNTIME_DIR"),
      qEnvironmentVariable(kProfileStorageDirEnv), geteuid());

  for (const auto &base : bases) {
    StorageCandidate candidate;
    candidate.path = base;
    candidate.driver = kDriverTmpfs;
    candidate.is_volatile = true;

    // Claimed before it is examined, because the interesting candidates do not
    // exist yet: /dev/shm/gpgfrontend-<uid> is ours to make.
    if (!ClaimBaseDirectory(base, candidate.reason)) {
      candidates << candidate;
      continue;
    }

    if (!IsRamBackedPath(base, candidate.reason)) {
      candidates << candidate;
      continue;
    }

    candidate.free_bytes = ProbeFreeBytes(base);
    if (candidate.free_bytes >= 0 && candidate.free_bytes < budget_bytes) {
      candidate.reason = DescribeShortfall(candidate.free_bytes, budget_bytes);
      candidates << candidate;
      continue;
    }

    candidate.usable = true;
    candidates << candidate;
  }

  // The encrypted rung, between memory and a plain temporary folder. Not
  // another place to put a session: it is the temporary folder with a policy
  // on it, so the rung below is the same folder without one.
  //
  // At most one candidate, because all of these answer the same question and
  // three lines of "not this one either" in a refusal dialog is noise rather
  // than something to act on.
  if (FscryptAvailable()) {
    StorageCandidate candidate;
    candidate.driver = kDriverFscrypt;
    candidate.is_encrypted_at_rest = true;

    const auto bases =
        EncryptableStoreSearchPaths(qEnvironmentVariable(kProfileStorageDirEnv),
                                    QDir::tempPath(), geteuid());

    for (const auto &base : bases) {
      candidate.path = base;
      candidate.free_bytes = 0;

      // The cheap answer first, on whatever part of the path already exists:
      // a temporary folder on a tmpfs is the common case and is settled here
      // without creating anything.
      if (!IsFscryptCapableOrigin(base, candidate.reason)) continue;

      if (!ClaimBaseDirectory(base, candidate.reason)) continue;

      // Asked again on the directory that was created, because the first
      // answer was about an ancestor and the two can be different mounts.
      if (!FscryptDirectoryIsUsable(base, candidate.reason)) {
        // Left behind it would be an empty directory this build will never use
        // again, in the user's temporary folder, once per launch.
        QDir().rmdir(base);
        continue;
      }

      candidate.free_bytes = ProbeFreeBytes(base);
      if (candidate.free_bytes >= 0 && candidate.free_bytes < budget_bytes) {
        candidate.reason =
            DescribeShortfall(candidate.free_bytes, budget_bytes);
        continue;
      }

      // Whether a policy actually applies is only knowable by setting one, and
      // that mints a key -- so it is settled at provisioning time against the
      // real session directory rather than guessed at here. The same division
      // the EFS candidate makes.
      candidate.usable = true;
      break;
    }

    if (candidate.path.isEmpty()) {
      candidate.path = bases.isEmpty() ? QDir::tempPath() : bases.constLast();
      candidate.reason = "there is nowhere on this machine that can encrypt it";
    }

    candidates << candidate;
  }
#endif

#ifdef Q_OS_WINDOWS
  {
    StorageCandidate candidate;
    candidate.driver = kDriverEfs;
    candidate.is_encrypted_at_rest = true;
    candidate.path = QDir::tempPath();
    candidate.free_bytes = ProbeFreeBytes(candidate.path);

    if (candidate.free_bytes >= 0 && candidate.free_bytes < budget_bytes) {
      candidate.reason = DescribeShortfall(candidate.free_bytes, budget_bytes);
    } else {
      // Whether EFS actually applies is only knowable by asking the volume, and
      // the answer is per-directory, so it is settled at provisioning time
      // against a real directory rather than guessed at here.
      candidate.usable = true;
    }
    candidates << candidate;
  }
#endif

  // The hardened temporary directory: no protection at all, and it says so.
  // Present on every platform, because it is better than the profiles folder
  // even when it is not better by much — the system clears it, and it is not
  // sitting next to the user's real profiles.
  {
    StorageCandidate candidate;
    candidate.path = QDir::tempPath();
    candidate.driver = kDriverTemp;
    candidate.free_bytes = ProbeFreeBytes(candidate.path);

    if (candidate.path.isEmpty()) {
      candidate.reason = "this system has no temporary folder";
    } else if (candidate.free_bytes >= 0 &&
               candidate.free_bytes < budget_bytes) {
      candidate.reason = DescribeShortfall(candidate.free_bytes, budget_bytes);
    } else {
      candidate.usable = true;
    }
    candidates << candidate;
  }

  // Last, always: the profiles folder, exactly where every earlier build put
  // it. This is what a `disk` policy means, and it is the fallback of last
  // resort for `auto`.
  {
    StorageCandidate candidate;
    candidate.path = anchor;
    candidate.driver = kDriverDisk;
    candidate.free_bytes = ProbeFreeBytes(anchor);
    candidate.usable = !anchor.isEmpty();
    if (!candidate.usable) candidate.reason = "there is nowhere to put it";
    candidates << candidate;
  }

  return candidates;
}

auto MakeProfileAccessorFor(const ProfileAccessorSpec &spec)
    -> ProfileAccessorResult {
  ProfileAccessorResult result;

  const auto candidates =
      ProbeStorageCandidates(spec.budget_bytes, spec.anchor);
  const auto plan = PlanProfileStorage(spec.policy, candidates);

  result.rejections = plan.rejections;

  if (plan.refuse) {
    result.refused_protected_only =
        spec.policy == ProfileStoragePolicy::kPROTECTED_ONLY;
    for (const auto &rejection : plan.rejections) {
      LOG_I() << "session storage passed over:" << rejection;
    }
    return result;
  }

  auto accessor = ProtectedFsProfileAccessor::Provision(spec.digest, plan);
  if (accessor.isNull()) return result;

  LOG_I() << "session storage:" << accessor->Driver() << "at"
          << accessor->PathOf(ProfileArea::kRoot)
          << "volatile:" << accessor->IsVolatile()
          << "encrypted:" << accessor->IsEncryptedAtRest();

  // Taken from the concrete driver here, where the type is still known, rather
  // than recovered by a cast at the call site.
  result.anchor_state = accessor->AnchorState();

  // A package carries its application key unprotected -- the package's own
  // passphrase was what protected it -- so unpacking one normally writes
  // another machine's key material onto this disk in plaintext, and the
  // rewrite that follows unlinks the plaintext without overwriting it.
  //
  // Holding the secure area in memory instead means those bytes never reach a
  // filesystem at all. Which areas may be held this way is the area table's
  // answer, not this function's.
  QSet<ProfileArea> resident;
  for (const auto &row : ProfileAreaTable()) {
    if (!row.area.has_value()) continue;
    if (row.residency != AreaResidency::kVirtualisable) continue;
    resident.insert(*row.area);
  }

  if (resident.isEmpty()) {
    result.accessor = accessor;
    return result;
  }

  result.accessor =
      QSharedPointer<MemoryAreaProfileAccessor>::create(accessor, resident);
  LOG_I() << "held in memory rather than on the storage:" << resident.size()
          << "area(s)";
  return result;
}

}  // namespace GpgFrontend
