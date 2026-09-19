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

#include "core/module/ModuleImageMapping.h"

#include <atomic>

#include "core/function/SecureMemoryAllocator.h"
#include "core/model/GFBuffer.h"
#include "core/utils/MemoryUtils.h"

#ifdef Q_OS_LINUX
#include <fcntl.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>
#endif

#ifdef Q_OS_UNIX
#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>
#endif

#ifdef Q_OS_WINDOWS
#include <windows.h>
#endif

namespace GpgFrontend::Module {

namespace {

/// Armed by a test, disarmed by the materialisation it affects.
std::atomic<ModuleImageFaultPoint> g_fault{ModuleImageFaultPoint::kNONE};

auto TakeFault() -> ModuleImageFaultPoint {
  return g_fault.exchange(ModuleImageFaultPoint::kNONE);
}

/// The parent of every per-process directory, so a sweep has one place to look.
auto PrivateRoot() -> QString {
  auto base = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
  if (base.isEmpty()) base = QDir::tempPath();
  return base + "/gpgfrontend-modules";
}

constexpr auto kLockFileName = ".in-use";

/// Hold an advisory lock for as long as this object lives.
///
/// The kernel releases it when the process dies, however it dies, which is what
/// makes "is anyone using this directory" answerable without recording a
/// process id. A recorded id is reused, and a sweep that trusted one would
/// eventually delete a directory belonging to somebody else.
class DirectoryLock {
 public:
  explicit DirectoryLock(const QString& directory) { Acquire(directory); }
  ~DirectoryLock() { Release(); }

  DirectoryLock(const DirectoryLock&) = delete;
  auto operator=(const DirectoryLock&) -> DirectoryLock& = delete;

  [[nodiscard]] auto Held() const -> bool {
#ifdef Q_OS_WINDOWS
    return handle_ != INVALID_HANDLE_VALUE;
#else
    return fd_ >= 0;
#endif
  }

 private:
  void Acquire(const QString& directory) {
    const auto path = directory + "/" + kLockFileName;
#ifdef Q_OS_WINDOWS
    // Share deletion and nothing else. Another process asking for write access
    // is refused, which is the signal a sweep reads; permitting deletion is
    // what lets the sweep remove a directory whose lock it is itself holding,
    // rather than having to let go first and race whoever starts next.
    handle_ = CreateFileW(reinterpret_cast<const wchar_t*>(
                              QDir::toNativeSeparators(path).utf16()),
                          GENERIC_WRITE, FILE_SHARE_DELETE, nullptr,
                          OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
#else
    fd_ = ::open(path.toLocal8Bit().constData(), O_RDWR | O_CREAT | O_CLOEXEC,
                 0600);
    if (fd_ < 0) return;
    if (::flock(fd_, LOCK_EX | LOCK_NB) != 0) {
      ::close(fd_);
      fd_ = -1;
    }
#endif
  }

  void Release() {
#ifdef Q_OS_WINDOWS
    if (handle_ != INVALID_HANDLE_VALUE) {
      CloseHandle(handle_);
      handle_ = INVALID_HANDLE_VALUE;
    }
#else
    if (fd_ >= 0) {
      ::close(fd_);  // releases the flock
      fd_ = -1;
    }
#endif
  }

#ifdef Q_OS_WINDOWS
  HANDLE handle_ = INVALID_HANDLE_VALUE;
#else
  int fd_ = -1;
#endif
};

auto Sha256Of(const char* data, qint64 size) -> QString {
  QCryptographicHash hash(QCryptographicHash::Sha256);
  hash.addData(QByteArrayView(data, static_cast<qsizetype>(size)));
  return QString::fromLatin1(hash.result().toHex());
}

}  // namespace

class ModuleImageMapping::Impl {
 public:
  ~Impl() {
    // Order matters: the image goes before the lock, so nothing ever sees an
    // unlocked directory that still has a module binary in it.
    RemoveImage();
    lock_.reset();
    dir_.reset();
  }

  QString load_path;
  bool anonymous = false;

