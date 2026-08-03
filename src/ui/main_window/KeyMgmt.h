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

#include "core/typedef/GpgTypedef.h"
#include "ui/main_window/GeneralMainWindow.h"
#include "ui/main_window/KeyActionState.h"

namespace GpgFrontend::UI {

class KeyList;
struct KeyTable;

/**
 * @brief
 *
 */
class KeyMgmt : public GeneralMainWindow {
  Q_OBJECT

 public:
  /**
   * @brief Construct a new Key Mgmt object
   *
   * @param parent
   */
  explicit KeyMgmt(QWidget* parent = nullptr);

 public slots:

  /**
   * @brief
   *
   */
  void SlotGenerateSubKey();

  /**
   * @brief
   *
   */
  void SlotExportKeyToKeyPackage();

  /**
   * @brief
   *
   */
  void SlotExportKeyToClipboard();

  /**
   * @brief
   *
   */
  void SlotExportAsOpenSSHFormat();

  /**
   * @brief
   *
   */
  void SlotDeleteSelectedKeys();

  /**
   * @brief
   *
   */
  void SlotDeleteCheckedKeys();

  /**
   * @brief
   *
   */
  void SlotGenerateKeyDialog();

  /**
   * @brief
   *
   */
  void SlotShowKeyDetails();

  /**
   * @brief
   *
   */
  void SlotImportKeyPackage();

 signals:

  /**
   * @brief
   *
   */
  void SignalStatusBarChanged(QString);

  /**
   * @brief
   *
   */
  void SignalKeyStatusUpdated();

 private slots:

  void slot_popup_menu_by_key_list(QContextMenuEvent* event, KeyTable*);

 protected:
  void dragEnterEvent(QDragEnterEvent* event) override;
  void dropEvent(QDropEvent* event) override;

 private:
  /**
   * @brief Recount the keyring and refresh the permanent status-bar summary.
   *
   * This is what makes the window worth opening: it answers "is anything about
   * to expire" without the user having to look for it.
   */
  void refresh_status_summary();

 private:
  /**
   * @brief Rebuild the category-membership submenu for the given target keys.
   *
   * Operates on all target keys at once (bulk), with a checkable entry per
   * custom category, a remove action when viewing a custom category tab, and a
   * "New Category" action.
   */
  void populate_key_category_menu(int channel, const QString& current_tab_id,
                                  const GpgAbstractKeyPtrList& keys);

  KeyList* key_list_;               ///<
  QLabel* status_summary_label_{};  ///<
  QToolBar* key_tool_bar_{};        ///<
  QMenu* file_menu_{};              ///<
  QMenu* edit_menu_{};              ///<
  QMenu* key_menu_{};               ///<
  QMenu* opera_menu_{};             ///<
  QMenu* view_menu_{};              ///<
  QMenu* generate_key_menu_{};      ///<
  QMenu* import_key_menu_{};        ///<
  QMenu* export_key_menu_{};        /// <

  QMenu* popup_menu_;
  QMenu* empty_area_menu_{};          ///<
  QMenu* add_key_2_category_menu_{};  ///<
  QMenu* keyserver_menu_{};           ///<
  QMenu* bulk_menu_{};                ///<
  QMenu* copy_menu_{};                ///<
  QMenu* popup_key_ops_menu_{};       ///<
  QMenu* popup_export_menu_{};        ///<
  QMenu* popup_keyserver_menu_{};     ///<

  QAction* open_key_file_act_{};                 ///<
  QAction* export_key_to_file_act_{};            ///<
  QAction* export_key_as_open_ssh_format_{};     ///<
  QAction* export_key_to_clipboard_act_{};       ///<
  QAction* delete_checked_keys_act_{};           ///<
  QAction* delete_selected_keys_act_{};          ///<
  QAction* generate_key_pair_act_{};             ///<
  QAction* generate_subkey_act_{};               ///<
  QAction* import_key_from_clipboard_act_{};     ///<
  QAction* import_key_from_file_act_{};          ///<
  QAction* import_key_from_key_server_act_{};    ///<
  QAction* import_keys_from_key_package_act_{};  ///<
  QAction* close_act_{};                         ///<
  QAction* refresh_keys_act_{};                  ///<
  QAction* focus_search_act_{};                  ///<
  QAction* show_key_details_act_{};              ///<
  QAction* set_owner_trust_of_key_act_{};        ///<

  // Quick actions (Part 2)
  QAction* copy_fingerprint_act_{};      ///<
  QAction* copy_key_id_act_{};           ///<
  QAction* copy_email_act_{};            ///<
  QAction* copy_public_key_act_{};       ///<
  QAction* export_public_key_act_{};     ///<
  QAction* export_private_key_act_{};    ///<
  QAction* certify_key_act_{};           ///<
  QAction* set_expiry_act_{};            ///<
  QAction* generate_revoke_cert_act_{};  ///<

