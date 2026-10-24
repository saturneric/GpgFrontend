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

#include <type_traits>

namespace GpgFrontend {

/**
 * @brief Which part of the application a setting belongs to.
 *
 * The unit a person thinks in. A new profile inherits whole groups, the
 * settings dialog could lay itself out by them, and a group is what a
 * migration note talks about -- so every setting has exactly one.
 */
enum class SettingGroup : std::uint8_t {
  kENGINE,       ///< which OpenPGP engine, and how GnuPG is found and run
  kRPGP,         ///< how the rPGP engine asks for and keeps passphrases
  kBEHAVIOUR,    ///< how everyday operations behave
  kAPPEARANCE,   ///< language, theme and what is shown on the way up
  kDIAGNOSTICS,  ///< logging
  kSECURITY,     ///< how this profile's own key is protected
  kNETWORK,      ///< updates and the proxy
  kWORKSPACE,    ///< where files are kept
  kLAYOUT,       ///< remembered tab orders and column choices
};

/**
 * @brief What it takes for a changed value to be in effect.
 */
enum class SettingRestart : std::uint8_t {
  kNONE,  ///< read whenever it is needed
  kUI,    ///< read while the interface is built
  kCORE,  ///< read while the core starts; a new process
};

/**
 * @brief Whether a value means the same thing on another computer.
 *
 * Decides what a profile package carries: a path to a GnuPG installation or
 * the name of this machine's credential store would not merely fail to apply
 * elsewhere, it would point the copy at the wrong thing.
 */
enum class SettingLocality : std::uint8_t {
  kPROFILE,  ///< part of the profile wherever it is opened
  kMACHINE,  ///< names something on this computer; stays here
};

/**
 * @brief Everything known about one setting except its type.
 *
 * One row per key the application reads or writes. Every question that used
 * to be answered by a string list somewhere else -- does this travel in a
 * package, may a new profile start with it, does changing it need a restart,
 * is it secret, what was it called before -- is answered from here instead,
 * so adding a setting means answering all of them once.
 *
 * Plain data, so a catalog of these can be constexpr and a pointer to one is
 * as good as its key.
 */
struct SettingMeta {
  /// The QSettings key, group included. A wire format: every existing
  /// profile already has values under it.
  const char *key;

  SettingGroup group;

  /// What to call it, untranslated; see Title(). Context
  /// "GpgFrontend::SettingItem".
  const char *title = "";

  /// One sentence on what it does, untranslated; see Description().
  const char *description = "";

  SettingRestart restart = SettingRestart::kNONE;
  SettingLocality locality = SettingLocality::kPROFILE;

  /// Whether a new profile may start with this window's value. Never for
  /// anything that points at the user's existing data or protects a key.
  bool inheritable = false;

  /// Holds a credential: never on a command line, never in a log.
  bool secret = false;

  /// The key a deployment override in profile.json pins this with, when it
  /// can be pinned at all.
  const char *deployment_key = nullptr;

  /// The release that first wrote this key; empty for anything older than
  /// this catalog.
  const char *since = "";

  /// The key this setting was stored under before, so a migration knows what
  /// to look for. A rename with a value conversion keeps its own rung in
  /// ProfileMigration; this records that there was one.
  const char *legacy_key = nullptr;

  /// The translated name.
  [[nodiscard]] auto Title() const -> QString {
    return QCoreApplication::translate("GpgFrontend::SettingItem", title);
  }

  /// The translated description.
  [[nodiscard]] auto Description() const -> QString {
    return QCoreApplication::translate("GpgFrontend::SettingItem", description);
  }
};

/**
 * @brief A setting with a type and the one default every reader shares.
 *
 * Reading through this rather than through QSettings::value() is what keeps
 * two places from disagreeing about what an unset value means -- which they
 * did: one reader of "confirm before importing" assumed yes and the dialog
 * that edits it assumed no, so opening Settings and pressing OK turned it off.
 *
 * @tparam T the value's type: bool, int, qint64 or QString
 */
template <typename T>
struct SettingItem : SettingMeta {
  /// QString cannot be constexpr; a string default is kept as a literal.
  using Stored =
      std::conditional_t<std::is_same_v<T, QString>, const char *, T>;

  Stored default_value{};

  /// The default, as the type a reader gets.
  [[nodiscard]] auto Default() const -> T { return T(default_value); }

  /// The stored value, or the default when there is none.
  [[nodiscard]] auto Read(const QSettings &settings) const -> T {
    const auto value = settings.value(QLatin1StringView(key));
    if (!value.isValid()) return Default();
    return value.template value<T>();
  }

  void Write(QSettings &settings, const T &value) const {
    settings.setValue(QLatin1StringView(key), value);
  }

  /// The key as a QString, for APIs that take one.
  [[nodiscard]] auto Key() const -> QString { return QLatin1StringView(key); }
};

using BoolSetting = SettingItem<bool>;
using IntSetting = SettingItem<int>;
using Int64Setting = SettingItem<qint64>;
using StringSetting = SettingItem<QString>;

/**
 * @brief A setting the application remembers rather than one a user sets.
 *
 * Tab orders and column choices: their shape belongs to the widget that owns
 * them, so there is no one type or default to give, only the metadata.
 */
using StateSetting = SettingMeta;

}  // namespace GpgFrontend
