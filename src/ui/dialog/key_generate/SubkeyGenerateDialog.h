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
#include "core/typedef/GpgTypedef.h"
#include "ui/dialog/GeneralDialog.h"
#include "ui/function/KeyGenerateHelper.h"

namespace GpgFrontend::UI {

class ExpiryPicker;
class KeyAlgoEditor;
class ToggleSwitch;

/**
 * @brief Add a subkey to a key pair.
 *
 * Asked as what the subkey is for, not which algorithm it uses: each purpose
 * picks the algorithm that matches the primary key, and Expert Mode shows
 * every choice for anyone who wants another.
 *
 * Opens itself and is deleted when closed.
 */
class SubkeyGenerateDialog : public GeneralDialog {
  Q_OBJECT

 public:
  /**
   * @param channel the key database the key is in
   * @param key the key pair the subkey is added to
   * @param parent parent widget
   */
  explicit SubkeyGenerateDialog(int channel, GpgKeyPtr key, QWidget* parent);

 private:
  int current_gpg_context_channel_;
  GpgKeyPtr key_;
  QSharedPointer<KeyGenerateInfo> gen_subkey_info_;
  QList<KeyAlgo> supported_subkey_algos_;
  QString primary_algo_;

  QButtonGroup* purpose_group_;
  ToggleSwitch* expert_switch_{};
  ExpiryPicker* expiry_{};
  KeyAlgoEditor* editor_;
  QCheckBox* no_passphrase_check_;
  QPushButton* generate_button_{};

  /// @brief Set the subkey up for a purpose.
  void apply_purpose(SubkeyPurpose purpose);

  void mark_custom();

  void refresh_generate();

  /// @brief Fit the height to the view shown.
  void fit_height();

  void slot_key_gen_accept();
};

}  // namespace GpgFrontend::UI
