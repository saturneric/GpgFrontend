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

#include "core/model/GpgKeyGenerateInfo.h"
#include "ui/dialog/GeneralWizard.h"
#include "ui/dialog/key_generate/KeyGenerateSummary.h"
#include "ui/struct/settings_object/KeyGenSO.h"

namespace GpgFrontend::UI {

class ExpiryPicker;
class ToggleSwitch;
class KeyAlgoEditor;
class MetaListPanel;

/**
 * @brief Make a new key pair, step by step.
 *
 * A profile from Settings answers every technical question at once, so most
 * people only type a name and press Continue. Expert Mode, a switch in the
 * button row, replaces the profile step with Key Details: every choice the
 * engines offer, each algorithm and its length, the second half of a hybrid,
 * what each key may do, when each expires, the key format and whether there is
 * a subkey at all.
 *
 * Opens itself and is deleted when closed.
 */
class KeyGenerateWizard : public GeneralWizard {
  Q_OBJECT

 public:
  /**
   * @param channel the key database the key goes into
   * @param parent parent widget
   */
  KeyGenerateWizard(int channel, QWidget* parent);

 private:
  int channel_;
  bool expiry_supported_;
  bool format_choice_;  ///< the engine can make more than one key format
  QList<KeyAlgo> primary_algos_;
  QList<KeyAlgo> subkey_algos_;
  KeyGenSO settings_;
  QList<ResolvedKeyGenProfile> offered_;
  int default_profile_ = -1;  ///< index into offered_
  int current_profile_ = -1;  ///< index into offered_, -1 for custom

  QSharedPointer<KeyGenerateInfo> key_info_;
  QSharedPointer<KeyGenerateInfo> subkey_info_;

  std::array<GeneralWizardPage*, kKeyGeneratePageCount> pages_{};

  QLineEdit* name_edit_{};
  QLineEdit* email_edit_{};
  QLineEdit* comment_edit_{};
  QLabel* name_problem_{};
  QLabel* email_problem_{};
  QLabel* comment_problem_{};

  QButtonGroup* profile_group_{};
  QComboBox* more_combo_{};
  QToolButton* options_toggle_{};  ///< Key Type's "Advanced options"
  QComboBox* start_combo_{};       ///< Key Details' "Start from"
  ToggleSwitch* expert_switch_{};
  bool switching_mode_ = false;  ///< passing back through Identity to swap
  KeyAlgoEditor* primary_editor_{};
  KeyAlgoEditor* subkey_editor_{};
  QCheckBox* subkey_check_{};
  QWidget* format_row_{};
  QComboBox* format_combo_{};

  QFrame* validity_card_{};
  ExpiryPicker* expiry_{};
  QCheckBox* passphrase_check_{};
  QLabel* passphrase_warning_{};
  QLabel* passphrase_twice_{};

  MetaListPanel* review_{};

  auto add_page(KeyGenerateStep step) -> GeneralWizardPage*;

  void build_identity_page();
  void build_key_type_page();
  void build_key_details_page();

  /// @brief Put the Expert Mode switch in the button row.
  void setup_expert_switch();

  /// @brief Show what depends on Expert Mode outside the second step.
  void sync_expert_mode();

  /// @brief Identity leads to Key Type, or with Expert Mode to Key Details.
  [[nodiscard]] auto nextId() const -> int override;
  void build_protection_page();
  void build_review_page();

  /// @brief The state the step rules judge.
  [[nodiscard]] auto state() const -> KeyGenerateState;

  void refresh_complete();

  void refresh_identity_problems();

  void fill_from_pattern();

  /// @brief Set every parameter from an offered profile.
  void apply_profile(int index);

  /// @brief The choices were changed by hand: the result is no profile.
  void mark_custom();

  /// @brief Leave every profile tile unselected.
  void clear_tiles();

  void set_has_subkey(bool on);

  /// @brief Pin the key format a post-quantum algorithm needs.
  void sync_key_format();

  void sync_passphrase();

  void save_as_profile();

  /// @brief Generate, and close on success.
  auto generate() -> bool;

  void slot_page_changed(int id);
};

}  // namespace GpgFrontend::UI
