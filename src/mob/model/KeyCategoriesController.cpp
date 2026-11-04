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

#include "model/KeyCategoriesController.h"

#include <array>

#include "core/function/SettingCatalog.h"
#include "core/function/openpgp/ChannelHandle.h"
#include "core/function/openpgp/KeyCategoryRepository.h"
#include "core/profile/ProfileSession.h"
#include "model/ChangeRelay.h"
#include "model/KeyFilterModel.h"

namespace GpgFrontend::Mobile {

namespace {

/// Tells every controller of a change any of them made.
auto CategoriesRelay() -> ChangeRelay& {
  static ChangeRelay relay;
  return relay;
}

auto Repository() -> KeyCategoryRepository& {
  return KeyCategoryRepository::GetInstance(UsableDefaultChannelHandle());
}

auto OrderKey() -> QString {
  return QString::fromLatin1(SettingItems::kCustomCategoryOrder.key);
}

int g_revision = 0;

/// Categories live in the open profile's key database: none before.
auto Ready() -> bool { return ProfileSession::Loaded(); }

}  // namespace

KeyCategoriesController::KeyCategoriesController(QObject* parent)
    : QObject(parent) {
  connect(&CategoriesRelay(), &ChangeRelay::SignalChanged, this,
          &KeyCategoriesController::SignalChanged);
}

auto KeyCategoriesController::ordered_ids() const -> QStringList {
  if (!Ready()) return {};
  auto& repo = Repository();
  QStringList ids;
  for (const auto& c : repo.Fetch()) {
    if (!c.builtin) ids << c.id;
  }
  // The desktop's order for its custom tabs, then whatever it does not name.
  QStringList ordered;
  for (const auto& id : repo.GetTabOrder(OrderKey())) {
    if (ids.contains(id) && !ordered.contains(id)) ordered << id;
  }
  for (const auto& id : ids) {
    if (!ordered.contains(id)) ordered << id;
  }
  return ordered;
}

auto KeyCategoriesController::Categories() const -> QVariantList {
  if (!Ready()) return {};
  auto& repo = Repository();
  const auto all = repo.Fetch();
  QVariantList list;
  for (const auto& id : ordered_ids()) {
    for (const auto& c : all) {
      if (c.id != id) continue;
      const auto color = repo.GetTabColor(c.id);
      list.append(QVariantMap{
          {QStringLiteral("id"), c.id},
          {QStringLiteral("name"), c.name},
          {QStringLiteral("color"),
           color.isEmpty() ? KeyCategoryFallbackColor(c.id).name() : color},
          {QStringLiteral("count"), c.key_ids.size()},
      });
    }
  }
  return list;
}

auto KeyCategoriesController::Builtins() const -> QVariantList {
  struct Builtin {
    const char* id;
    QString name;
  };
  const std::array<Builtin, 6> builtins{{
      {"has_private_key", tr("My keys")},
      {"only_public_key", tr("Others' keys")},
      {kKeyGroupCategory, tr("Key groups")},
      {"expiring_soon", tr("Expiring soon")},
      {"expired", tr("Expired")},
      {"revoked", tr("Revoked")},
  }};
  QVariantList list;
  for (const auto& b : builtins) {
    // A colour chosen for the tab on the desktop wins, as it does there.
    const auto chosen =
        Ready() ? Repository().GetTabColor(QString::fromLatin1(b.id))
                : QString();
    list.append(QVariantMap{
        {QStringLiteral("id"), QString::fromLatin1(b.id)},
        {QStringLiteral("name"), b.name},
        {QStringLiteral("color"),
         chosen.isEmpty()
             ? KeyCategoryDefaultColor(QString::fromLatin1(b.id)).name()
             : chosen},
        {QStringLiteral("builtin"), true},
    });
  }
  return list;
}

auto KeyCategoriesController::PresetColors() const -> QVariantList {
  QVariantList list;
  for (const auto& preset : KeyCategoryPresetColors()) {
    list.append(QVariantMap{{QStringLiteral("color"), preset.color.name()},
                            {QStringLiteral("name"), preset.name}});
  }
  return list;
}

auto KeyCategoriesController::Revision() const -> int { return g_revision; }

auto KeyCategoriesController::suggestColor() const -> QString {
  // The built-in tabs' colours count too, as on the desktop: a new category
  // should not look like "Revoked".
  QList<QColor> used;
  for (const auto& c : Builtins() + Categories()) {
    used << QColor(c.toMap().value(QStringLiteral("color")).toString());
  }
  return SuggestKeyCategoryColor(used).name();
}

auto KeyCategoriesController::check_name(const QString& name,
                                         const QString& except) const
    -> QString {
  QStringList taken;
  for (const auto& c : Repository().Fetch()) {
    if (c.id != except) taken << c.name;
  }
  const auto problem = ValidateKeyCategoryName(name, taken);
  if (problem == KeyCategoryNameProblem::kNone) return {};
  const auto said = DescribeKeyCategoryNameProblem(problem);
  return said.isEmpty() ? tr("Give it a name.") : said;
}

auto KeyCategoriesController::add(const QString& name, const QString& color)
    -> QString {
  if (!Ready()) return tr("No profile is open.");
  if (auto why = check_name(name, {}); !why.isEmpty()) return why;
  auto& repo = Repository();
  // Kept with the category itself, as the desktop's dialog keeps it.
  const auto id = repo.AddCategory(name.trimmed(), color);
  // At the end of the shared order, as the desktop puts a new tab.
  auto order = ordered_ids();
  repo.SetTabOrder(OrderKey(), order);
  changed();
  emit SignalAdded(id);
  return {};
}

auto KeyCategoriesController::edit(const QString& id, const QString& name,
                                   const QString& color) -> QString {
  if (!Ready()) return tr("No profile is open.");
  if (auto why = check_name(name, id); !why.isEmpty()) return why;
  auto& repo = Repository();
  if (!repo.Rename(id, name.trimmed())) return tr("This category is gone.");
  repo.SetTabColor(id, color);
  changed();
  return {};
}

void KeyCategoriesController::remove(const QString& id) {
  if (!Ready()) return;
  auto& repo = Repository();
  if (!repo.Remove(id)) return;
  repo.SetTabColor(id, {});
  repo.SetTabOrder(OrderKey(), ordered_ids());
  changed();
}

void KeyCategoriesController::move(const QString& id, int by) {
  if (!Ready()) return;
  auto order = ordered_ids();
  const auto from = order.indexOf(id);
  const auto to = from + by;
  if (from < 0 || to < 0 || to >= order.size()) return;
  order.move(from, to);
  Repository().SetTabOrder(OrderKey(), order);
  changed();
}

auto KeyCategoriesController::keyIdsOf(const QString& id) const -> QStringList {
  if (!Ready()) return {};
  return Repository().KeyIdsOf(id);
}

auto KeyCategoriesController::categoriesOf(const QString& key_id) const
    -> QStringList {
  if (!Ready()) return {};
  auto& repo = Repository();
  QStringList ids;
  for (const auto& id : ordered_ids()) {
    if (repo.Contains(id, key_id)) ids << id;
  }
  return ids;
}

void KeyCategoriesController::setMember(const QString& id,
                                        const QString& key_id, bool member) {
  if (!Ready()) return;
  auto& repo = Repository();
  const auto done = member ? repo.AddKey2Category(id, key_id)
                           : repo.RemoveKeyFromCategory(id, key_id);
  if (done) changed();
}

auto KeyCategoriesController::setMembers(const QString& id,
                                         const QStringList& key_ids,
                                         bool member) -> int {
  if (!Ready()) return 0;
  auto& repo = Repository();
  int count = 0;
  for (const auto& key_id : key_ids) {
    const auto done = member ? repo.AddKey2Category(id, key_id)
                             : repo.RemoveKeyFromCategory(id, key_id);
    if (done) ++count;
  }
  if (count > 0) changed();
  return count;
}

void KeyCategoriesController::NotifyChanged() {
  ++g_revision;
  emit CategoriesRelay().SignalChanged();
}

void KeyCategoriesController::changed() { NotifyChanged(); }

}  // namespace GpgFrontend::Mobile
