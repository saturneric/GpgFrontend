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

 private:
  QList<Offer> items_;
};

}  // namespace GpgFrontend::Mobile