  void AdoptDirectory(std::unique_ptr<QTemporaryDir> dir,
                      std::unique_ptr<DirectoryLock> lock) {
    dir_ = std::move(dir);
    lock_ = std::move(lock);
  }

#ifdef Q_OS_LINUX
  int memfd = -1;
#endif

  void RemoveImage() {
    if (image_removed_ || load_path.isEmpty() || anonymous) return;
    if (QFile::remove(load_path)) image_removed_ = true;
  }

 private:
  bool image_removed_ = false;
  std::unique_ptr<DirectoryLock> lock_;
  std::unique_ptr<QTemporaryDir> dir_;
};

ModuleImageMapping::ModuleImageMapping() : impl_(std::make_unique<Impl>()) {}

ModuleImageMapping::~ModuleImageMapping() {
#ifdef Q_OS_LINUX
  // The descriptor is deliberately never closed.
  //
  // On this path the descriptor number *is* the name the loader was given, and
  // glibc matches an already-loaded object by name before it ever looks at an
  // inode. Close it and the number is recycled, so the next image gets the
  // identical path string -- and dlopen hands back the previous object instead
  // of mapping the new one. It reports success and runs no initialisers, which
  // is the worst possible shape for this bug: the wrong module, silently.
  //
  // Holding the descriptor makes the number unavailable, which is precisely
  // the guarantee needed. The cost is one descriptor per module load attempt,
  // and the number of modules is a handful.
  (void)impl_;
#endif
}

auto ModuleImageMapping::LoadPath() const -> QString {
  return impl_->load_path;
}

auto ModuleImageMapping::IsAnonymous() const -> bool {
  return impl_->anonymous;
}

void ModuleImageMapping::NotifyLoaded() { impl_->RemoveImage(); }

namespace {

/// Write the whole buffer, honouring an armed fault. True only when every byte
/// of it landed.
auto WriteAll(QIODevice& out, const QByteArray& bytes,
              ModuleImageFaultPoint fault) -> bool {
  if (fault == ModuleImageFaultPoint::kWRITE_FAILS) return false;

  auto to_write = static_cast<qint64>(bytes.size());
  if (fault == ModuleImageFaultPoint::kSHORT_WRITE && to_write > 0) {
    to_write -= 1;
  }

  qint64 done = 0;
  while (done < to_write) {
    const auto n = out.write(bytes.constData() + done, to_write - done);
    if (n <= 0) return false;
    done += n;
  }
  return done == static_cast<qint64>(bytes.size());
}

auto Fail(QString* reason, const QString& text)
    -> std::unique_ptr<ModuleImageMapping> {
  if (reason != nullptr) *reason = text;
  return nullptr;
}

}  // namespace

auto ModuleImageMapping::Create(const VerifiedModuleImage& image,
                                QString* reason)
    -> std::unique_ptr<ModuleImageMapping> {
  // The type system already makes an unverified image unrepresentable; this is
  // the one remaining way to arrive here with nothing to load.
  if (!image.IsValid()) {
    return Fail(reason, "there is no verified image to load");
  }

  const auto fault = TakeFault();

  std::unique_ptr<ModuleImageMapping> mapping(new ModuleImageMapping());

#ifdef Q_OS_LINUX
  // Off by default. A module mapped from a memfd has no file for a debugger to
  // find symbols in, which is a real cost to anyone working on a module, so
  // there is a way to ask for the file-backed path without weakening anything:
  // the bytes are the same verified bytes either way.
  const auto force_file_backed =
      qEnvironmentVariableIsSet("GPGFRONTEND_MODULE_FILE_BACKED");

  if (!force_file_backed) {
    if (fault == ModuleImageFaultPoint::kCREATE_BACKING) {
      return Fail(reason, "a memory image could not be created");
    }

    const auto name = image.LibraryName().toLocal8Bit();
    const int fd = static_cast<int>(
        ::syscall(SYS_memfd_create, name.constData(), MFD_CLOEXEC));
    if (fd >= 0) {
      QFile sink;
      if (!sink.open(fd, QIODevice::WriteOnly, QFileDevice::DontCloseHandle)) {
        ::close(fd);
        return Fail(reason, "a memory image could not be opened");
      }
      const auto ok = WriteAll(sink, image.Bytes(), fault);
      sink.flush();
      sink.close();
      if (!ok) {
        ::close(fd);
        return Fail(reason, "the module image could not be written in full");
      }

      mapping->impl_->memfd = fd;
      mapping->impl_->anonymous = true;
      mapping->impl_->load_path = QString("/proc/self/fd/%1").arg(fd);
      return mapping;
    }
    // No memfd: an old kernel, or a sandbox that forbids the call. The
    // file-backed path below is the fallback rather than a failure.
    FLOG_D("memfd_create unavailable, falling back to a temporary file");
  }
#endif

  // ---- file-backed: the loader insists on a real path ----

  if (fault == ModuleImageFaultPoint::kCREATE_BACKING) {
    return Fail(reason, "a private folder for the module could not be made");
  }

  const auto root = PrivateRoot();
  if (!QDir().mkpath(root)) {
    return Fail(reason, "a private folder for the module could not be made");
  }

  auto dir = std::make_unique<QTemporaryDir>(root + "/img-XXXXXX");
  if (!dir->isValid()) {
    return Fail(reason, "a private folder for the module could not be made");
  }
  // It is ours to remove; a temporary directory that outlives the process is
  // what the startup sweep is for, not something to leave to chance here.
  dir->setAutoRemove(true);

  const auto dir_path = dir->path();
  QFile::setPermissions(dir_path, QFileDevice::ReadOwner |
                                      QFileDevice::WriteOwner |
                                      QFileDevice::ExeOwner);

  auto lock = std::make_unique<DirectoryLock>(dir_path);
  if (!lock->Held()) {
    return Fail(reason, "a private folder for the module could not be claimed");
  }

  const auto path = dir_path + "/" + image.LibraryName();
  {
    QFile out(path);
    if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
      return Fail(reason, "the module image could not be written");
    }
    const auto ok = WriteAll(out, image.Bytes(), fault);
    out.flush();
    out.close();
    if (!ok) {
      QFile::remove(path);
      return Fail(reason, "the module image could not be written in full");
    }
  }

