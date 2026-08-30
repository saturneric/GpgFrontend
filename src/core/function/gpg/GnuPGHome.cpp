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

#include "GnuPGHome.h"

namespace GpgFrontend {

namespace {

/// Marks a link as this application's, and is what somebody staring at their
/// temporary directory sees. Not parsed back: nothing sweeps by name.
constexpr auto kLinkPrefix = "gfp-";

/// 32 bits of randomness. Wide enough that a collision is a curiosity rather
/// than a case to design around, and short enough that the whole link still
/// fits inside a macOS sandbox container's temporary directory.
constexpr int kLinkHexDigits = 8;

/// A collision means one more draw, not a failure. Bounded so that a root that
/// rejects every write fails promptly instead of spinning.
constexpr int kMintAttempts = 8;

auto MintLinkName() -> QString {
  return QLatin1String(kLinkPrefix) +
         QString::number(QRandomGenerator::global()->generate(), 16)
             .rightJustified(kLinkHexDigits, QLatin1Char('0'));
}

/// Every link name is the same length, so one sample settles whether a root can
/// host a link at all, before any key database is in hand.
auto SampleLinkPathIn(const QString& root) -> QString {
  return root + "/" + QLatin1String(kLinkPrefix) +
         QString(kLinkHexDigits, QLatin1Char('0'));
}

/// The runtime directory first: on Linux it is /run/user/<uid>, per-user, mode
/// 0700, on tmpfs, and far shorter than anything under the user's home. Qt
/// leaves it empty off XDG, where the temporary directory takes over -- inside
/// a macOS sandbox that is the container's own private tmp, which needs no
/// special case here because the budget check below measures it either way.
auto ChooseRoot() -> QString {
  const QStringList candidates = {
      QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation),
      QDir::tempPath()};

  for (const auto& candidate : candidates) {
    if (candidate.isEmpty()) continue;
    if (GnuPGSocketBudget::Fits(SampleLinkPathIn(candidate))) return candidate;
  }

  return {};
}

auto OverBudgetReason(const QString& path) -> QString {
  return QString(
             "gnupg homedir exceeds unix socket path limit: %1 bytes, max %2")
      .arg(path.toUtf8().size())
      .arg(GnuPGSocketBudget::Bytes());
}

}  // namespace

auto GnuPGSocketBudget::Bytes() -> int {
#ifdef Q_OS_WINDOWS
  // Windows gnupg does not address these sockets through sun_path, so there is
  // no length to budget against.
  return -1;
#else
#ifdef Q_OS_MACOS
  constexpr int kSunPathSize = 104;
#else
  constexpr int kSunPathSize = 108;
#endif
  constexpr int kLongestSocketSuffix =
      static_cast<int>(sizeof("/S.gpg-agent.browser") - 1);

  // -1 for the terminating NUL, which sun_path has to hold too.
  return kSunPathSize - kLongestSocketSuffix - 1;
#endif
}

auto GnuPGSocketBudget::Fits(const QString& path) -> bool {
  const auto budget = Bytes();
  if (budget < 0) return true;

  return path.toUtf8().size() <= budget;
}

GnuPGHomeLinkStore::GnuPGHomeLinkStore(int channel)
    : SingletonFunctionObject<GnuPGHomeLinkStore>(channel),
      root_(ChooseRoot()) {}

auto GnuPGHomeLinkStore::Root() const -> QString {
  QMutexLocker locker(&mutex_);
  return root_;
}

void GnuPGHomeLinkStore::UseRoot(const QString& root) {
  QMutexLocker locker(&mutex_);

  // Same filter the candidate search applies, so Root() means one thing however
  // it was set: a directory that can host a link inside the budget, or nothing.
  // Without it a caller could install a root too long to be any use, and the
  // resolver would hand GnuPG a link no shorter than the path it replaced.
  root_ = GnuPGSocketBudget::Fits(SampleLinkPathIn(root)) ? root : QString{};
}

auto GnuPGHomeLinkStore::Acquire(const QString& real_home) -> QString {
  QMutexLocker locker(&mutex_);

  // Sibling channels on one key database share a link rather than each minting
  // their own.
  if (const auto lease = leases_.constFind(real_home);
      lease != leases_.constEnd()) {
    return lease.value();
  }

  if (root_.isEmpty() || !QDir().mkpath(root_)) return {};

  for (int attempt = 0; attempt < kMintAttempts; ++attempt) {
    const auto link = root_ + "/" + MintLinkName();

    // QFile::link refuses an existing name, so this is only an early out for
    // the ordinary collision; the create itself is what decides.
    if (QFileInfo(link).isSymLink() || QFileInfo::exists(link)) continue;
    if (!QFile::link(real_home, link)) continue;

    leases_.insert(real_home, link);
    return link;
  }

  return {};
}

void GnuPGHomeLinkStore::ReleaseAll() {
  QMutexLocker locker(&mutex_);

  // Removing a symlink discards no data: the key database it points at is
  // untouched.
  for (const auto& link : leases_) QFile::remove(link);
  leases_.clear();
}

GnuPGHomeResolver::GnuPGHomeResolver(GnuPGHomeLinkStore& store)
    : store_(store) {}

auto GnuPGHomeResolver::Inspect(const QString& key_db_path) const -> GnuPGHome {
  GnuPGHome home;
  home.key_db_path = key_db_path;

  if (GnuPGSocketBudget::Fits(key_db_path)) {
    home.engine_path = key_db_path;
    return home;
  }

  if (store_.Root().isEmpty()) {
    home.unusable_reason = OverBudgetReason(key_db_path) +
                           "; no directory short enough to link from";
    return home;
  }

  // Usable, but the link does not exist until Provision() makes one, so there
  // is deliberately no engine_path yet.
  return home;
}

auto GnuPGHomeResolver::Provision(const QString& key_db_path) -> GnuPGHome {
  auto home = Inspect(key_db_path);

  // Rejected, or it already fits and needs nothing made for it.
  if (!home.IsUsable() || !home.engine_path.isEmpty()) return home;

  const auto link = store_.Acquire(key_db_path);
  if (link.isEmpty()) {
    home.unusable_reason =
        OverBudgetReason(key_db_path) + "; a shorter link could not be created";
    return home;
  }

  home.engine_path = link;
  return home;
}

}  // namespace GpgFrontend
