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
#include "ui/dialog/GeneralDialog.h"
#include "ui/function/KeyGenProfile.h"

namespace GpgFrontend::UI {

class KeyAlgoEditor;

/**
 * @brief Make or change one key generation profile.
 *
 * The same editors the wizard's advanced options use, without a user id: a
 * profile says what kind of key, never whose.
 */
class KeyGenProfileEditDialog : public GeneralDialog {
  Q_OBJECT

 public:
  /**
   * @param channel the key database whose engine decides what is offered
   * @param resolved the profile to start from, already resolved for it
   * @param others the other profiles, whose names this one may not take
   * @param parent parent widget
   */
  KeyGenProfileEditDialog(int channel, const ResolvedKeyGenProfile& resolved,
                          QList<KeyGenProfile> others, QWidget* parent);

  /// @brief The profile as edited; valid once accepted.
  [[nodiscard]] auto Profile() const -> KeyGenProfile;

 private:
  int channel_;
  QList<KeyGenProfile> others_;
  KeyGenProfile base_;  ///< what is kept that the editors do not show
  QSharedPointer<KeyGenerateInfo> key_info_;
  QSharedPointer<KeyGenerateInfo> subkey_info_;
  QList<KeyAlgo> subkey_algos_;

  QLineEdit* name_edit_;
  QLineEdit* description_edit_;
  QLabel* name_problem_;
  KeyAlgoEditor* primary_editor_;
  QCheckBox* subkey_check_;
  KeyAlgoEditor* subkey_editor_;
  QPushButton* ok_button_;

  void refresh_ok();
};

}  // namespace GpgFrontend::UI
