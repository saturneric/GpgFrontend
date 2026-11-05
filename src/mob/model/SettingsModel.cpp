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

#include <QQmlEngine>

#include "core/function/GlobalSettingStation.h"
#include "core/function/SettingCatalog.h"
#include "core/function/rpgp/PasswordFetcher.h"
#include "core/profile/ProfileSession.h"
#include "core/utils/RustUtils.h"
#include "files/FileOperations.h"
#include "files/ShareRetentionStore.h"
#include "model/ChangeRelay.h"
#include "shell/present/TranslationUtils.h"

namespace GpgFrontend::Mobile {

namespace {

/// Tells every model that the operations offered changed.
auto OperationsRelay() -> ChangeRelay& {
  static ChangeRelay relay;
  return relay;
}

/// What a button may do: its plain operation, and the others of its kind
/// that are turned on.
auto EncryptChoices() -> QStringList {
  QStringList choices{QStringLiteral("encrypt")};
  for (const auto& op : SettingsModel::ExtraOperations()) {
    if (op == QLatin1String("encrypt_sign") ||
        op == QLatin1String("symmetric")) {
      choices << op;
    }
  }
  return choices;
}

auto DecryptChoices() -> QStringList {
  QStringList choices{QStringLiteral("decrypt")};
  if (SettingsModel::ExtraOperations().contains(
          QLatin1String("decrypt_verify"))) {
    choices << QStringLiteral("decrypt_verify");
  }
  return choices;
}

auto IsOperationSetting(const SettingMeta* meta) -> bool {
  return meta == &SettingItems::kShowEncryptSign ||
         meta == &SettingItems::kShowDecryptVerify ||
         meta == &SettingItems::kShowSymmetricEncrypt ||
         meta == &SettingItems::kDefaultEncryptOperation ||
         meta == &SettingItems::kDefaultDecryptOperation;
}

auto Meta(const SettingsModel::Item& item) -> const SettingMeta* {
  return std::visit(
      [](const auto* setting) -> const SettingMeta* { return setting; }, item);
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
      // What the operation menus offer besides the four, as the desktop's
      // toolbar settings.
      {&SettingItems::kShowEncryptSign, {}, QStringLiteral("operations"), {}},
      {&SettingItems::kShowDecryptVerify, {}, QStringLiteral("operations"), {}},
      {&SettingItems::kShowSymmetricEncrypt,
       {},
       QStringLiteral("operations"),
       {}},
      // What the buttons do, among what is turned on (data() says which).
      {&SettingItems::kDefaultEncryptOperation,
       QStringLiteral("operation"),
       QStringLiteral("operations"),
       {},
       false},
      {&SettingItems::kDefaultDecryptOperation,
       QStringLiteral("operation"),
       QStringLiteral("operations"),
       {},
       false},
  };
}

SettingsModel::SettingsModel(QObject* parent) : QAbstractListModel(parent) {
  items_ = Offered();
  // What is offered changed: so may the choices and values of the rows.
  connect(&OperationsRelay(), &ChangeRelay::SignalChanged, this, [this]() {
    emit dataChanged(index(0), index(rowCount() - 1),
                     {kValueRole, kChoicesRole});
    emit SignalOperationsChanged();
  });
}

auto SettingsModel::Languages() const -> QVariantList {
  QVariantList list{
      QVariantMap{{QStringLiteral("code"), QString()},
                  {QStringLiteral("name"), tr("System default")}}};
  for (const auto& [code, name] : AvailableUILanguages()) {
    list.append(QVariantMap{{QStringLiteral("code"), code},
                            {QStringLiteral("name"), name}});
  }
  return list;
}

auto SettingsModel::Language() -> QString {
  if (!ProfileSession::Loaded()) return {};
  return SettingItems::kLanguage.Read(GetSettings());
}

void SettingsModel::SetLanguage(const QString& code) {
  if (!ProfileSession::Loaded() || code == Language()) return;
  {
    auto settings = GetSettings();
    SettingItems::kLanguage.Write(settings, code);
  }
  // As at start (InitLocale(), InstallTranslations()), but now: a phone
  // application is not started again to change its language.
  const auto locale =
      code.trimmed().isEmpty() ? QLocale::system() : QLocale(code);
  QLocale::setDefault(locale);
  InstallTranslations(locale);
  if (auto* engine = qmlEngine(this); engine != nullptr) engine->retranslate();
  emit SignalLanguageChanged();
}

auto SettingsModel::DefaultEncrypt() -> QString {
  if (!ProfileSession::Loaded()) return QStringLiteral("encrypt");
  const auto chosen =
      SettingItems::kDefaultEncryptOperation.Read(GetSettings());
  return EncryptChoices().contains(chosen) ? chosen : QStringLiteral("encrypt");
}

auto SettingsModel::DefaultDecrypt() -> QString {
  if (!ProfileSession::Loaded()) return QStringLiteral("decrypt");
  const auto chosen =
      SettingItems::kDefaultDecryptOperation.Read(GetSettings());
  return DecryptChoices().contains(chosen) ? chosen : QStringLiteral("decrypt");
}

auto SettingsModel::ExtraOperations() -> QStringList {
  // Settings live in the open profile: none before, so none offered.
  if (!ProfileSession::Loaded()) return {};
  auto settings = GetSettings();
  QStringList ops;
  if (SettingItems::kShowEncryptSign.Read(settings)) {
    ops << QStringLiteral("encrypt_sign");
  }
  if (SettingItems::kShowDecryptVerify.Read(settings)) {
    ops << QStringLiteral("decrypt_verify");
  }
  if (SettingItems::kShowSymmetricEncrypt.Read(settings)) {
    ops << QStringLiteral("symmetric");
  }
  return ops;
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

auto SettingsModel::data(const QModelIndex& index, int role) const -> QVariant {
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
      // A button's operation: the one in effect.
      if (meta == &SettingItems::kDefaultEncryptOperation) {
        return DefaultEncrypt();
      }
      if (meta == &SettingItems::kDefaultDecryptOperation) {
        return DefaultDecrypt();
      }
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
      if (meta == &SettingItems::kDefaultEncryptOperation) {
        return QVariant(EncryptChoices());
      }
      if (meta == &SettingItems::kDefaultDecryptOperation) {
        return QVariant(DecryptChoices());
      }
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

  // A button does only what is offered.
  if ((meta == &SettingItems::kDefaultEncryptOperation &&
       !EncryptChoices().contains(value.toString())) ||
      (meta == &SettingItems::kDefaultDecryptOperation &&
       !DecryptChoices().contains(value.toString()))) {
    emit dataChanged(changed, changed, {kValueRole});
    return false;
  }

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
  if (IsOperationSetting(meta)) emit OperationsRelay().SignalChanged();
  return true;
}

void SettingsModel::forgetPassphrases() { ClearRpgpPasswordCache(); }

void SettingsModel::clearSharedFiles() {
  if (auto* store = SharedFileStore(); store != nullptr) store->Clear();
  emit SignalSharedFilesChanged();
}

auto SettingsModel::SharedFileCount() const -> int {
  const auto* store = SharedFileStore();
  return store != nullptr ? static_cast<int>(store->Entries().size()) : 0;
}

auto SettingsModel::SharedFileSize() const -> qint64 {
  const auto* store = SharedFileStore();
  if (store == nullptr) return 0;
  qint64 total = 0;
  for (const auto& entry : store->Entries()) total += entry.size;
  return total;
}

}  // namespace GpgFrontend::Mobile
