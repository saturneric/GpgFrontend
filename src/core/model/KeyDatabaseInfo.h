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

#include <optional>

namespace GpgFrontend {

/// What a key database is, which decides what may be done to it and whether it
/// travels. Three kinds rather than a flat list because they are not variations
/// on one thing: only one of them is stored configuration at all.
///
/// Recorded with the database rather than worked out from its path each time.
/// A path says where a database is now; the kind says what the user meant it to
/// be, and that is the thing a package has to honour. Derived from the path
/// only for an entry written before the field existed.
enum class KeyDatabaseKind {
  kDEFAULT,   ///< derived from this computer's engine; never stored, never
              ///< packed
  kMANAGED,   ///< inside the profile, in a directory a package carries
  kEXTERNAL,  ///< somewhere the user pointed; this computer's arrangement alone
};

/// The spelling that goes into settings and into a package manifest. Words
/// rather than numbers, so a stored list stays readable and so inserting a kind
/// later cannot silently renumber the ones already written.
inline auto ConvertKeyDatabaseKind2String(KeyDatabaseKind kind) -> QString {
  switch (kind) {
    case KeyDatabaseKind::kDEFAULT:
      return "default";
    case KeyDatabaseKind::kMANAGED:
      return "managed";
    case KeyDatabaseKind::kEXTERNAL:
      return "external";
  }
  return "external";
}

/// Returns nothing for a value this build does not know, including the empty
/// string an older settings file leaves behind. The caller then derives the
/// kind from the path, which is what every build before this field did.
inline auto ConvertString2KeyDatabaseKind(const QString& kind)
    -> std::optional<KeyDatabaseKind> {
  const auto normalized = kind.trimmed().toLower();
  if (normalized == "default") return KeyDatabaseKind::kDEFAULT;
  if (normalized == "managed") return KeyDatabaseKind::kMANAGED;
  if (normalized == "external") return KeyDatabaseKind::kEXTERNAL;
  return std::nullopt;
}

struct KeyDatabaseInfo {
  int channel;
  QString name;
  QString path;
  QString origin_path;
  bool valid{false};
  QString backend_type;

  /// Defaults to the kind that can do the least: external databases never
  /// travel and never take channel 0, so an entry whose kind was never settled
  /// cannot smuggle itself into a package by being left uninitialised.
  KeyDatabaseKind kind{KeyDatabaseKind::kEXTERNAL};

  KeyDatabaseInfo() = default;
};

}  // namespace GpgFrontend