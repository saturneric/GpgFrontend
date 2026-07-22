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

#include "core/module/Module.h"

namespace GpgFrontend::UI {

/**
 * @brief Roles carried by every item of the module list model.
 */
enum ModuleItemRole {
  kModuleIdRole = Qt::UserRole + 1,  ///< module identifier
  kModuleNameRole,                   ///< display name from metadata
  kModuleDescriptionRole,            ///< description from metadata
  kModuleAuthorRole,                 ///< author from metadata
  kModuleVersionRole,                ///< module version
  kModuleIntegratedRole,             ///< true if built-in
  kModuleActiveRole,                 ///< true if currently activated
  kModuleAutoActivateRole,           ///< true if activated on start
  kModuleSearchTextRole,             ///< joined text used by the search filter
};

/**
 * @brief Category quick-filter applied on top of the text filter.
 */
enum class ModuleCategory {
  kAll,
  kActive,
  kInactive,
  kIntegrated,
  kExternal,
};

/**
 * @brief Item delegate painting a module as a small card.
 *
 * Line 1: status dot, name, version. Line 2: description. Line 3: chips.
 */
class ModuleItemDelegate : public QStyledItemDelegate {
  Q_OBJECT
 public:
  explicit ModuleItemDelegate(QObject* parent = nullptr);

  void paint(QPainter* painter, const QStyleOptionViewItem& option,
             const QModelIndex& index) const override;

  [[nodiscard]] auto sizeHint(const QStyleOptionViewItem& option,
                              const QModelIndex& index) const -> QSize override;
};

/**
 * @brief Filter proxy combining a free-text search and a category filter.
 */
class ModuleListProxyModel : public QSortFilterProxyModel {
  Q_OBJECT
 public:
  explicit ModuleListProxyModel(QObject* parent = nullptr);

  void SetCategory(ModuleCategory category);

 protected:
  [[nodiscard]] auto filterAcceptsRow(int source_row,
                                      const QModelIndex& source_parent) const
      -> bool override;

  /**
   * @brief Order: active first, then integrated, then by name.
   */
  [[nodiscard]] auto lessThan(const QModelIndex& left,
                              const QModelIndex& right) const -> bool override;

 private:
  ModuleCategory category_ = ModuleCategory::kAll;
};

class ModuleListView : public QListView {
  Q_OBJECT
 public:
  explicit ModuleListView(QWidget* parent);

  auto GetCurrentModuleID() -> Module::ModuleIdentifier;

  /**
   * @brief Reload all module information, restoring the current selection.
   */
  void Refresh();

  /**
   * @brief Apply a case-insensitive free-text filter.
   */
  void SetSearchFilter(const QString& text);

  /**
   * @brief Apply a category quick-filter.
   */
  void SetCategoryFilter(ModuleCategory category);

  /**
   * @brief Select the given module if it is currently visible.
   */
  void SelectModule(const Module::ModuleIdentifier& module_id);

  [[nodiscard]] auto ModuleCount() const -> int;

  [[nodiscard]] auto ActiveModuleCount() const -> int;

 signals:
  void SignalSelectModule(Module::ModuleIdentifier);

  /**
   * @brief Emitted after a reload with the total and active module counts.
   */
  void SignalCountsChanged(int total, int active);

 protected:
  void currentChanged(const QModelIndex& current,
                      const QModelIndex& previous) override;

 private:
  QStandardItemModel* model_;
  ModuleListProxyModel* proxy_model_;

  void init_view_style();
  void load_module_information();
};
};  // namespace GpgFrontend::UI
