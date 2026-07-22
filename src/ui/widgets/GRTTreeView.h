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

namespace GpgFrontend::Module {
class GlobalRegisterTableTreeModel;
}

namespace GpgFrontend::UI {

class GRTTreeView : public QTreeView {
  Q_OBJECT
 public:
  /**
   * @brief Construct a new GRTTreeView object
   *
   * @param parent
   */
  explicit GRTTreeView(QWidget* parent);

  /**
   * @brief Destroy the GRTTreeView object
   *
   */
  virtual ~GRTTreeView() override;

  /**
   * @brief Apply a case-insensitive filter over all columns.
   *
   * Parent rows are kept when any descendant matches.
   */
  void SetFilter(const QString& text);

  /**
   * @brief Expand every node of the tree.
   */
  void ExpandAll();

  /**
   * @brief Collapse every node of the tree.
   */
  void CollapseAll();

  /**
   * @brief Rebuild the model snapshot, keeping expansion and selection.
   */
  void Refresh();

 protected:
  /**
   * @brief
   *
   * @param event
   */
  void paintEvent(QPaintEvent* event) override;

 private slots:

  /**
   * @brief
   *
   */
  void slot_adjust_column_widths();

  /**
   * @brief Show the copy / expand context menu at the given position.
   */
  void slot_show_context_menu(const QPoint& pos);

 private:
  bool initial_resize_done_ = false;
  Module::GlobalRegisterTableTreeModel* model_;
  QSortFilterProxyModel* proxy_model_;
  QSet<QString> expanded_paths_cache_;

  /**
   * @brief Collect the key paths of all currently expanded rows.
   */
  [[nodiscard]] auto save_expanded_paths() const -> QSet<QString>;

  /**
   * @brief Re-expand the rows whose key path is in @p paths.
   */
  void restore_expanded_paths(const QSet<QString>& paths);
};

}  // namespace GpgFrontend::UI