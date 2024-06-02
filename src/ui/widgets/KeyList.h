/**
 * Copyright (C) 2021 Saturneric <eric@bktus.com>
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
#include "core/model/GpgKeyTableModel.h"
#include "core/model/GpgKeyTableProxyModel.h"

class Ui_KeyList;

namespace GpgFrontend::UI {

/**
 * @brief
 *
 */
struct KeyMenuAbility {
  using AbilityType = unsigned int;

  static constexpr AbilityType ALL = ~0;                  ///<
  static constexpr AbilityType NONE = 0;                  ///<
  static constexpr AbilityType REFRESH = 1 << 0;          ///<
  static constexpr AbilityType SYNC_PUBLIC_KEY = 1 << 1;  ///<
  static constexpr AbilityType UNCHECK_ALL = 1 << 3;      ///<
  static constexpr AbilityType CHECK_ALL = 1 << 5;        ///<
  static constexpr AbilityType SEARCH_BAR = 1 << 6;       ///<
};

/**
 * @brief
 *
 */
struct KeyTable : public QTableView {
  Q_OBJECT
 public:
  using KeyTableFilter = std::function<bool(const GpgKey&, const KeyTable&)>;

  /**
   * @brief Construct a new Key Table object
   *
   * @param _key_list
   * @param _select_type
   * @param _info_type
   * @param _filter
   */
  KeyTable(
      QWidget* parent, QSharedPointer<GpgKeyTableModel> model,
      GpgKeyTableDisplayMode _select_type, GpgKeyTableColumn _info_type,
      GpgKeyTableProxyModel::KeyFilter _filter = [](const GpgKey&) -> bool {
        return true;
      });

  /**
   * @brief
   *
   * @param model
   */
  void RefreshModel(QSharedPointer<GpgKeyTableModel> model);

  /**
   * @brief Get the Checked object
   *
   * @return KeyIdArgsListPtr&
   */
  auto GetChecked() -> KeyIdArgsListPtr;

  /**
   * @brief
   *
   */
  void UncheckALL() const;

  /**
   * @brief
   *
   */
  void CheckALL() const;

  /**
   * @brief
   *
   */
  void SetMenuAbility(KeyMenuAbility::AbilityType ability);

  /**
   * @brief
   *
   */
  void SetFilterKeyword(const QString& keyword);

  /**
   * @brief
   *
   * @param row
   * @return true
   * @return false
   */
  [[nodiscard]] auto IsRowChecked(int row) const -> bool;

  /**
   * @brief Set the Row Checked object
   *
   * @param row
   */
  void SetRowChecked(int row) const;

  /**
   * @brief Set the Row Checked object
   *
   * @param row
   */
  [[nodiscard]] auto GetRowSelected() const -> int;

  /**
   * @brief Get the Row Count object
   *
   * @return auto
   */
  [[nodiscard]] auto GetRowCount() const -> int;

  /**
   * @brief Get the Key Id By Row object
   *
   * @param row
   * @return QString
   */
  [[nodiscard]] auto GetKeyIdByRow(int row) const -> QString;

  /**
   * @brief
   *
   * @param row
   * @return true
   * @return false
   */
  [[nodiscard]] auto IsPublicKeyByRow(int row) const -> bool;

  /**
   * @brief
   *
   * @param row
   * @return true
   * @return false
   */
  [[nodiscard]] auto IsPrivateKeyByRow(int row) const -> bool;

  /**
   * @brief
   *
   */
  void CheckAll();

  /**
   * @brief
   *
   */
  void UncheckAll();

 private:
  KeyMenuAbility::AbilityType ability_;  ///<

  QSharedPointer<GpgKeyTableModel> model_;
  GpgKeyTableProxyModel proxy_model_;
};

/**
 * @brief
 *
 */
class KeyList : public QWidget {
  Q_OBJECT

 public:
  /**
   * @brief Construct a new Key List object
   *
   * @param menu_ability
   * @param parent
   */
  explicit KeyList(KeyMenuAbility::AbilityType menu_ability,
                   QWidget* parent = nullptr);

