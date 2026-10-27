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

#include <array>

#include "core/function/SettingItem.h"
#include "ui/dialog/GeneralWizard.h"
#include "ui/dialog/profile/ProfileCreateSummary.h"

class QLineEdit;
class QRadioButton;
class QCheckBox;

namespace GpgFrontend::UI {

/**
 * @brief Ask for the name, the keyring and the starting settings of a new
 * profile, one step each.
 *
 * The keyring question is where the profile's built-in key database lives,
 * and it is asked rather than defaulted because both answers are wrong for
 * somebody. Inside the profile it is isolated and can be packaged, but the
 * profile opens with no keys at all, which reads as "my keys are gone". This
 * computer's database (the GnuPG home, where there is GnuPG) is familiar, but
 * the profile is then not really separate and its keys cannot travel inside a
 * package.
 *
 * A GeneralWizard, the same frame as Export Profile, so creating a profile and
 * carrying one away read as one family.
 */
class ProfileCreateDialog : public GeneralWizard {
  Q_OBJECT

 public:
  /**
   * @brief Construct the wizard.
   *
   * A name is all that is asked for. The directory a profile lives in is named
   * by a generated id that nobody types and nobody has to keep unique, so there
   * is no folder question here and no collision to catch.
   *
   * @param parent parent widget
   */
  explicit ProfileCreateDialog(QWidget* parent);

  /// Free-form display name.
  [[nodiscard]] auto DisplayName() const -> QString;

  /// Whether the profile keeps its own keyring rather than the system one.
  [[nodiscard]] auto SelfContained() const -> bool;

  /// The setting groups the profile starts with from this window; empty when
  /// it starts from the defaults.
  [[nodiscard]] auto InheritedGroups() const -> QList<SettingGroup>;

 private slots:
  void slot_page_changed(int id);

 private:
  std::array<GeneralWizardPage*, kProfileCreateStepCount> pages_{};

  QLineEdit* name_edit_{};
  QRadioButton* own_keyring_{};
  QRadioButton* system_keyring_{};
  QCheckBox* inherit_settings_{};
  QList<std::pair<SettingGroup, QCheckBox*>> group_boxes_;

  /// @brief Step 1: what the profile is called.
  void build_name_page();

  /// @brief Step 2: whose keyring it uses.
  void build_keys_page();

  /// @brief Step 3: which of this window's settings it starts with.
  void build_settings_page();

  /// @brief A page for a step, titled and registered under the step's id.
  auto add_page(ProfileCreateStep step) -> GeneralWizardPage*;

  /// Make the select-all say what the group boxes add up to.
  void sync_inherit_master();
};

}  // namespace GpgFrontend::UI
