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
   * @param name
   * @param selectType
   * @param infoType
   * @param filter
   */
  void AddListGroupTab(
      const QString& name, const QString& id,
      GpgKeyTableDisplayMode display_mode =
          GpgKeyTableDisplayMode::kPRIVATE_KEY,
      GpgKeyTableProxyModel::KeyFilter search_filter =
          [](const GpgKey&) -> bool { return true; },
      GpgKeyTableColumn custom_columns_filter = GpgKeyTableColumn::kALL);

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
   * @brief Get the Checked Keys object
   *
   * @return QStringList
   */
  auto GetCheckedKeys() -> QStringList;

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
  static void SetChecked(const KeyIdArgsListPtr& key_ids,
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

  std::shared_ptr<Ui_KeyList> ui_;                                   ///<
  QMenu* popup_menu_{};                                              ///<
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
