/**
 * Copyright (C) 2021 Saturneric
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
 * Saturneric<eric@bktus.com> starting on May 12, 2021.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#ifndef __KEYLIST_H__
#define __KEYLIST_H__

#include <utility>

#include "core/GpgContext.h"
#include "ui/dialog/import_export/KeyImportDetailDialog.h"

class Ui_KeyList;

namespace GpgFrontend::UI {

/**
 * @brief
 *
 */
struct KeyListRow {
  using KeyType = unsigned int;

  static const KeyType SECRET_OR_PUBLIC_KEY = 0;  ///<
  static const KeyType ONLY_SECRET_KEY = 1;       ///<
};

/**
 * @brief
 *
 */
struct KeyListColumn {
  using InfoType = unsigned int;

  static constexpr InfoType ALL = ~0;               ///<
  static constexpr InfoType TYPE = 1 << 0;          ///<
  static constexpr InfoType NAME = 1 << 1;          ///<
  static constexpr InfoType EmailAddress = 1 << 2;  ///<
  static constexpr InfoType Usage = 1 << 3;         ///<
  static constexpr InfoType Validity = 1 << 4;      ///<
  static constexpr InfoType FingerPrint = 1 << 5;   ///<
};

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
};

/**
 * @brief
 *
 */
struct KeyTable {
  QTableWidget* key_list_;                     ///<
  KeyListRow::KeyType select_type_;            ///<
  KeyListColumn::InfoType info_type_;          ///<
  std::vector<GpgKey> buffered_keys_;          ///<
  std::function<bool(const GpgKey&)> filter_;  ///<
  KeyIdArgsListPtr checked_key_ids_;           ///<

  /**
   * @brief Construct a new Key Table object
   *
   * @param _key_list
   * @param _select_type
   * @param _info_type
   * @param _filter
   */
  KeyTable(
      QTableWidget* _key_list, KeyListRow::KeyType _select_type,
      KeyListColumn::InfoType _info_type,
      std::function<bool(const GpgKey&)> _filter = [](const GpgKey&) -> bool {
        return true;
      })
      : key_list_(_key_list),
        select_type_(_select_type),
        info_type_(_info_type),
        filter_(std::move(_filter)) {}

  /**
   * @brief
   *
   * @param m_keys
   */
  void Refresh(KeyLinkListPtr m_keys = nullptr);

  /**
   * @brief Get the Checked object
   *
   * @return KeyIdArgsListPtr&
   */
  KeyIdArgsListPtr& GetChecked();

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
   * @brief Set the Checked object
   *
   * @param key_ids
   */
  void SetChecked(KeyIdArgsListPtr key_ids);
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
      const QString& name,
      KeyListRow::KeyType selectType = KeyListRow::SECRET_OR_PUBLIC_KEY,
      KeyListColumn::InfoType infoType = KeyListColumn::ALL,
      const std::function<bool(const GpgKey&)>& filter =
          [](const GpgKey&) -> bool { return true; });

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
  KeyIdArgsListPtr GetChecked();

  /**
   * @brief Get the Checked object
   *
   * @param key_table
   * @return KeyIdArgsListPtr
   */
  static KeyIdArgsListPtr GetChecked(const KeyTable& key_table);

  /**
   * @brief Get the Private Checked object
   *
   * @return KeyIdArgsListPtr
   */
  KeyIdArgsListPtr GetPrivateChecked();

  /**
   * @brief Get the All Private Keys object
   *
   * @return KeyIdArgsListPtr
   */
  KeyIdArgsListPtr GetAllPrivateKeys();

  /**
   * @brief Set the Checked object
   *
   * @param key_ids
   */
  void SetChecked(KeyIdArgsListPtr key_ids);

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
  KeyIdArgsListPtr GetSelected();

  /**
   * @brief Get the Selected Key object
   *
   * @return std::string
   */
  std::string GetSelectedKey();

  /**
   * @brief
   *
   * @param keyIds
   */
  [[maybe_unused]] static void MarkKeys(QStringList* keyIds);

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[maybe_unused]] bool ContainsPrivateKeys();

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

  std::mutex buffered_key_list_mutex_;  ///<

  std::shared_ptr<Ui_KeyList> ui_;                                   ///<
  QTableWidget* m_key_list_{};                                       ///<
  std::vector<KeyTable> m_key_tables_;                               ///<
  QMenu* popup_menu_{};                                              ///<
  GpgFrontend::KeyLinkListPtr buffered_keys_list_;                   ///<
  std::function<void(const GpgKey&, QWidget*)> m_action_ = nullptr;  ///<
  KeyMenuAbility::AbilityType menu_ability_ = KeyMenuAbility::ALL;   ///<

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
  void slot_refresh_ui();

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

#endif  // __KEYLIST_H__
