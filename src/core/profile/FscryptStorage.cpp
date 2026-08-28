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

#include "core/profile/FscryptStorage.h"

#include <QDir>
#include <QFile>

#include "core/GFCoreLog.h"

#if defined(Q_OS_LINUX) && __has_include(<linux/fscrypt.h>)
#define GF_HAS_FSCRYPT 1
#endif

#ifdef GF_HAS_FSCRYPT
#include <fcntl.h>
#include <linux/fscrypt.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>

#include "core/function/SecureRandomGenerator.h"
#include "core/model/GFBuffer.h"
#endif

namespace GpgFrontend {

namespace {

#ifdef GF_HAS_FSCRYPT

/// A directory file descriptor for the length of a call. Every fscrypt ioctl
/// takes one, and every early return in here would otherwise leak it.
class FdGuard {
 public:
  explicit FdGuard(const QString &path)
      : fd_(::open(path.toLocal8Bit().constData(),
                   O_RDONLY | O_DIRECTORY | O_CLOEXEC)) {}

  ~FdGuard() {
    if (fd_ >= 0) ::close(fd_);
  }

  FdGuard(const FdGuard &) = delete;
  auto operator=(const FdGuard &) -> FdGuard & = delete;
  FdGuard(FdGuard &&) = delete;
  auto operator=(FdGuard &&) -> FdGuard & = delete;

  [[nodiscard]] auto Ok() const -> bool { return fd_ >= 0; }
  [[nodiscard]] auto Get() const -> int { return fd_; }

 private:
  int fd_;
};

/**
 * @brief Ask the filesystem whether it can encrypt at all.
 *
 * A key-status query for an identifier of all zeroes: no such key can exist, so
 * the answer is always "absent" and nothing is created or changed. What is
 * being read is not the answer but whether the question was accepted — ext4 and
 * f2fs refuse it when the superblock does not carry the encrypt feature, which
 * is how it is off on a stock Debian or Ubuntu install.
 *
 * @param fd a directory on the filesystem
 * @param reason set to why not
 * @return true when the filesystem accepts fscrypt ioctls
 */
auto SupportsFscrypt(int fd, QString &reason) -> bool {
  struct fscrypt_get_key_status_arg arg{};
  arg.key_spec.type = FSCRYPT_KEY_SPEC_TYPE_IDENTIFIER;

  if (ioctl(fd, FS_IOC_GET_ENCRYPTION_KEY_STATUS, &arg) == 0) return true;

  switch (errno) {
    case ENOTTY:
      reason = "this kernel has no filesystem encryption";
      break;
    case EOPNOTSUPP:
      reason = "this filesystem was created without encryption support";
      break;
    default:
      reason = QString("this filesystem refused an encryption query (%1)")
                   .arg(QLatin1String(strerror(errno)));
      break;
  }
  return false;
}

/**
 * @brief Whether a directory already sits under somebody else's policy.
 *
 * A policy cannot be set inside one, and a home directory managed by
 * systemd-homed is exactly such a place. Left unasked, the machines most likely
 * to have fscrypt at all would be the ones that quietly fell back every time.
 *
 * @param fd the directory
 * @return true when a policy is already in force here
 */
auto AlreadyEncrypted(int fd) -> bool {
  struct fscrypt_get_policy_ex_arg arg{};
  arg.policy_size = sizeof(arg.policy);
  return ioctl(fd, FS_IOC_GET_ENCRYPTION_POLICY_EX, &arg) == 0;
}

/**
 * @brief Generate a master key and hand it to the kernel.
 *
 * The key exists in this process only for the length of this function. It is
 * generated into a secure buffer, copied once into the ioctl argument, and both
 * are wiped before returning — the kernel keeps the only copy from here on, and
 * this process keeps the identifier, which cannot decrypt anything.
 *
 * @param fd a directory on the target filesystem
 * @param identifier set to the identifier the kernel derived
 * @param reason set to why not
 * @return true when the key is in the kernel
 */
auto AddKey(int fd, QByteArray &identifier, QString &reason) -> bool {
  auto key = SecureRandomGenerator::Generate(kFscryptMasterKeySize);
  if (!key || key->Size() != static_cast<size_t>(kFscryptMasterKeySize)) {
    reason = "a key for this session could not be generated";
    return false;
  }

  // The argument ends in a flexible array holding the key, so it is allocated
  // rather than declared, and in the secure allocator because for the few lines
  // between here and the ioctl it *is* the key.
  GFBuffer arg_buffer(static_cast<size_t>(sizeof(struct fscrypt_add_key_arg) +
                                          kFscryptMasterKeySize));
  memset(arg_buffer.Data(), 0, arg_buffer.Size());

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast) -- the uapi
  // struct has a flexible array member and there is no other way to fill it.
  auto *arg = reinterpret_cast<struct fscrypt_add_key_arg *>(arg_buffer.Data());
  arg->key_spec.type = FSCRYPT_KEY_SPEC_TYPE_IDENTIFIER;
  arg->raw_size = kFscryptMasterKeySize;
  arg->key_id = 0;
  memcpy(arg->raw, key->Data(), kFscryptMasterKeySize);

