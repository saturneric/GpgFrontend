/**
 * This file is part of GpgFrontend.
 *
 * GpgFrontend is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Foobar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Foobar.  If not, see <https://www.gnu.org/licenses/>.
 *
 * The initial version of the source code is inherited from gpg4usb-team.
 * Their source code version also complies with GNU General Public License.
 *
 * The source code version of this software was modified and released
 * by Saturneric<eric@bktus.com> starting on May 12, 2021.
 *
 */

#ifndef __KEYLIST_H__
#define __KEYLIST_H__

#include <utility>

#include "gpg/GpgContext.h"
#include "ui/KeyImportDetailDialog.h"

class Ui_KeyList;

namespace GpgFrontend::UI {

struct KeyListRow {
  using KeyType = unsigned int;

  static const KeyType SECRET_OR_PUBLIC_KEY = 0;
  static const KeyType ONLY_SECRET_KEY = 1;
};

struct KeyListColumn {
  using InfoType = unsigned int;

  static constexpr InfoType ALL = ~0;
  static constexpr InfoType TYPE = 1 << 0;
  static constexpr InfoType NAME = 1 << 1;
  static constexpr InfoType EmailAddress = 1 << 2;
  static constexpr InfoType Usage = 1 << 3;
  static constexpr InfoType Validity = 1 << 4;
  static constexpr InfoType FingerPrint = 1 << 5;
};

struct KeyTable {
  QTableWidget* key_list;
  KeyListRow::KeyType select_type;
  KeyListColumn::InfoType info_type;
  std::vector<GpgKey> buffered_keys;
  std::function<bool(const GpgKey&)> filter;
  KeyIdArgsListPtr checked_key_ids_;

  KeyTable(
      QTableWidget* _key_list, KeyListRow::KeyType _select_type,
      KeyListColumn::InfoType _info_type,
      std::function<bool(const GpgKey&)> _filter = [](const GpgKey&) -> bool {
        return true;
      })
      : key_list(_key_list),
        select_type(_select_type),
        info_type(_info_type),
        filter(std::move(_filter)) {}

  void Refresh(KeyLinkListPtr m_keys = nullptr);

  KeyIdArgsListPtr& GetChecked();

  void SetChecked(KeyIdArgsListPtr key_ids);
};

class KeyList : public QWidget {
  Q_OBJECT

 public:
  explicit KeyList(bool menu, QWidget* parent = nullptr);

  void addListGroupTab(
      const QString& name,
      KeyListRow::KeyType selectType = KeyListRow::SECRET_OR_PUBLIC_KEY,
      KeyListColumn::InfoType infoType = KeyListColumn::ALL,
      const std::function<bool(const GpgKey&)>& filter =
          [](const GpgKey&) -> bool { return true; });

  void setDoubleClickedAction(
      std::function<void(const GpgKey&, QWidget*)> action);

  void setColumnWidth(int row, int size);
  void addMenuAction(QAction* act);
  void addSeparator();

  KeyIdArgsListPtr getChecked();
  static KeyIdArgsListPtr getChecked(const KeyTable& key_table);
  KeyIdArgsListPtr getPrivateChecked();
  KeyIdArgsListPtr getAllPrivateKeys();
  void setChecked(KeyIdArgsListPtr key_ids);
  static void setChecked(const KeyIdArgsListPtr& keyIds,
                         const KeyTable& key_table);
  KeyIdArgsListPtr getSelected();
  std::string getSelectedKey();

  [[maybe_unused]] static void markKeys(QStringList* keyIds);

  [[maybe_unused]] bool containsPrivateKeys();

 signals:
  void signalRefreshStatusBar(const QString& message, int timeout);
  void signalRefreshDatabase();

 public slots:

  void slotRefresh();

 private:
  void init();
  void importKeys(const QByteArray& inBuffer);

  static int key_list_id;
  int _m_key_list_id;
  std::mutex buffered_key_list_mutex;

  std::shared_ptr<Ui_KeyList> ui;
  QTableWidget* mKeyList{};
  std::vector<KeyTable> mKeyTables;
  QMenu* popupMenu{};
  GpgFrontend::KeyLinkListPtr _buffered_keys_list;
  std::function<void(const GpgKey&, QWidget*)> mAction = nullptr;
  bool menu_status = false;

 private slots:

  void slotDoubleClicked(const QModelIndex& index);

  void slotRefreshUI();

  void slotSyncWithKeyServer();

 protected:
  void contextMenuEvent(QContextMenuEvent* event) override;

  void dragEnterEvent(QDragEnterEvent* event) override;

  void dropEvent(QDropEvent* event) override;
};

}  // namespace GpgFrontend::UI

#endif  // __KEYLIST_H__
