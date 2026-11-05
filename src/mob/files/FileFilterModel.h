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
#include <QVariantList>
#include <functional>

namespace GpgFrontend::Mobile {

/**
 * @brief The files the Files page shows: searched by name, in the
 * workspace's order. Over a FileWorkspace.
 */
class FileFilterModel : public QSortFilterProxyModel {
  Q_OBJECT
  QML_ELEMENT

  /// The FileWorkspace filtered.
  Q_PROPERTY(QAbstractItemModel* sourceModel READ sourceModel WRITE
                 setSourceModel NOTIFY SignalSourceModelChanged)
  /// Matched, ignoring case, against the file's name.
  Q_PROPERTY(QString searchText READ SearchText WRITE SetSearchText NOTIFY
                 SignalFilterChanged)
  Q_PROPERTY(int count READ rowCount NOTIFY SignalCountChanged)

 public:
  explicit FileFilterModel(QObject* parent = nullptr);

  void setSourceModel(QAbstractItemModel* source) override;

  [[nodiscard]] auto SearchText() const -> QString;
  void SetSearchText(const QString& text);
  [[nodiscard]] auto Filtering() const -> bool;

  [[nodiscard]] Q_INVOKABLE int rowCount(
      const QModelIndex& parent = {}) const override;

  /// The row of the file @p id here; -1 when it is left out or not listed.
  [[nodiscard]] Q_INVOKABLE int rowOf(int id) const;

  /// The ids of the files shown, in order.
  [[nodiscard]] Q_INVOKABLE QVariantList ids() const;

  /// Show every file again.
  Q_INVOKABLE void reset();

 signals:
  void SignalSourceModelChanged();
  void SignalFilterChanged();
  void SignalCountChanged();

 protected:
  [[nodiscard]] auto filterAcceptsRow(int source_row,
                                      const QModelIndex& source_parent) const
      -> bool override;

 private:
  QString search_text_;

  /// Make @p change to what is filtered, and filter again.
  void refilter(const std::function<void()>& change);
};

}  // namespace GpgFrontend::Mobile
