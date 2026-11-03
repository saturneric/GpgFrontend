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

#include "model/SettingsModel.h"

#include "core/function/GlobalSettingStation.h"
#include "core/function/SettingCatalog.h"
#include "core/function/rpgp/PasswordFetcher.h"
#include "core/utils/RustUtils.h"

namespace GpgFrontend::Mobile {

namespace {

auto Meta(const SettingsModel::Item& item) -> const SettingMeta* {
  return std::visit([](const auto* setting) -> const SettingMeta* {
    return setting;
  }, item);
}

/// How a page edits @p item's value: by the type the setting holds.
auto TypeName(const SettingsModel::Item& item) -> QString {
  return std::visit(
      [](const auto* setting) -> QString {
        using T = decltype(setting->Default());
        if constexpr (std::is_same_v<T, bool>) {
          return QStringLiteral("bool");
        } else if constexpr (std::is_same_v<T, QString>) {
          return QStringLiteral("string");
        } else {
          return QStringLiteral("int");
        }
      },
      item);
}

/// The rPGP cache reads its times at start; a change applies at once. The
/// longest a passphrase stays is never shorter than how long it stays unused,
/// as on the desktop: what is stored is what the engine does.
void ApplyPassphraseCacheTimes() {
  auto settings = GetSettings();
  const auto ttl = SettingItems::kPasswordCacheTtl.Read(settings);
  auto max_ttl = SettingItems::kPasswordCacheMaxTtl.Read(settings);
  if (max_ttl < ttl) {
    max_ttl = ttl;
    SettingItems::kPasswordCacheMaxTtl.Write(settings, max_ttl);
  }
  SetRpgpPasswordCacheTtl(static_cast<uint64_t>(ttl),
                          static_cast<uint64_t>(max_ttl));
}

}  // namespace

auto SettingsModel::Offered() -> QList<Offer> {
  const auto passphrases = QStringLiteral("passphrases");
  const auto seconds = QStringLiteral("seconds");
  // How long a passphrase stays cached is a choice among sensible spans, not
  // any number of seconds.
  return {
      {&SettingItems::kPasswordCacheTtl,
       seconds,
       passphrases,
       {60, 300, 600, 1800, 3600},
       false},
      {&SettingItems::kPasswordCacheMaxTtl,
       seconds,
       passphrases,
       {600, 1800, 3600, 7200, 14400, 28800},
       false},
      {&SettingItems::kPassphrasePromptTimeout,
       seconds,
       passphrases,
       {30, 60, 120, 300}},
      {&SettingItems::kExpiringSoonDays,
       QStringLiteral("days"),
       QStringLiteral("keys"),
       {7, 14, 30, 60, 90}},
  };
}

SettingsModel::SettingsModel(QObject* parent) : QAbstractListModel(parent) {
  items_ = Offered();
}

auto SettingsModel::rowCount(const QModelIndex& parent) const -> int {
  return parent.isValid() ? 0 : static_cast<int>(items_.size());
}

auto SettingsModel::roleNames() const -> QHash<int, QByteArray> {
  return {
      {kKeyRole, "key"},
      {kTitleRole, "title"},
      {kDescriptionRole, "description"},
      {kTypeRole, "type"},
      {kValueRole, "value"},
      {kUnitRole, "unit"},
      {kSectionRole, "section"},
      {kChoicesRole, "choices"},
      {kCustomRole, "custom"},
  };
}

auto SettingsModel::data(const QModelIndex& index, int role) const
    -> QVariant {
  if (!index.isValid() || index.row() >= items_.size()) return {};
  const auto& offer = items_.at(index.row());
  const auto& item = offer.item;
  const auto* meta = Meta(item);

  switch (role) {
    case kKeyRole:
      return QString::fromLatin1(meta->key);
    case kTitleRole:
    case Qt::DisplayRole:
      return meta->Title();
    case kDescriptionRole:
      return meta->Description();
    case kTypeRole:
      return TypeName(item);
    case kValueRole: {
      const auto settings = GetSettings();
      return std::visit(
          [&settings](const auto* setting) {
            return QVariant::fromValue(setting->Read(settings));
          },
          item);
    }
    case kUnitRole:
      return offer.unit;
    case kSectionRole:
      return offer.section;
    case kChoicesRole:
      return offer.choices;
    case kCustomRole:
      return offer.custom;
    default:
      return {};
  }
}

bool SettingsModel::setValue(int row, const QVariant& value) {
  if (row < 0 || row >= items_.size()) return false;
  const auto& item = items_.at(row).item;
  const auto* meta = Meta(item);

  // Whatever is refused, the row says again what is in effect.
  const auto changed = index(row);

  bool stored = false;
  {
    auto settings = GetSettings();
    stored = std::visit(
        [&settings, &value](const auto* setting) {
          using T = decltype(setting->Default());
          if (!value.canConvert<T>()) return false;
          setting->Write(settings, value.value<T>());
          return true;
        },
        item);
  }  // written to disk here
  if (!stored) {
    emit dataChanged(changed, changed, {kValueRole});
    return false;
  }

  if (meta == &SettingItems::kPasswordCacheTtl ||
      meta == &SettingItems::kPasswordCacheMaxTtl) {
    ApplyPassphraseCacheTimes();
    // The other of the two may have moved with it.
    emit dataChanged(index(0), index(rowCount() - 1), {kValueRole});
    return true;
  }

  emit dataChanged(changed, changed, {kValueRole});
  return true;
}

void SettingsModel::forgetPassphrases() { ClearRpgpPasswordCache(); }

}  // namespace GpgFrontend::Mobile
