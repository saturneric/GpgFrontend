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
constexpr auto kDriverEfs = "fs-efs";
constexpr auto kDriverTemp = "fs-temp";
constexpr auto kDriverDisk = "fs";

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

#ifdef Q_OS_WINDOWS

/// Ask NTFS to encrypt everything created inside a directory from here on.
///
/// Applied to the directory while it is still empty, which is exactly the
/// provision-then-extract order: a directory-level encrypt marks it so newly
/// created files inherit encryption.
///
/// Verified by reading the attribute back rather than trusting the return
/// value. A driver that claims an encryption it did not get is worse than one
/// that admits it has none.
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

  HardenStorageDirectory(root);

  auto accessor = QSharedPointer<ProtectedFsProfileAccessor>::create(
      root, root + "/config/config.ini", settled);

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
  state["driver"] = plan_.driver;
  state["root"] = PathOf(ProfileArea::kRoot);
  return state;
}

void ProtectedFsProfileAccessor::Release(ProfileStorageRelease mode) {
  if (released_ || mode == ProfileStorageRelease::kKEEP) return;
  released_ = true;

  const auto root = PathOf(ProfileArea::kRoot);
  if (root.isEmpty()) return;

  // Skipped where it buys nothing: unlinking a tmpfs file already frees the
  // page, ciphertext whose key is gone is already unreadable, and kFAST runs
  // from the shutdown watchdog against a deadline it must not miss.
  const bool worth_scrubbing = mode == ProfileStorageRelease::kSCRUB &&
                               !plan_.is_volatile &&
                               !plan_.is_encrypted_at_rest;

  if (worth_scrubbing) ScrubDirectory(root);

  QDir(root).removeRecursively();
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

  result.accessor = accessor;
  return result;
}

}  // namespace GpgFrontend