  // Keyserver actions (Part 3)
  QAction* publish_key_to_key_server_act_{};         ///<
  QAction* refresh_selected_from_key_server_act_{};  ///<

  // Bulk actions (Part 4)
  QAction* bulk_set_owner_trust_act_{};     ///<
  QAction* bulk_extend_expiry_act_{};       ///<
  QAction* backup_all_private_keys_act_{};  ///<

  ///< Every gated action, by the rule that governs it. Walked once per
  ///< selection change.
  QHash<KeyAction, QAction*> action_map_;

  ///< Flat list of the actions reachable from the context menu, submenu
  ///< children included. Flat on purpose: the actions shared with the menu bar
  ///< are exactly the nested ones, and a per-menu walk would skip them.
  QVector<QAction*> popup_actions_;

  /**
   * @brief Build one action, wired up the way every KeyMgmt action should be.
   *
   * Sets the tooltip and the status tip from the same text, and stashes that
   * text under the "gfBaseToolTip" property. The gating pass appends a reason
   * to the tooltip when it disables an action, so it needs the untouched
   * original to restore once the action becomes usable again.
   */
  auto make_action(const QString& text, const QString& icon, const QString& tip,
                   const QContainer<QKeySequence>& shortcuts = {}) -> QAction*;

  /**
   * @brief Build the key-list context menu.
   *
   * Runs before create_menus(): it owns the Copy / Key Operations / Keyserver /
   * Category submenus, which the menu bar and the tool bar then reuse rather
   * than duplicating.
   */
  void create_popup_menu();

  /**
   * @brief Create a menus object
   *
   */
  void create_menus();

  /**
   * @brief Create a actions object
   *
   */
  void create_actions();

  /**
   * @brief Create the per-key quick actions (copy, certify, expiry, revoke).
   */
  void create_quick_actions();

  /**
   * @brief Create the keyserver actions (search, publish, refresh).
   */
  void create_keyserver_actions();

  /**
   * @brief Create the bulk actions operating on all checked keys.
   */
  void create_bulk_actions();

  /**
   * @brief Create a tool bars object
   *
   */
  void create_tool_bars();

  /**
   * @brief
   *
   * @param uidList
   */
  void delete_keys_with_warning(const GpgAbstractKeyPtrList& keys);

  /**
   * @brief Export the given keys, armored, to the system clipboard.
   *
   * Shared by the "export checked keys" toolbar action and the per-key
   * "Copy Public Key" context-menu entry.
   */
  void export_keys_to_clipboard(const GpgAbstractKeyPtrList& keys);

  /**
   * @brief Copy a plain string to the clipboard and flash a status message.
   */
  void copy_text_to_clipboard(const QString& text, const QString& what);

  /**
   * @brief The keys a multi-key operation should act on: the checked set when
   * anything is checked, otherwise the current selection.
   *
   * The checkboxes are for assembling a set across category tabs; the selection
   * is for acting on what is in front of you. Every operation that can take
   * several keys reads this, so selecting a row and hitting Export does the
   * obvious thing instead of refusing until you have also ticked its box.
   */
  [[nodiscard]] auto target_keys() const -> GpgAbstractKeyPtrList;

  /**
   * @brief Read the current selection, checked set, engine support and module
   * availability into a plain struct.
   *
   * The only place any of that is looked up; everything downstream reasons
   * about the struct, which is what makes the rules testable.
   */
  [[nodiscard]] auto build_key_action_context() const -> KeyActionContext;

  /**
   * @brief Bring every key action in line with what is currently selected.
   *
   * The single pass behind the menu bar, the tool bar and the context menu.
   * Visibility follows what the engine supports; enabled-state follows the
   * selection, with the reason appended to the tooltip so a greyed entry can
   * say why.
   */
  void update_key_action_state();

  /**
   * @brief Hide any submenu all of whose entries are hidden.
   */
  void sync_submenu_visibility();

  /**
   * @brief Publish the checked public key(s) to the default keyserver.
   */
  void publish_keys_to_key_server(const GpgAbstractKeyPtrList& keys);

  /**
   * @brief Set the owner-trust level of every checked key at once.
   */
  void bulk_set_owner_trust();

  /**
   * @brief Extend (or clear) the expiry of every checked private key at once.
   */
  void bulk_extend_expiry();

  /**
   * @brief Export every private key in the keyring as one key package backup.
   */
  void backup_all_private_keys();

  /**
   * @brief
   *
   */
  void apply_default_layout();
};

}  // namespace GpgFrontend::UI
