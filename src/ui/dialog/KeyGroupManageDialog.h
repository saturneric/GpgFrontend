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

#include "core/function/openpgp/KeyGroupRepository.h"
#include "core/model/GpgKeyGroup.h"
#include "ui/dialog/GeneralDialog.h"

class QLabel;
class QLineEdit;
class QTimer;
class QToolButton;

namespace GpgFrontend::UI {

class KeyList;
class KeyTreeView;

/**
 * @brief Manage one key group: its members, its name and its existence.
 *
 * The left pane is a tree of the group's direct members, where a nested group
 * expands to show what it holds; the right pane offers every key that may
 * still be added.
 */
class KeyGroupManageDialog : public GeneralDialog {
  Q_OBJECT
 public:
  /**
   * @brief Construct a new Key Group Manage Dialog object.
   *
   * @param channel gpg context channel
   * @param key_group key group to manage
   * @param parent parent widget
   */
  explicit KeyGroupManageDialog(int channel,
                                const QSharedPointer<GpgKeyGroup>& key_group,
                                QWidget* parent = nullptr);

 protected:
  void showEvent(QShowEvent* event) override;

 private slots:
  void slot_add_to_key_group();
  void slot_remove_from_key_group();
  void slot_edit_metadata();
  void slot_delete_group();
  void slot_reload();
  void slot_notify_invalid_key_ids();
  void slot_update_action_state();
  void slot_members_context_menu(const QPoint& pos);

 private:
  int channel_;
  // The id, not the object: KeyGroupRepository rebuilds every GpgKeyGroup when
  // the key database changes, which would leave a held pointer editing an
  // orphan.
  QString group_id_;

  KeyTreeView* members_view_ = nullptr;
  KeyList* available_list_ = nullptr;
  QLineEdit* member_filter_ = nullptr;
  QTimer* filter_timer_ = nullptr;

  QLabel* icon_label_ = nullptr;
  QLabel* name_label_ = nullptr;
  QLabel* identity_label_ = nullptr;
  QLabel* footer_label_ = nullptr;
  QToolButton* add_button_ = nullptr;
  QToolButton* remove_button_ = nullptr;

  bool invalid_prompt_shown_ = false;

  // Re-resolve the managed group; nullptr means it is gone.
  [[nodiscard]] auto group() const -> QSharedPointer<GpgKeyGroup>;

  // Build the header card, the two panes and the footer.
  void init_ui();
  void init_header_card(QVBoxLayout* layout);
  void init_panes(QVBoxLayout* layout);

  // Redraw the header card and the footer summary from the current group.
  void refresh_header();

  // Refresh both panes, the header, the footer and the button states, then
  // tell the rest of the app that membership changed.
  void refresh_after_mutation(const QStringList& failed_ids,
                              const QString& failure_title,
                              const QString& failure_text);
};

}  // namespace GpgFrontend::UI