  QFile::setPermissions(path, QFileDevice::ReadOwner | QFileDevice::ExeOwner);

  // The in-memory bytes are the source of truth and this file is only how the
  // loader gets at them, so the one question worth asking is whether the
  // transport was faithful. It catches a truncated or failed write, which is
  // the failure this path actually has; it is not a defence against a hostile
  // same-user process, and nothing here pretends it is.
  {
    QFile back(path);
    if (!back.open(QIODevice::ReadOnly)) {
      QFile::remove(path);
      return Fail(reason, "the module image could not be read back");
    }
    const auto written = back.readAll();
    back.close();

    const auto expected =
        Sha256Of(image.Bytes().constData(), image.Bytes().size());
    auto actual = Sha256Of(written.constData(), written.size());
    if (fault == ModuleImageFaultPoint::kREADBACK_DIFFERS) actual.clear();

    if (actual != expected) {
      QFile::remove(path);
      return Fail(reason,
                  "the module image did not read back as what was written");
    }
  }

  mapping->impl_->anonymous = false;
  mapping->impl_->load_path = path;
  mapping->impl_->AdoptDirectory(std::move(dir), std::move(lock));
  return mapping;
}

auto ModuleImageMapping::SweepAbandonedDirectories() -> int {
  const QDir root(PrivateRoot());
  if (!root.exists()) return 0;

  int removed = 0;
  const auto entries =
      root.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden);
  for (const auto& entry : entries) {
    // Taking the lock is the whole test. It succeeds only when no live process
    // holds it, because the kernel released it when that process ended --
    // however it ended. Nothing here consults a process id.
    DirectoryLock probe(entry.absoluteFilePath());
    if (!probe.Held()) continue;

    QDir victim(entry.absoluteFilePath());
    if (victim.removeRecursively()) ++removed;
  }
  return removed;
}

void SetModuleImageFaultForTesting(ModuleImageFaultPoint point) {
  g_fault.store(point);
}

}  // namespace GpgFrontend::Module