  key->Zeroize();

  const auto ok = ioctl(fd, FS_IOC_ADD_ENCRYPTION_KEY, arg) == 0;
  const auto saved_errno = errno;

  if (ok) {
    identifier =
        QByteArray(reinterpret_cast<const char *>(arg->key_spec.u.identifier),
                   kFscryptKeyIdentifierSize);
  }

  // Before any early return, and before the buffer's own destructor, because
  // the raw key is still sitting in it either way.
  arg_buffer.Zeroize();

  if (!ok) {
    reason = QString("this session's key was refused by the kernel (%1)")
                 .arg(QLatin1String(strerror(saved_errno)));
    return false;
  }

  return true;
}

/**
 * @brief Put a v2 policy naming our key on an empty directory.
 *
 * @param fd the directory, which must be empty
 * @param identifier the key to encrypt it under
 * @param reason set to why not
 * @return true when the policy is set
 */
auto SetPolicyV2(int fd, const QByteArray &identifier, QString &reason)
    -> bool {
  struct fscrypt_policy_v2 policy{};
  policy.version = FSCRYPT_POLICY_V2;
  policy.contents_encryption_mode = FSCRYPT_MODE_AES_256_XTS;
  policy.filenames_encryption_mode = FSCRYPT_MODE_AES_256_CTS;

  // Filenames are padded to a multiple of 16 so their lengths leak less. The
  // cost is that a name may be at most 240 bytes rather than 255, which is
  // ample for everything this program writes and for anything GnuPG writes.
  policy.flags = FSCRYPT_POLICY_FLAGS_PAD_16;

  memcpy(policy.master_key_identifier, identifier.constData(),
         kFscryptKeyIdentifierSize);

  if (ioctl(fd, FS_IOC_SET_ENCRYPTION_POLICY, &policy) == 0) return true;

  switch (errno) {
    case ENOTEMPTY:
      reason = "this folder is not empty, so it cannot be encrypted";
      break;
    case EEXIST:
      reason = "this folder is already encrypted by something else";
      break;
    default:
      reason = QString("this folder could not be encrypted (%1)")
                   .arg(QLatin1String(strerror(errno)));
      break;
  }
  return false;
}

/**
 * @brief Whether a path is under a policy naming exactly our key.
 *
 * Used on the directory and then on a file created inside it. The second is the
 * one that matters: the directory's policy says what files created here
 * *should* inherit, and the claim being made to the user is entirely about the
 * files.
 *
 * @param path the directory or file to ask about
 * @param identifier the key it should be under
 * @return true when it is encrypted under that key
 */
auto PolicyMatches(const QString &path, const QByteArray &identifier) -> bool {
  const auto fd = ::open(path.toLocal8Bit().constData(), O_RDONLY | O_CLOEXEC);
  if (fd < 0) return false;

  struct fscrypt_get_policy_ex_arg arg{};
  arg.policy_size = sizeof(arg.policy);
  const auto ok = ioctl(fd, FS_IOC_GET_ENCRYPTION_POLICY_EX, &arg) == 0;
  ::close(fd);

  if (!ok || arg.policy.version != FSCRYPT_POLICY_V2) return false;

  return memcmp(arg.policy.v2.master_key_identifier, identifier.constData(),
                kFscryptKeyIdentifierSize) == 0;
}

/**
 * @brief Create a file inside and ask whether it came out encrypted.
 *
 * Inheritance is what fails when a policy is set but unusable, and the
 * directory keeps its own attribute either way. Without this the user would be
 * told their keys were in an encrypted folder while every file in it was
 * plaintext.
 *
 * @param dir the provisioned directory
 * @param identifier the key it should be under
 * @return true when a file created here is encrypted under that key
 */
auto ProbeFileInherits(const QString &dir, const QByteArray &identifier)
    -> bool {
  const auto probe = dir + "/.gf-encryption-probe";

  QFile file(probe);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
  file.write("probe");
  file.close();

  const auto inherited = PolicyMatches(probe, identifier);

  // Removed whatever the answer: the caller relies on this directory being
  // empty, and leaving a file behind in it is its own bug.
  QFile::remove(probe);
  return inherited;
}

/**
 * @brief Bind a unix socket inside, because gpg-agent will need to.
 *
 * The session tree is GnuPG's home directory, and gpg-agent puts its sockets
 * there. Whether a socket can be bound under an encryption policy is a question
 * about this kernel and this filesystem, so it is answered here rather than
 * assumed and discovered later as a profile that will not open.
 *
 * @param dir the provisioned directory
 * @return true when a socket can be bound inside it
 */
auto ProbeSocketBinds(const QString &dir) -> bool {
  const auto path = dir + "/.gf-sock-probe";
  const auto native = path.toLocal8Bit();

  struct sockaddr_un address{};
  address.sun_family = AF_UNIX;

  // Not a filesystem limit but the socket API's own, and a session root deep
  // enough to overflow it would fail for gpg-agent too.
  if (static_cast<size_t>(native.size()) >= sizeof(address.sun_path)) {
    return false;
  }
  memcpy(address.sun_path, native.constData(), native.size());

  const auto fd = ::socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
  if (fd < 0) return false;

  const auto bound =
      ::bind(fd, reinterpret_cast<struct sockaddr *>(&address),  // NOLINT
             sizeof(address)) == 0;
  ::close(fd);

  QFile::remove(path);
  return bound;
}

#endif  // GF_HAS_FSCRYPT

}  // namespace