  /**
   * @brief
   *
   * @param name
   * @param selectType
   * @param infoType
   * @param filter
   */
  void AddListGroupTab(
      const QString& name, const QString& id,
      GpgKeyTableDisplayMode selectType = GpgKeyTableDisplayMode::kPrivateKey,
      GpgKeyTableColumn infoType = GpgKeyTableColumn::kAll,
      GpgKeyTableProxyModel::KeyFilter filter = [](const GpgKey&) -> bool {
        return true;
      });

  /**
   * @brief Set the Double Clicked Action object
   *
   * @param action
   */
  void SetDoubleClickedAction(
      std::function<void(const GpgKey&, QWidget*)> action);

  /**
   * @brief Set the Column Width object
   *
   * @param row
   * @param size
   */
  void SetColumnWidth(int row, int size);

  /**
   * @brief
   *
   * @param act
   */
  void AddMenuAction(QAction* act);

  /**
   * @brief
   *
   */
  void AddSeparator();

  /**
   * @brief Get the Checked object
   *
   * @return KeyIdArgsListPtr
   */
  auto GetChecked() -> KeyIdArgsListPtr;

  /**
   * @brief Get the Checked object
   *
   * @param key_table
   * @return KeyIdArgsListPtr
   */
  static auto GetChecked(const KeyTable& key_table) -> KeyIdArgsListPtr;

  /**
   * @brief Get the Private Checked object
   *
   * @return KeyIdArgsListPtr
   */
  auto GetCheckedPrivateKey() -> KeyIdArgsListPtr;

  /**
   * @brief
   *
   * @return KeyIdArgsListPtr
   */
  auto GetCheckedPublicKey() -> KeyIdArgsListPtr;

  /**
   * @brief Get the All Private Keys object
   *
   * @return KeyIdArgsListPtr
   */
  auto GetAllPrivateKeys() -> KeyIdArgsListPtr;

  /**
   * @brief Set the Checked object
   *
   * @param keyIds
   * @param key_table
   */
  static void SetChecked(const KeyIdArgsListPtr& keyIds,
                         const KeyTable& key_table);

  /**
   * @brief Get the Selected object
   *
   * @return KeyIdArgsListPtr
   */
  auto GetSelected() -> KeyIdArgsListPtr;

  /**
   * @brief Get the Selected Key object
   *
   * @return QString
   */
  auto GetSelectedKey() -> QString;

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[maybe_unused]] auto ContainsPrivateKeys() -> bool;

 signals:
  /**
   * @brief
   *
   * @param message
   * @param timeout
   */
  void SignalRefreshStatusBar(const QString& message, int timeout);

  /**
   * @brief
   *
   */
  void SignalRefreshDatabase();

 public slots:

  /**
   * @brief
   *
   */
  void SlotRefresh();

  /**
   * @brief
   *
   */
  void SlotRefreshUI();

 private:
  /**
   * @brief
   *
   */
  void init();

  /**
   * @brief
   *
   * @param inBuffer
   */
  void import_keys(const QByteArray& inBuffer);

  /**
   * @brief
   *
   */
  void uncheck_all();

  /**
   * @brief
   *
   */
  void check_all();

  /**
   * @brief
   *
   */
  void filter_by_keyword();

  std::shared_ptr<Ui_KeyList> ui_;                                   ///<
  QMenu* popup_menu_{};                                              ///<
  std::function<void(const GpgKey&, QWidget*)> m_action_ = nullptr;  ///<
  KeyMenuAbility::AbilityType menu_ability_ = KeyMenuAbility::ALL;   ///<
  QSharedPointer<GpgKeyTableModel> model_;

 private slots:

  /**
   * @brief
   *
   * @param index
   */
  void slot_double_clicked(const QModelIndex& index);

  /**
   * @brief
   *
   */
  void slot_sync_with_key_server();

 protected:
  /**
   * @brief
   *
   * @param event
   */
  void contextMenuEvent(QContextMenuEvent* event) override;

  /**
   * @brief
   *
   * @param event
   */
  void dragEnterEvent(QDragEnterEvent* event) override;

  /**
   * @brief
   *
   * @param event
   */
  void dropEvent(QDropEvent* event) override;
};

}  // namespace GpgFrontend::UI
