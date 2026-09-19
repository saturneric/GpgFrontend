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

#pragma once

#include <memory>

#include "core/module/ModulePackageVerifier.h"

class QTemporaryDir;

namespace GpgFrontend::Module {

/**
 * @brief Gives a verified module image a form the native loader will accept.
 *
 * `QLibrary::load()` takes a path, so verified bytes have to become something
 * the operating system's loader can open. What "something" means is the only
 * part of module loading that differs per platform, and it is confined here:
 * everything above this class works with a path and never learns what is
 * behind it.
 *
 * ## What is and is not claimed
 *
 * GpgFrontend never installs or persistently caches module binaries. Verified
 * bytes are materialised only as far as the native loader requires, in private
 * per-process ephemeral storage, and removed as soon as the platform permits.
 *
 * On Linux that requirement is nothing at all: an anonymous `memfd` is loaded
 * through `/proc/self/fd/<n>` and never appears in any directory. On macOS and
 * Windows the loader insists on a real file, so one is written into a private
 * per-process temporary directory. There the file is a *transport format*, not
 * a cache: the verified in-memory bytes remain the source of truth, the file is
 * written from them, checked, and unlinked as soon as the platform allows.
 *
 * Against a hostile process running as the same user this boundary offers
 * nothing, and none is claimed -- such a process can already attach to this
 * one. What it does defend against is a partial or failed write, which is a
 * real failure mode and is why the written file is checked before it is loaded.
 *
 * ## Dependencies
 *
 * A module loaded from `/proc/self/fd/<n>` has no meaningful directory, so it
 * cannot resolve `$ORIGIN`-relative dependencies. Packaged modules must
 * therefore be self-contained apart from host and system libraries, which is
 * true of every module today: no RPATH in the tree uses `$ORIGIN`, and the only
 * non-system dependency is `gf_sdk`, which lives with the application. If a
 * package ever carries private native libraries of its own, this class must
 * fall back to directory-backed materialisation for it -- the memfd path cannot
 * serve them, and pretending otherwise would fail at `dlopen` time with an
 * error pointing nowhere near the cause.
 */
class GF_CORE_EXPORT ModuleImageMapping {
 public:
  ~ModuleImageMapping();

  ModuleImageMapping(const ModuleImageMapping&) = delete;
  auto operator=(const ModuleImageMapping&) -> ModuleImageMapping& = delete;

  /**
   * @brief Materialise a verified image, or fail without materialising one.
   *
   * Takes a VerifiedModuleImage rather than bytes and a name, so there is no
   * way to hand this anything that verification has not already vouched for.
   *
   * Fails closed: on any failure -- the backing could not be created, the write
   * was short, the written file did not read back as what was written -- this
   * returns nullptr and there is no path for a caller to load. A caller must
   * not reach `QLibrary::load()` without a mapping in hand.
   *
   * @param image the verified module image to materialise
   * @param[out] reason why it could not be materialised, when it could not
   * @return the mapping, or nullptr
   */
  [[nodiscard]] static auto Create(const VerifiedModuleImage& image,
                                   QString* reason = nullptr)
      -> std::unique_ptr<ModuleImageMapping>;

  /// The path to hand the native loader. Never empty on a live mapping.
  [[nodiscard]] auto LoadPath() const -> QString;

  /// True when nothing about this image exists in any directory.
  [[nodiscard]] auto IsAnonymous() const -> bool;

  /**
   * @brief Tell the mapping the loader has finished with the path.
   *
   * Where a platform allows an open image to be unlinked -- everywhere POSIX --
   * this is when it happens, so the file's lifetime is the load and not the
   * process. Where it does not, notably Windows, the file stays until the
   * mapping is destroyed. Removal here is best-effort by design: a platform
   * configuration that declines it is not a failed load, and treating it as one
   * would turn a tidy-up into an outage.
   */
  void NotifyLoaded();

  /**
   * @brief Remove private module directories no live process is using.
   *
   * A crash leaves a directory behind. Ownership is decided by an advisory lock
   * the owning process holds for its whole lifetime and the kernel releases
   * when it dies -- not by a recorded process id, which is reused and would
   * eventually name somebody else's directory. A directory whose lock cannot be
   * taken is in use and is left alone.
   *
   * @return how many directories were removed
   */
  static auto SweepAbandonedDirectories() -> int;

  /**
   * @brief The private directory file-backed images are materialized into.
   *
   * The territory SweepAbandonedDirectories() owns, and nothing else's. Named
   * here rather than recomputed by callers: a test that wants to assert
   * nothing was left behind has to look in the same place this writes, and a
   * second copy of the rule is a second place for it to be wrong.
   *
   * @return the root path, which may not exist yet
   */
  static auto PrivateRoot() -> QString;

 private:
  ModuleImageMapping();

  class Impl;
  std::unique_ptr<Impl> impl_;
};

/**
 * @brief Where a materialisation can be made to fail, for tests.
 *
 * Failing closed is the property that matters most here and it is unreachable
 * from outside: a write does not go short because a test asked nicely. This
 * exists so the refusal paths can be exercised, and so a test can assert the
 * thing that actually matters -- that no load is attempted afterwards.
 */
enum class ModuleImageFaultPoint {
  kNONE,
  kCREATE_BACKING,   ///< the memfd or temporary directory cannot be made
  kSHORT_WRITE,      ///< fewer bytes land than were handed over
  kWRITE_FAILS,      ///< the write reports an error
  kREADBACK_DIFFERS  ///< the file reads back as something else
};

/// Arm the next materialisation to fail at @p point. Tests only.
void GF_CORE_EXPORT SetModuleImageFaultForTesting(ModuleImageFaultPoint point);

}  // namespace GpgFrontend::Module