// ---------------------------------------------------------------- the surface

auto FscryptAvailable() -> bool {
#ifdef GF_HAS_FSCRYPT
  return true;
#else
  return false;
#endif
}

auto FscryptDirectoryIsUsable(const QString &path, QString &reason) -> bool {
#ifdef GF_HAS_FSCRYPT
  FdGuard dir(path);
  if (!dir.Ok()) {
    reason = "this folder could not be examined";
    return false;
  }

  if (!SupportsFscrypt(dir.Get(), reason)) return false;

  if (AlreadyEncrypted(dir.Get())) {
    reason = "this folder is already encrypted by something else";
    return false;
  }

  return true;
#else
  Q_UNUSED(path)
  reason = "this build has no filesystem encryption support";
  return false;
#endif
}

auto FscryptProvisionDirectory(const QString &dir, QByteArray &identifier,
                               QString &reason) -> bool {
#ifdef GF_HAS_FSCRYPT
  identifier.clear();

  FdGuard handle(dir);
  if (!handle.Ok()) {
    reason = "this folder could not be opened";
    return false;
  }

  QByteArray minted;
  if (!AddKey(handle.Get(), minted, reason)) return false;

  if (!SetPolicyV2(handle.Get(), minted, reason)) {
    // The key is ours and nothing is using it, so it does not outlive the
    // attempt. A failure here is a fallback to plain storage, not a reason to
    // leave a key sitting in the kernel until the next reboot.
    QString ignored;
    FscryptRemoveKey(dir, minted, ignored);
    return false;
  }

  // From here the policy is set, so every failure must also evict the key --
  // otherwise the directory stays encrypted under a key nobody will ever drop.
  const auto give_up = [&dir, &minted](const QString &why, QString &out) {
    out = why;
    QString ignored;
    FscryptRemoveKey(dir, minted, ignored);
    return false;
  };

  if (!PolicyMatches(dir, minted)) {
    return give_up("encryption was accepted but did not take effect", reason);
  }

  if (!ProbeFileInherits(dir, minted)) {
    return give_up("files created in this folder are not encrypted", reason);
  }

  if (!ProbeSocketBinds(dir)) {
    return give_up("this folder cannot hold the sockets GnuPG needs", reason);
  }

  identifier = minted;
  return true;
#else
  Q_UNUSED(dir)
  identifier.clear();
  reason = "this build has no filesystem encryption support";
  return false;
#endif
}

