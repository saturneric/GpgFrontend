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

inline auto operator|(KeyMenuAbility lhs, KeyMenuAbility rhs)
    -> KeyMenuAbility {
  using T = std::underlying_type_t<KeyMenuAbility>;
  return static_cast<KeyMenuAbility>(static_cast<T>(lhs) | static_cast<T>(rhs));
}

inline auto operator|=(KeyMenuAbility& lhs, KeyMenuAbility rhs)
    -> KeyMenuAbility& {
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
      GpgKeyTableColumn fixed_columns_filter = GpgKeyTableColumn::kALL,
      QWidget* parent = nullptr);

  /**
   * @brief
   *
   * @param channel
   * @param menu_ability
   * @param fixed_column_filter
   */
  void InitAfter(
      int channel, KeyMenuAbility menu_ability,
      GpgKeyTableColumn fixed_columns_filter = GpgKeyTableColumn::kALL);

  /**
   * @brief Scope under which this key list persists its tab order.
   *
   * Distinct key lists (e.g. the Key ToolBox dock and the Key Management
   * window) should use distinct keys so their tab orders do not interfere.
   *
   * @param settings_key QSettings key used to store the tab order
   */
  void SetTabOrderSettingsKey(const QString& settings_key);

  /**
   * @brief Scope under which this key list persists its visible-column choice,
   * and the default column set to show when nothing is persisted yet.
   *
   * Distinct key lists (e.g. the Key ToolBox dock and the Key Management
   * window) should use distinct keys so their column choices stay independent.
   * The ToolBox uses this to default to a compact, operation-focused set while
   * the Key Management window keeps the full set.
   *
   * @param settings_key QSettings key used to store the column filter
   * @param default_columns columns shown when the key has no stored value
   */
  void SetColumnFilterSettingsKey(
      const QString& settings_key,
      GpgKeyTableColumn default_columns = GpgKeyTableColumn::kALL);

  /**
   * @brief Scope under which this key list persists its column widths in the
   * durable cache.
   *
   * Distinct key lists (e.g. the Key ToolBox dock and the Key Management
   * window) should use distinct scopes: the narrow dock and the wide window
   * want very different widths. Within one key list every category tab shares
   * the same widths.
   *
   * @param scope scope name, e.g. "keymgmt"
   */
  void SetColumnWidthsScope(const QString& scope);

  /**
   * @brief Render the category strip as a compact colour rail (a swatch of the
   * category colour with the name in a tooltip) instead of the default
   * full-width text list.
   *
   * Intended for the space-constrained main-window dock; the Key Management
   * window keeps the text list. Call before category tabs are added.
   *
   * @param compact true for the colour rail, false for the text list
   */
  void SetCategoryRailCompact(bool compact = true);

  /**
   * @brief Enable the full category-management affordances on this key list:
   * an add-category (+) button and a context menu offering New / Rename /
   * Set Colour / Manage Members / Delete.
   *
   * Intended for the Key Management window; the main-window rail stays a simple
   * switcher. Call before category tabs are added.
   *
   * @param enabled true to expose the management tools
   */
  void SetCategoryManagementEnabled(bool enabled = true);

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
      GpgKeyTableColumn custom_columns_filter = GpgKeyTableColumn::kALL,
      const QString& category_id = {}, const QString& color_hint = {})
      -> KeyTable*;

  /**
   * @brief Synchronise the user-defined category tabs with the repository.
   *
   * Appends a tab for every custom category that has no tab yet and removes
   * tabs whose category no longer exists, leaving all other tabs untouched so
   * membership-only changes never disturb the current tab, selection, or
   * scroll position.
   */
  void RebuildCategoryTabs();

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

  /**
   * @brief React to a change of the selected category row: show the matching
   * key table page and refresh action state.
   *
   * @param row newly selected row in the category source list
   */
  void slot_current_category_changed(int row);

  /**
   * @brief Show a context menu for the category source list. User-defined
   * category rows (id prefixed "cat:") offer a "Delete Category" action.
   *
   * @param pos position within the source list viewport
   */
  void slot_category_context_menu(const QPoint& pos);

 private:
  QSharedPointer<Ui_KeyList> ui_;                                    ///<
  std::function<void(const GpgKey&, QWidget*)> m_action_ = nullptr;  ///<
  int current_gpg_context_channel_;
  KeyMenuAbility menu_ability_ = KeyMenuAbility::kALL;  ///<
  QSharedPointer<GpgKeyTableModel> model_;
  GpgKeyTableColumn fixed_columns_filter_;
  GpgKeyTableColumn global_column_filter_;

  ///< Maps a category id (the KeyTable object name) to its page in keyStack.
  QHash<QString, KeyTable*> pages_;

  ///< When true the category strip is a compact colour rail (main-window dock);
  ///< otherwise a full-width text list (Key Management window, dialogs).
  bool compact_rail_ = false;

  ///< Id of the primary category (the first one added) that is always kept at
  ///< the top of the strip regardless of the persisted order.
  QString pinned_first_id_;

  ///< When true this key list exposes the full category-management tools
  ///< (add button + New/Rename/Manage/Delete menu); the main-window rail does
  ///< not.
  bool category_management_enabled_ = false;

  QAction* key_id_column_action_;
  QAction* algo_column_action_;
  QAction* create_date_column_action_;
  QAction* owner_trust_column_action_;
  QAction* subkeys_number_column_action_;
  QAction* comment_column_action_;

  QTimer* search_timer_ = nullptr;
  bool applying_tab_order_ = false;
  QString tab_order_settings_key_ = "keys/key_tab_order";
  QString column_filter_settings_key_ = "keys/global_columns_filter";
  QString column_widths_scope_ = "global";

  /**
   * @brief
   *
   */
  void init();

  /**
   * @brief Persist the current tab order (by object name) to settings.
   */
  void save_tab_order();

  /**
   * @brief Reorder the tabs to match the persisted orders: the per-window
   * integrated order and the shared custom-category order, merged as the
   * integrated block (primary pinned first) followed by the custom block.
   */
  void apply_saved_tab_order();

  /**
   * @brief Delete a user-defined category after confirmation.
   *
   * Removes the grouping only; the keys themselves are untouched.
   *
   * @param id category id (must be prefixed "cat:")
   * @param name display name shown in the confirmation prompt
   */
  void delete_category(const QString& id, const QString& name);

  /**
   * @brief The key table currently shown in the stacked pages, or nullptr.
   */
  [[nodiscard]] auto current_page() const -> KeyTable*;

  /**
   * @brief The key table page registered under a category id, or nullptr.
   */
  [[nodiscard]] auto page_for_id(const QString& id) const -> KeyTable*;

  /**
   * @brief Set the context button label to the given key database name.
   */
  void set_context_button_text(const QString& db_name);

  /**
   * @brief Apply the category strip styling for the current strip mode
   * (compact colour rail vs. full-width text list).
   */
  void apply_category_strip_style();

  /**
   * @brief Resolve the display colour for a category.
   *
   * Prefers an explicit stored colour hint, then a semantic colour for known
   * built-in categories, and finally a stable colour derived from the id.
   */
  [[nodiscard]] auto resolve_category_color(const QString& id,
                                            const QString& hint) const
      -> QColor;

  /**
   * @brief Build a colour-swatch icon for a category. Built-in categories are
   * drawn as rounded squares and custom ones as circles.
   *
   * @param color the swatch colour
   * @param custom true for a user-defined category
   */
  [[nodiscard]] auto make_category_icon(const QColor& color, bool custom) const
      -> QIcon;

  /**
   * @brief Row size for a category strip item.
   *
   * Set explicitly on every row: left to the delegate, the height comes out of
   * the platform style's item metrics, which are noticeably taller on Windows
   * than on Linux and stretch the compact rail's swatch spacing.
   */
  [[nodiscard]] auto category_item_size_hint() const -> QSize;

  /**
   * @brief (Re-)apply category_item_size_hint() to every row in the strip.
   */
  void apply_category_item_metrics();

  /**
   * @brief Prompt for a colour and persist it as this category's user-chosen
   * colour; all open key lists refresh their swatches.
   *
   * @param id category id
   */
  void choose_category_color(const QString& id);

  /**
   * @brief The source-list row carrying the given category id, or nullptr.
   */
  [[nodiscard]] auto item_for_id(const QString& id) const -> QListWidgetItem*;

  /**
   * @brief Select the category row with the given id, if present.
   */
  void select_category(const QString& id);

  /**
   * @brief Re-render every row's colour swatch from the current settings
   * (used after a colour change so all open key lists stay consistent).
   */
  void refresh_category_icons();

  /**
   * @brief Prompt for a name (and colour) and create a new custom category.
   */
  void new_category();

  /**
   * @brief Prompt for a new name and rename a custom category.
   */
  void rename_category(const QString& id, const QString& current_name);

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

  /**
   * @brief
   *
   * @param key_ids
   */
  void sync_keys_from_key_server(
      const KeyIdArgsList& key_ids,
      const std::function<void(const QString&, const QString&, size_t, size_t)>&
          callback) const;

  /**
   * @brief
   *
   */
  void init_ui_visibility();

  /**
   * @brief
   *
   */
  void init_ui_style();

  /**
   * @brief
   *
   */
  void init_context_menu();

  /**
   * @brief
   *
   */
  void init_column_menu();

  /**
   * @brief
   *
   */
  void init_signals();

  /**
   * @brief
   *
   */
  void init_texts();

  /**
   * @brief
   *
   */
  void update_action_state();

  /**
   * @brief
   *
   * @param ability
   * @return true
   * @return false
   */
  [[nodiscard]] auto has_ability(KeyMenuAbility ability) const -> bool;
};

}  // namespace GpgFrontend::UI
