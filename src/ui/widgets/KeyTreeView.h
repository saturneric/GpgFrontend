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

#include "core/model/GpgKey.h"
#include "model/GpgKeyTreeProxyModel.h"

namespace GpgFrontend::UI {

/**
 * @brief
 *
 */
class KeyTreeView : public QTreeView {
  Q_OBJECT
 public:
  /**
   * @brief Construct a new Key Table object
   *
   * @param _key_list
   * @param _select_type
   * @param _info_type
   * @param _filter
   */
  explicit KeyTreeView(QWidget* parent = nullptr);

  /**
   * @brief Construct a new Key Table object
   *
   * @param _key_list
   * @param _select_type
   * @param _info_type
   * @param _filter
   */
  explicit KeyTreeView(int channel,
                       GpgKeyTreeModel::Detector checkable_detector,
                       GpgKeyTreeProxyModel::KeyFilter filter,
                       QWidget* parent = nullptr);

  /**
   * @brief Get the Key By Index object
   *
   * @param index
   * @return GpgAbstractKeyPtr
   */
  auto GetKeyByIndex(QModelIndex index) -> GpgAbstractKeyPtr;

  /**
   * @brief Get the All Checked Key Ids object
   *
   * @return KeyIdArgsList
   */
  auto GetAllCheckedKeyIds() -> KeyIdArgsList;

  /**
   * @brief Get the All Checked Sub Key object
   *
   * @return QContainer<GpgSubKey>
   */
  auto GetAllCheckedSubKey() -> QContainer<GpgSubKey>;

  /**
   * @brief Get the All Checked Keys object
   *
   * @return GpgAbstractKeyPtrList
   */
  auto GetAllCheckedKeys() -> GpgAbstractKeyPtrList;

  /**
   * @brief Set the Key Filter object
   *
   * @param filter
   */
  void SetKeyFilter(const GpgKeyTreeProxyModel::KeyFilter& filter);

  /**
   * @brief Supplies the root keys the tree is built from.
   */
  using KeyProvider = std::function<GpgAbstractKeyPtrList()>;

  /**
   * @brief Seed the tree from something other than the whole keyring.
   *
   * @param provider called on every rebuild; the default returns every key
   * known to the channel
   */
  void SetKeyProvider(KeyProvider provider);

  /**
   * @brief Choose how the provided keys are turned into a tree.
   *
   * @param mode build mode, see GpgKeyTreeBuildMode
   */
  void SetBuildMode(GpgKeyTreeBuildMode mode);

  /**
   * @brief Filter the visible rows by a search keyword.
   *
   * @param keywords keyword to match, empty clears the filter
   */
  void SetSearchKeywords(const QString& keywords);

  /**
   * @brief Keep a row whose descendant matches the search keyword.
   *
   * @param enabled whether descendants may keep their ancestors visible
   */
  void SetRecursiveFiltering(bool enabled);

  /**
   * @brief Draw an explanatory message when no row is visible.
   *
   * @param enabled whether the overlay is drawn
   */
  void SetEmptyStateEnabled(bool enabled);

  /**
   * @brief Override the message shown when the tree holds nothing at all.
   *
   * The shared default points the user at key generation, which is the wrong
   * advice when the tree shows the contents of something rather than a
   * keyring.
   *
   * @param when_empty message to draw, empty restores the default
   */
  void SetEmptyStateText(const QString& when_empty);

  /**
   * @brief Whether a double click opens the key details dialog.
   *
   * @param enabled false leaves double click to expand and collapse only
   */
  void SetOpenDetailsOnDoubleClick(bool enabled);

  /**
   * @brief
   *
   * @param channel
   */
  void SetChannel(int channel);

  /**
   * @brief
   *
   */
  void Refresh();

 signals:
  /**
   * @brief
   *
   * @param keys
   */
  void SignalKeysChecked(GpgAbstractKeyPtrList keys);

 protected:
  /**
   * @brief
   *
   */
  void paintEvent(QPaintEvent* event) override;

  /**
   * @brief Toggle the current row's check state on Space.
   *
   */
  void keyPressEvent(QKeyEvent* event) override;

 private:
  bool init_ = false;
  int channel_ = kGpgFrontendDefaultChannel;
  GpgKeyTreeModel::Detector checkable_detector_ = [](GpgAbstractKey*) -> bool {
    return false;
  };
  GpgKeyTreeProxyModel::KeyFilter key_filter_ =
      [](const GpgAbstractKey*) -> bool { return true; };
  GpgKeyTreeBuildMode build_mode_ = GpgKeyTreeBuildMode::kKEYS_AND_SUBKEYS;
  KeyProvider key_provider_;
  QString search_keywords_;
  bool empty_state_enabled_ = false;
  bool open_details_on_double_click_ = true;
  QString empty_state_text_;
  QSharedPointer<GpgKeyTreeModel> model_;
  GpgKeyTreeProxyModel proxy_model_;

  /**
   * @brief
   *
   */
  void slot_adjust_column_widths();

  /**
   * @brief
   *
   */
  void init();

  /**
   * @brief
   *
   */
  void init_view_style();

  /**
   * @brief
   *
   */
  void reset_model();
};

}  // namespace GpgFrontend::UI