auto FscryptRemoveKey(const QString &any_path_on_fs,
                      const QByteArray &identifier, QString &reason) -> bool {
#ifdef GF_HAS_FSCRYPT
  if (identifier.size() != kFscryptKeyIdentifierSize) {
    reason = "there is no key to remove";
    return false;
  }

  FdGuard handle(any_path_on_fs);
  if (!handle.Ok()) {
    reason = "the folder holding the key could not be opened";
    return false;
  }

  struct fscrypt_remove_key_arg arg{};
  arg.key_spec.type = FSCRYPT_KEY_SPEC_TYPE_IDENTIFIER;
  memcpy(arg.key_spec.u.identifier, identifier.constData(),
         kFscryptKeyIdentifierSize);

  if (ioctl(handle.Get(), FS_IOC_REMOVE_ENCRYPTION_KEY, &arg) != 0) {
    // Already gone is the outcome this asks for, so it is not a failure. It is
    // also the ordinary answer when a sweep reaches a session whose own
    // shutdown got there first.
    if (errno == ENOKEY) return true;

    reason = QString("this session's key could not be removed (%1)")
                 .arg(QLatin1String(strerror(errno)));
    return false;
  }

  if ((arg.removal_status_flags & FSCRYPT_KEY_REMOVAL_STATUS_FLAG_FILES_BUSY) !=
      0) {
    // The master secret is wiped; what remains is per-inode keys held by files
    // somebody still has open. Reported rather than swallowed, because it is
    // the one outcome where the tree is not yet unreadable.
    reason = "the key was dropped but some files are still open";
    return false;
  }

  return true;
#else
  Q_UNUSED(any_path_on_fs)
  Q_UNUSED(identifier)
  reason = "this build has no filesystem encryption support";
  return false;
#endif
}

auto FscryptIdentifierToHex(const QByteArray &id) -> QString {
  if (id.size() != kFscryptKeyIdentifierSize) return {};
  return QString::fromLatin1(id.toHex());
}

auto FscryptIdentifierFromHex(const QString &hex) -> QByteArray {
  if (hex.size() != kFscryptKeyIdentifierSize * 2) return {};

  // Every character checked, rather than trusting the length of what comes
  // back. fromHex() *skips* what it cannot read and pads an odd count, so a
  // string of the right length with one bad character in it still decodes to a
  // full-length identifier -- one naming a key nobody minted. This parses a
  // file on disk and what is done with the result is an eviction, so it has to
  // refuse rather than approximate.
  for (const auto c : hex) {
    const auto ch = c.toLatin1();
    const auto is_hex_digit = (ch >= '0' && ch <= '9') ||
                              (ch >= 'a' && ch <= 'f') ||
                              (ch >= 'A' && ch <= 'F');
    if (!is_hex_digit) return {};
  }

  const auto raw = QByteArray::fromHex(hex.toLatin1());
  if (raw.size() != kFscryptKeyIdentifierSize) return {};

  return raw;
}

}  // namespace GpgFrontend
