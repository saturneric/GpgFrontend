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

#include "ui/widgets/KeyTable.h"

class Ui_KeyList;

namespace GpgFrontend::UI {

/**
 * @brief
 *
 */
enum class KeyMenuAbility : unsigned int {
  kNONE = 0,
  kREFRESH = 1 << 0,
  kSYNC_PUBLIC_KEY = 1 << 1,
  kUNCHECK_ALL = 1 << 2,
  kCHECK_ALL = 1 << 3,
  kCOLUMN_FILTER = 1 << 4,
  kSEARCH_BAR = 1 << 5,
  kKEY_DATABASE = 1 << 6,
  kKEY_GROUP = 1 << 7,

  kALL = ~0U
};

inline auto operator|(KeyMenuAbility lhs,
                      KeyMenuAbility rhs) -> KeyMenuAbility {
  using T = std::underlying_type_t<KeyMenuAbility>;
  return static_cast<KeyMenuAbility>(static_cast<T>(lhs) | static_cast<T>(rhs));
}

inline auto operator|=(KeyMenuAbility& lhs,
                       KeyMenuAbility rhs) -> KeyMenuAbility& {
  lhs = lhs | rhs;
  return lhs;
}

inline auto operator&(KeyMenuAbility lhs, KeyMenuAbility rhs) -> bool {
  using T = std::underlying_type_t<KeyMenuAbility>;
  return (static_cast<T>(lhs) & static_cast<T>(rhs)) != 0;
}

inline auto operator~(KeyMenuAbility hs) -> KeyMenuAbility {
  using T = std::underlying_type_t<GpgKeyTableColumn>;
  return static_cast<KeyMenuAbility>(~static_cast<T>(hs));
}

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
   */
  explicit KeyList(QWidget* parent = nullptr);

  /**
   * @brief Construct a new Key List object
   *
   * @param menu_ability
   * @param parent
   */
  explicit KeyList(
      int channel, KeyMenuAbility menu_ability,
      GpgKeyTableColumn fixed_column_filter = GpgKeyTableColumn::kALL,
      QWidget* parent = nullptr);

  /**
   * @brief
   *
   * @param channel
   * @param menu_ability
   * @param fixed_column_filter
   */
  void Init(int channel, KeyMenuAbility menu_ability,
            GpgKeyTableColumn fixed_column_filter = GpgKeyTableColumn::kALL);

  /**
   * @brief
   *
   * @param name
   * @param selectType
   * @param infoType
   * @param filter
   */
  auto AddListGroupTab(
      const QString& name, const QString& id,
      GpgKeyTableDisplayMode display_mode =
          GpgKeyTableDisplayMode::kPRIVATE_KEY,
      GpgKeyTableProxyModel::KeyFilter search_filter =
          [](const GpgAbstractKey*) -> bool { return true; },
      GpgKeyTableColumn custom_columns_filter = GpgKeyTableColumn::kALL)
      -> KeyTable*;

  /**
   * @brief Set the Column Width object
   *
   * @param row
   * @param size
   */
  void SetColumnWidth(int row, int size);

  /**
   * @brief Get the Checked Keys object
   *
   * @return QStringList
   */
  auto GetCheckedKeys() -> GpgAbstractKeyPtrList;

  /**
   * @brief Get the Checked object
   *
   * @param key_table
   * @return KeyIdArgsListPtr
   */
  static auto GetChecked(const KeyTable& key_table) -> GpgAbstractKeyPtrList;

  /**
   * @brief Get the Private Checked object
   *
   * @return KeyIdArgsListPtr
   */
  auto GetCheckedPrivateKey() -> GpgAbstractKeyPtrList;

  /**
   * @brief
   *
   * @return KeyIdArgsListPtr
   */
  auto GetCheckedPublicKey() -> GpgAbstractKeyPtrList;

  /**
   * @brief Set the Checked object
   *
   * @param keyIds
   * @param key_table
   */
  static void SetChecked(const KeyIdArgsList& key_ids,
                         const KeyTable& key_table);

  /**
   * @brief Get the Selected Key object
   *
   * @return QString
   */
  auto GetSelectedKey() -> GpgAbstractKeyPtr;

  /**
   * @brief Get the Selected Keys object
   *
   * @return GpgAbstractKeyPtrList
   */
  auto GetSelectedKeys() -> GpgAbstractKeyPtrList;

  /**
   * @brief Get the Selected Key object
   *
   * @return QString
   */
  auto GetSelectedGpgKey() -> GpgKeyPtr;

  /**
   * @brief Get the Selected Keys object
   *
   * @return GpgAbstractKeyPtrList
   */
  auto GetSelectedGpgKeys() -> GpgKeyPtrList;

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[maybe_unused]] auto ContainsPrivateKeys() -> bool;

  /**
   * @brief
   *
   */
  void UpdateKeyTableColumnType(GpgKeyTableColumn);

  /**
   * @brief
   *
   * @return int
   */
  [[nodiscard]] auto GetCurrentGpgContextChannel() const -> int;

  /**
   * @brief
   *
   */
  void UpdateKeyTableFilter(int index, const GpgKeyTableProxyModel::KeyFilter&);

  /**
   * @brief
   *
   * @param index
   */
  void RefreshKeyTable(int index);

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

  /**
   * @brief
   *
   */
  void SignalColumnTypeChange(GpgKeyTableColumn);

  /**
   * @brief
   *
   */
  void SignalKeyChecked();

  /**
   * @brief
   *
   */
  void SignalRequestContextMenu(QContextMenuEvent* event, KeyTable*);

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

 private slots:

  /**
   * @brief
   *
   */
  void slot_sync_with_key_server();

  /**
   * @brief
   *
   */
  void slot_new_key_group();

 private:
  std::shared_ptr<Ui_KeyList> ui_;                                   ///<
  std::function<void(const GpgKey&, QWidget*)> m_action_ = nullptr;  ///<
  int current_gpg_context_channel_;
  KeyMenuAbility menu_ability_ = KeyMenuAbility::kALL;  ///<
  QSharedPointer<GpgKeyTableModel> model_;
  GpgKeyTableColumn fixed_columns_filter_;
  GpgKeyTableColumn global_column_filter_;

  QAction* key_id_column_action_;
  QAction* algo_column_action_;
  QAction* create_date_column_action_;
  QAction* owner_trust_column_action_;
  QAction* subkeys_number_column_action_;
  QAction* comment_column_action_;

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
  void import_keys(const QByteArray& in_buffer);

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
};

}  // namespace GpgFrontend::UI
