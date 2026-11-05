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

#include <QAbstractListModel>
#include <QtQml/qqmlregistration.h>
#include <variant>

#include "core/function/SettingItem.h"

namespace GpgFrontend::Mobile {

/**
 * @brief The settings a phone offers, as a list QML can edit.
 *
 * Rows come from the core's setting catalog, limited to an allow-list: a
 * setting is offered here only once it means something on a phone. A setting
 * this build pins is never offered. Each row reads and writes through its
 * catalog item, so the default, the type and the pin are the catalog's.
 *
 * Roles: key, title, description, type ("bool", "int", "string"), value,
 * unit ("seconds", "days" or empty), section (which group the page shows it
 * in), choices (values worth offering first, in order) and custom (whether a
 * value other than the choices may be set).
 *
 * Two actions that are not settings live here too: forgetting cached
 * passphrases, and clearing the files shared with other applications.
 */
class SettingsModel : public QAbstractListModel {
  Q_OBJECT
  QML_ELEMENT

  Q_PROPERTY(
      int sharedFileCount READ SharedFileCount NOTIFY SignalSharedFilesChanged)
  Q_PROPERTY(
      qint64 sharedFileSize READ SharedFileSize NOTIFY SignalSharedFilesChanged)
  /// The operations the menus offer besides Encrypt, Decrypt, Sign and
  /// Verify, as turned on here: "encrypt_sign", "decrypt_verify",
  /// "symmetric". None until asked for, as on the desktop's toolbar. The
  /// same for every page, whichever model changed it.
  Q_PROPERTY(QStringList extraOperations READ ExtraOperations NOTIFY
                 SignalOperationsChanged)
  /// What the Encrypt and Decrypt buttons do, as the desktop's toolbar
  /// defaults: "encrypt", "encrypt_sign" or "symmetric"; "decrypt" or
  /// "decrypt_verify". Only ever one that is offered.
  Q_PROPERTY(
      QString defaultEncrypt READ DefaultEncrypt NOTIFY SignalOperationsChanged)
  Q_PROPERTY(
      QString defaultDecrypt READ DefaultDecrypt NOTIFY SignalOperationsChanged)
  /// The languages there is a catalog for: [{code, name}], the system's
  /// own first (code ""), then by name, as the desktop offers them.
  Q_PROPERTY(QVariantList languages READ Languages NOTIFY SignalLanguageChanged)
  /// The language chosen ("de_DE"; "" for the system's), the desktop's
  /// setting. Set, it applies at once: every page is translated again.
  Q_PROPERTY(QString language READ Language WRITE SetLanguage NOTIFY
                 SignalLanguageChanged)

 public:
  using Item = std::variant<const BoolSetting*, const IntSetting*,
                            const Int64Setting*, const StringSetting*>;

  /// A setting on the allow-list, with how a phone presents it.
  struct Offer {
    Item item;
    QString unit;
    QString section;
    QVariantList choices;
    bool custom = true;
  };

  enum Role : int {
    kKeyRole = Qt::UserRole + 1,
    kTitleRole,
    kDescriptionRole,
    kTypeRole,
    kValueRole,
    kUnitRole,
    kSectionRole,
    kChoicesRole,
    kCustomRole,
  };

  explicit SettingsModel(QObject* parent = nullptr);

  /// The settings a person may change here.
  static auto Offered() -> QList<Offer>;

  [[nodiscard]] auto rowCount(const QModelIndex& parent = {}) const
      -> int override;
  [[nodiscard]] auto data(const QModelIndex& index, int role) const
      -> QVariant override;
  [[nodiscard]] auto roleNames() const -> QHash<int, QByteArray> override;

  /// Store @p value for the row @p row; false when it is not that type.
  Q_INVOKABLE bool setValue(int row, const QVariant& value);

  /// Forget every cached passphrase now.
  Q_INVOKABLE void forgetPassphrases();

  /// Revoke and delete every file shared with another application.
  Q_INVOKABLE void clearSharedFiles();

  [[nodiscard]] auto SharedFileCount() const -> int;

  /// How many bytes the files shared with other applications take.
  [[nodiscard]] auto SharedFileSize() const -> qint64;

  [[nodiscard]] static auto ExtraOperations() -> QStringList;
  [[nodiscard]] static auto DefaultEncrypt() -> QString;
  [[nodiscard]] static auto DefaultDecrypt() -> QString;

  [[nodiscard]] auto Languages() const -> QVariantList;
  [[nodiscard]] static auto Language() -> QString;
  void SetLanguage(const QString& code);

 signals:
  void SignalSharedFilesChanged();
  void SignalOperationsChanged();
  void SignalLanguageChanged();

 private:
  QList<Offer> items_;
};

}  // namespace GpgFrontend::Mobile
