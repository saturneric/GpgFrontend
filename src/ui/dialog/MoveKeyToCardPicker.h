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

#include "core/model/KeyDatabaseInfo.h"
#include "core/typedef/GpgTypedef.h"
#include "ui/dialog/GeneralDialog.h"

namespace GpgFrontend::UI {

class KeyTreeView;

/**
 * @brief Let the user pick a single secret (sub)key to move onto a smart card.
 *
 * Reuses the key-tree picker infrastructure: the user chooses the key database
 * and then checks the (sub)key to move. Only the parts that can actually be
 * moved onto a card are checkable, and exactly one of them may be chosen.
 */
class MoveKeyToCardPicker : public GeneralDialog {
  Q_OBJECT

 public:
  explicit MoveKeyToCardPicker(QWidget* parent = nullptr);

  /**
   * @brief Key databases that support moving a key to a smart card.
   *
   * Smart card operations are GnuPG-only, so rpgp-backed databases are
   * excluded.
   *
   * @return the list of supported (GnuPG) key databases
   */
  [[nodiscard]] static auto SupportedDatabases() -> QContainer<KeyDatabaseInfo>;

  /**
   * @brief The primary key that owns the chosen (sub)key.
   *
   * @return the selected key, or nullptr if nothing was confirmed
   */
  [[nodiscard]] auto GetSelectedKey() const -> GpgKeyPtr;

  /**
   * @brief The key database (channel) the chosen key was picked from.
   *
   * @return the selected channel
   */
  [[nodiscard]] auto GetSelectedChannel() const -> int;

  /**
   * @brief Index of the chosen part within GetSelectedKey()->SubKeys().
   *
   * The primary key is index 0.
   *
   * @return the subkey index, or -1 if nothing was confirmed
   */
  [[nodiscard]] auto GetSelectedSubKeyIndex() const -> int;

 private slots:
  void update_confirm_button_state();

 protected:
  void showEvent(QShowEvent* event) override;

 private:
  int channel_;
  KeyTreeView* tree_view_;
  QComboBox* db_combo_ = nullptr;
  QPushButton* confirm_btn_ = nullptr;
  GpgKeyPtr selected_key_;
  int selected_subkey_index_ = -1;

  /**
   * @brief Resolve the single checked item into a key + subkey index.
   *
   * @return true when exactly one movable part is checked and resolvable
   */
  auto resolve_selection() -> bool;
};

}  // namespace GpgFrontend::UI
