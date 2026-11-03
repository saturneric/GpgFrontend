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

#include <QtQml/qqmlregistration.h>

#include <QSortFilterProxyModel>

namespace GpgFrontend::Mobile {

/**
 * @brief The keys a page shows: searched, narrowed, sorted by name.
 *
 * Over a KeyRoleModel. Key groups are left out; the mobile application has
 * none.
 */
class KeyFilterModel : public QSortFilterProxyModel {
  Q_OBJECT
  QML_ELEMENT

  /// The KeyRoleModel filtered. QAbstractProxyModel has no such property for
  /// QML to set.
  Q_PROPERTY(QAbstractItemModel* sourceModel READ sourceModel WRITE
                 setSourceModel NOTIFY SignalSourceModelChanged)
  /// Matched, ignoring case, against name, email, key id and fingerprint.
  Q_PROPERTY(QString searchText READ SearchText WRITE SetSearchText NOTIFY
                 SignalFilterChanged)
  /// Only keys with their secret part here.
  Q_PROPERTY(bool onlySecret READ OnlySecret WRITE SetOnlySecret NOTIFY
                 SignalFilterChanged)
  /// Only keys a message can be encrypted to, now.
  Q_PROPERTY(bool onlyEncryptCapable READ OnlyEncryptCapable WRITE
                 SetOnlyEncryptCapable NOTIFY SignalFilterChanged)
  /// Only keys that can sign, now.
  Q_PROPERTY(bool onlySignCapable READ OnlySignCapable WRITE SetOnlySignCapable
                 NOTIFY SignalFilterChanged)
  Q_PROPERTY(int count READ rowCount NOTIFY SignalCountChanged)

 public:
  explicit KeyFilterModel(QObject* parent = nullptr);

  void setSourceModel(QAbstractItemModel* source) override;

  [[nodiscard]] auto SearchText() const -> QString;
  void SetSearchText(const QString& text);
  [[nodiscard]] auto OnlySecret() const -> bool;
  void SetOnlySecret(bool only);
  [[nodiscard]] auto OnlyEncryptCapable() const -> bool;
  void SetOnlyEncryptCapable(bool only);
  [[nodiscard]] auto OnlySignCapable() const -> bool;
  void SetOnlySignCapable(bool only);

  [[nodiscard]] Q_INVOKABLE int rowCount(
      const QModelIndex& parent = {}) const override;

 signals:
  void SignalSourceModelChanged();
  void SignalFilterChanged();
  void SignalCountChanged();

 protected:
  [[nodiscard]] auto filterAcceptsRow(int source_row,
                                      const QModelIndex& source_parent) const
      -> bool override;
  [[nodiscard]] auto lessThan(const QModelIndex& left,
                              const QModelIndex& right) const -> bool override;

 private:
  QString search_text_;
  bool only_secret_ = false;
  bool only_encrypt_capable_ = false;
  bool only_sign_capable_ = false;

  void refilter();
};

}  // namespace GpgFrontend::Mobile
