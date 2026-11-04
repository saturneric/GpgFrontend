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

#include "KeyCategoryRepository.h"

#include <QCryptographicHash>

#include "core/function/CacheManager.h"
#include "core/function/openpgp/ChannelHandle.h"
#include "core/function/openpgp/helper/KeyDatabaseScopedCache.h"
#include "core/struct/cache_object/KeyCategoriesCO.h"
#include "core/utils/CommonUtils.h"

namespace GpgFrontend {

KeyCategoryRepository::KeyCategoryRepository(int channel)
    : SingletonFunctionObject<KeyCategoryRepository>(channel) {
  fetch_categories();
}

auto KeyCategoryRepository::Fetch() -> QList<KeyCategoryCO> {
  return categories_;
}

auto KeyCategoryRepository::find_category(const QString& id) -> KeyCategoryCO* {
  for (auto& c : categories_) {
    if (c.id == id) return &c;
  }
  return nullptr;
}

void KeyCategoryRepository::fetch_categories() {
  categories_.clear();
  tab_colors_.clear();
  tab_orders_.clear();

  auto json = LoadKeyDatabaseScopedCache(cm_, "kcats", ctx_.KeyDBId(),
                                         ctx_.KeyDBName());
  if (!json) return;

  auto co = KeyCategoriesCO(*json);
  categories_ = co.categories;
  tab_colors_ = co.tab_colors;
  tab_orders_ = co.tab_orders;
}

void KeyCategoryRepository::persist_categories() {
  // A retired channel's copy was loaded before the Runtime that replaced it,
  // which has its own copy of the same database's entry by now. Work still
  // finishing here must not write the older one over it.
  if (IsRetiredChannelHandle(GetChannel())) {
    LOG_W() << "not persisting from a retired channel:" << GetChannel();
    return;
  }

  KeyCategoriesCO co;
  co.key_db_name = ctx_.KeyDBName();
  co.categories = categories_;
  co.tab_colors = tab_colors_;
  co.tab_orders = tab_orders_;

  SaveKeyDatabaseScopedCache(cm_, "kcats", ctx_.KeyDBId(), ctx_.KeyDBName(),
                             co.ToJson());
}

auto KeyCategoryRepository::AddCategory(const QString& name,
                                        const QString& color) -> QString {
  KeyCategoryCO c;
  c.id = "cat:" + QUuid::createUuid().toRfc4122().toHex().left(14).toUpper();
  c.name = name;
  c.color = color;
  c.builtin = false;
  c.creation_time = CurrentDateTime();

  categories_.push_back(c);
  persist_categories();
  return c.id;
}

auto KeyCategoryRepository::Remove(const QString& id) -> bool {
  auto* c = find_category(id);
  if (c == nullptr || c->builtin) return false;

  for (auto it = categories_.begin(); it != categories_.end(); ++it) {
    if (it->id == id) {
      categories_.erase(it);
      break;
    }
  }
  persist_categories();
  return true;
}

auto KeyCategoryRepository::Rename(const QString& id, const QString& name)
    -> bool {
  auto* c = find_category(id);
  if (c == nullptr || c->builtin) return false;

  c->name = name;
  persist_categories();
  return true;
}

auto KeyCategoryRepository::AddKey2Category(const QString& id,
                                            const QString& key_id) -> bool {
  auto* c = find_category(id);
  if (c == nullptr || key_id.isEmpty() || c->key_ids.contains(key_id)) {
    return false;
  }

  c->key_ids.append(key_id);
  persist_categories();
  return true;
}

auto KeyCategoryRepository::RemoveKeyFromCategory(const QString& id,
                                                  const QString& key_id)
    -> bool {
  auto* c = find_category(id);
  if (c == nullptr || !c->key_ids.contains(key_id)) return false;

  c->key_ids.removeAll(key_id);
  persist_categories();
  return true;
}

auto KeyCategoryRepository::Contains(const QString& id, const QString& key_id)
    -> bool {
  auto* c = find_category(id);
  return c != nullptr && c->key_ids.contains(key_id);
}

auto KeyCategoryRepository::KeyIdsOf(const QString& id) -> QStringList {
  auto* c = find_category(id);
  return c != nullptr ? c->key_ids : QStringList{};
}

auto KeyCategoryRepository::GetTabColor(const QString& id) -> QString {
  if (const auto it = tab_colors_.constFind(id); it != tab_colors_.constEnd()) {
    return it.value();
  }
  // Fall back to a custom category's own colour, if any.
  if (auto* c = find_category(id); c != nullptr) return c->color;
  return {};
}

void KeyCategoryRepository::SetTabColor(const QString& id,
                                        const QString& color) {
  if (color.isEmpty()) {
    tab_colors_.remove(id);
  } else {
    tab_colors_.insert(id, color);
  }
  persist_categories();
}

auto KeyCategoryRepository::GetTabOrder(const QString& scope) -> QStringList {
  return tab_orders_.value(scope);
}

void KeyCategoryRepository::SetTabOrder(const QString& scope,
                                        const QStringList& order) {
  if (order.isEmpty()) {
    tab_orders_.remove(scope);
  } else {
    tab_orders_.insert(scope, order);
  }
  persist_categories();
}

auto KeyCategoryRepository::FlushCache() -> bool {
  fetch_categories();
  return true;
}

namespace {

// Hues nearer than this read as the same colour at swatch size.
constexpr int kMinKeyCategoryHueDistance = 12;
// Below this a colour reads as a grey (the slate "All" tab is 72): its hue
// says nothing about how it looks.
constexpr int kMinChromaticSaturation = 100;

auto LooksAlike(const QColor& a, const QColor& b) -> bool {
  if (a.rgb() == b.rgb()) return true;
  if (a.hsvSaturation() < kMinChromaticSaturation ||
      b.hsvSaturation() < kMinChromaticSaturation) {
    return false;
  }

  const int diff = std::abs(a.hsvHue() - b.hsvHue());
  return std::min(diff, 360 - diff) < kMinKeyCategoryHueDistance;
}

}  // namespace

// The translate() context below is spelled out at every call site on purpose.
// lupdate is a static parser: hoisting it into a constant silently drops every
// string here from the .ts files, with no warning and no compile error.

auto ValidateKeyCategoryName(const QString& name, const QStringList& taken)
    -> KeyCategoryNameProblem {
  const auto trimmed = name.trimmed();
  if (trimmed.isEmpty()) return KeyCategoryNameProblem::kEmpty;
  if (trimmed.size() > kMaxKeyCategoryNameLength) {
    return KeyCategoryNameProblem::kTooLong;
  }

  const bool duplicate =
      std::any_of(taken.cbegin(), taken.cend(), [&](const QString& other) {
        return other.trimmed().compare(trimmed, Qt::CaseInsensitive) == 0;
      });
  if (duplicate) return KeyCategoryNameProblem::kDuplicate;

  return KeyCategoryNameProblem::kNone;
}

auto DescribeKeyCategoryNameProblem(KeyCategoryNameProblem problem) -> QString {
  switch (problem) {
    case KeyCategoryNameProblem::kNone:
    case KeyCategoryNameProblem::kEmpty:
      return {};

    case KeyCategoryNameProblem::kTooLong:
      return QCoreApplication::translate("GpgFrontend::UI::KeyCategoryRules",
                                         "The name is too long.");

    case KeyCategoryNameProblem::kDuplicate:
      return QCoreApplication::translate(
          "GpgFrontend::UI::KeyCategoryRules",
          "A category with this name already exists.");
  }

  return {};
}

auto KeyCategoryPresetColors()
    -> std::array<KeyCategoryPresetColor, kKeyCategoryPresetColorCount> {
  // The same tone family as the built-in tabs' colours, so a custom category
  // sits beside them in the rail without looking out of place. In hue order,
  // so the grid reads as a colour wheel; a few deep tones close it.
  return {{
      {QColor(0xEF4444),
       QCoreApplication::translate("GpgFrontend::UI::KeyCategoryRules", "Red")},
      {QColor(0xF97316), QCoreApplication::translate(
                             "GpgFrontend::UI::KeyCategoryRules", "Orange")},
      {QColor(0xF59E0B), QCoreApplication::translate(
                             "GpgFrontend::UI::KeyCategoryRules", "Amber")},
      {QColor(0xEAB308), QCoreApplication::translate(
                             "GpgFrontend::UI::KeyCategoryRules", "Yellow")},
      {QColor(0x84CC16), QCoreApplication::translate(
                             "GpgFrontend::UI::KeyCategoryRules", "Lime")},
      {QColor(0x22C55E), QCoreApplication::translate(
                             "GpgFrontend::UI::KeyCategoryRules", "Green")},
      {QColor(0x10B981), QCoreApplication::translate(
                             "GpgFrontend::UI::KeyCategoryRules", "Emerald")},
      {QColor(0x14B8A6), QCoreApplication::translate(
                             "GpgFrontend::UI::KeyCategoryRules", "Teal")},
      {QColor(0x06B6D4), QCoreApplication::translate(
                             "GpgFrontend::UI::KeyCategoryRules", "Cyan")},
      {QColor(0x0EA5E9),
       QCoreApplication::translate("GpgFrontend::UI::KeyCategoryRules", "Sky")},
      {QColor(0x3B82F6), QCoreApplication::translate(
                             "GpgFrontend::UI::KeyCategoryRules", "Blue")},
      {QColor(0x6366F1), QCoreApplication::translate(
                             "GpgFrontend::UI::KeyCategoryRules", "Indigo")},
      {QColor(0x8B5CF6), QCoreApplication::translate(
                             "GpgFrontend::UI::KeyCategoryRules", "Violet")},
      {QColor(0xA855F7), QCoreApplication::translate(
                             "GpgFrontend::UI::KeyCategoryRules", "Purple")},
      {QColor(0xD946EF), QCoreApplication::translate(
                             "GpgFrontend::UI::KeyCategoryRules", "Fuchsia")},
      {QColor(0xEC4899), QCoreApplication::translate(
                             "GpgFrontend::UI::KeyCategoryRules", "Pink")},
      {QColor(0xF43F5E), QCoreApplication::translate(
                             "GpgFrontend::UI::KeyCategoryRules", "Rose")},
      {QColor(0x92400E), QCoreApplication::translate(
                             "GpgFrontend::UI::KeyCategoryRules", "Brown")},
      {QColor(0x1E3A8A), QCoreApplication::translate(
                             "GpgFrontend::UI::KeyCategoryRules", "Navy")},
      {QColor(0x78716C), QCoreApplication::translate(
                             "GpgFrontend::UI::KeyCategoryRules", "Stone")},
  }};
}

auto SuggestKeyCategoryColor(const QList<QColor>& used) -> QColor {
  const auto presets = KeyCategoryPresetColors();

  for (const auto& preset : presets) {
    const bool taken = std::any_of(
        used.cbegin(), used.cend(),
        [&](const QColor& c) { return LooksAlike(c, preset.color); });
    if (!taken) return preset.color;
  }

  return presets.at(static_cast<std::size_t>(used.size()) % presets.size())
      .color;
}

auto KeyCategoryFallbackColor(const QString& id) -> QColor {
  // Well spread over the hues. Not qHash(): its seed is random per process,
  // so the colour would change on every launch.
  const auto digest =
      QCryptographicHash::hash(id.toUtf8(), QCryptographicHash::Sha256);
  const auto bits = (static_cast<quint8>(digest.at(0)) << 8) |
                    static_cast<quint8>(digest.at(1));
  const auto hue = static_cast<int>(static_cast<unsigned>(bits) % 360U);
  return QColor::fromHsv(hue, 160, 205);
}

auto KeyCategoryDefaultColor(const QString& id) -> QColor {
  // Semantic default colours for the well-known built-in tabs.
  static const QHash<QString, QString> kBuiltinColors = {
      {"default", "#3B82F6"},          // blue
      {"mine", "#10B981"},             // emerald
      {"key_group", "#8B5CF6"},        // violet
      {"only_public_key", "#06B6D4"},  // cyan
      {"has_private_key", "#F43F5E"},  // rose
      {"all", "#64748B"},              // slate
      {"no_primary_key", "#9CA3AF"},   // gray
      {"expiring_soon", "#F59E0B"},    // amber
      {"revoked", "#EF4444"},          // red
      {"expired", "#F97316"},          // orange
      {"disabled", "#71717A"},         // zinc
  };

  if (const auto it = kBuiltinColors.constFind(id);
      it != kBuiltinColors.constEnd()) {
    return QColor(*it);
  }
  return KeyCategoryFallbackColor(id);
}

}  // namespace GpgFrontend
