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

#include <algorithm>
#include <optional>

#include "core/model/KeyDatabaseInfo.h"
#include "core/struct/settings_object/KeyDatabaseItemSO.h"
#include "core/struct/settings_object/KeyDatabaseListSO.h"
#include "core/utils/GpgUtils.h"

namespace GpgFrontend::UI {

/**
 * @brief What the Key Databases page holds until it is applied.
 *
 * The rules of the page without its widgets: which database is the default,
 * whether one may be made the default, what an explicit choice becomes when
 * the database it names goes away, and what is written back. The page shows
 * this and writes it on OK; Cancel simply drops it.
 *
 * The effective default is asked of the same rules the core opens the
 * databases with, so what the page marks is what the next start, or the next
 * apply, opens as the default.
 */
class KeyDatabaseSettingsModel {
 public:
  /// The built-in database, present exactly when it is turned on.
  std::optional<KeyDatabaseInfo> builtin;
  QContainer<KeyDatabaseInfo> managed;   ///< kept inside the profile
  QContainer<KeyDatabaseInfo> external;  ///< on this computer only

  /// The database the user made the default, empty when none was chosen. May
  /// name one that is not in the list; the fallback then applies, as it does
  /// in the core.
  QString choice_id;

  /// The engine the built-in database is opened with (`basic/default_engine`),
  /// upper-cased.
  QString engine;

  /**
   * @brief Fill from what settings hold.
   *
   * @param all every configured database, as GetAllKeyDatabaseInfoBySettings()
   * returns them
   * @param stored_choice the stored `default_database_id`
   * @param stored_engine the stored `basic/default_engine`
   */
  void Load(const QContainer<KeyDatabaseInfo>& all,
            const QString& stored_choice, const QString& stored_engine) {
    builtin.reset();
    managed.clear();
    external.clear();
    for (const auto& info : all) {
      switch (info.kind) {
        case KeyDatabaseKind::kDEFAULT:
          builtin = info;
          break;
        case KeyDatabaseKind::kMANAGED:
          managed.append(info);
          break;
        case KeyDatabaseKind::kEXTERNAL:
          external.append(info);
          break;
      }
    }
    choice_id = stored_choice;
    engine = stored_engine.toUpper();
  }

  /**
   * @brief The list as it would be stored, in the order the core gives it.
   */
  [[nodiscard]] auto Compose() const -> QContainer<KeyDatabaseItemSO> {
    const auto to_items = [](const QContainer<KeyDatabaseInfo>& infos) {
      QContainer<KeyDatabaseItemSO> items;
      items.reserve(infos.size());
      for (const auto& info : infos) items.append(KeyDatabaseItemSO(info));
      return items;
    };

    std::optional<KeyDatabaseItemSO> builtin_item;
    if (builtin) builtin_item = KeyDatabaseItemSO(*builtin);
    return ComposeKeyDatabaseList(builtin_item, to_items(managed),
                                  to_items(external));
  }

  /**
   * @brief The database that would be opened as the default.
   *
   * The choice while it names a database in the list, else the built-in one,
   * else the first (ResolveDefaultKeyDatabaseId). One whose folder could not
   * be resolved is skipped for the first usable one, as the core skips it when
   * it opens the list.
   *
   * @return its id, empty for an empty list
   */
  [[nodiscard]] auto EffectiveDefaultId() const -> QString {
    const auto items = Compose();
    auto resolved = ResolveDefaultKeyDatabaseId(choice_id, items);
    if (is_usable(resolved)) return resolved;
    for (const auto& item : items) {
      if (is_usable(item.id)) return item.id;
    }
    return resolved;
  }

  /**
   * @brief Whether @p id is in the list as it stands.
   */
  [[nodiscard]] auto Contains(const QString& id) const -> bool {
    return find(id) != nullptr;
  }

  /**
   * @brief Whether "Use as Default" is offered for @p id.
   *
   * Only for a database that is in the list -- the built-in one only while it
   * is turned on -- and is not already the default.
   */
  [[nodiscard]] auto CanUseAsDefault(const QString& id) const -> bool {
    return !id.isEmpty() && Contains(id) && id != EffectiveDefaultId();
  }

  /**
   * @brief Make @p id the default.
   *
   * @return false when it cannot be (see CanUseAsDefault())
   */
  auto UseAsDefault(const QString& id) -> bool {
    if (!CanUseAsDefault(id)) return false;
    choice_id = id;
    return true;
  }

  /**
   * @brief Turn the built-in database on (@p info) or off (nothing).
   *
   * Turning it off withdraws an explicit choice of it. Turning it back on
   * does not restore that choice: it becomes the default again only by the
   * fallback, when nothing else was chosen.
   */
  void SetBuiltIn(std::optional<KeyDatabaseInfo> info) {
    if (!info && choice_id == QLatin1String(kDefaultKeyDatabaseId)) {
      choice_id.clear();
    }
    builtin = std::move(info);
  }

  /**
   * @brief Remove row @p row of @p list, one of managed or external.
   *
   * Removing the chosen default withdraws the choice.
   */
  void RemoveAt(QContainer<KeyDatabaseInfo>& list, int row) {
    if (row < 0 || row >= list.size()) return;
    if (!choice_id.isEmpty() && list[row].id == choice_id) choice_id.clear();
    list.removeAt(row);
  }

  /**
   * @brief Write the list and the choice into @p list_so.
   *
   * The same on every store path, the one that overrides an unreadable list
   * included.
   */
  void ApplyTo(KeyDatabaseListSO& list_so) const {
    list_so.key_databases = Compose();
    list_so.default_database_id = choice_id;
  }

  /**
   * @brief The engine the built-in database would be opened with.
   *
   * What the core decides (ChooseChannelZeroEngine), so a choice this build
   * cannot honour shows the engine it falls back to.
   */
  [[nodiscard]] auto BuiltInEngine(bool gnupg_supported,
                                   bool rpgp_supported) const -> EngineChoice {
    return ChooseChannelZeroEngine(
        QLatin1String(kDefaultKeyDatabaseName),
        builtin ? builtin->backend_type : QString{}, engine, gnupg_supported,
        rpgp_supported);
  }

 private:
  [[nodiscard]] auto find(const QString& id) const -> const KeyDatabaseInfo* {
    if (id.isEmpty()) return nullptr;
    if (builtin && builtin->id == id) return &*builtin;
    for (const auto* list : {&managed, &external}) {
      const auto it = std::find_if(
          list->begin(), list->end(),
          [&](const KeyDatabaseInfo& info) { return info.id == id; });
      if (it != list->end()) return &*it;
    }
    return nullptr;
  }

  /// Opened by the core only when its folder resolved.
  [[nodiscard]] auto is_usable(const QString& id) const -> bool {
    const auto* info = find(id);
    return info != nullptr && !info->path.isEmpty();
  }
};

}  // namespace GpgFrontend::UI
