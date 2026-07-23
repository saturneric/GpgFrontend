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
#include "ui/dialog/GeneralDialog.h"

namespace GpgFrontend::UI {

class KeyTreeView;

/**
 * @brief Dialog to pick the encryption key(s) / subkey(s) for recipients.
 *
 * Encryption counterpart of SigningKeysPicker. Expanding a recipient key lets
 * the user pin a specific encryption subkey; the chosen subkey is carried to
 * the engine through the `<fpr>!` armor-block marker. Revoked subkeys are not
 * offered.
 */
class EncryptionKeysPicker : public GeneralDialog {
  Q_OBJECT

 public:
  explicit EncryptionKeysPicker(int channel, QWidget* parent = nullptr);

  /**
   * @brief Restrict the selectable keys to the given set.
   *
   * When @p restrict_keys is non-empty, only those keys (and their encryption
   * subkeys) are shown; everything else in the keyring is filtered out. This is
   * used when the user has already checked specific recipients in the key
   * toolbox before triggering an encryption operation. An empty list shows all
   * encryption-capable keys.
   */
  EncryptionKeysPicker(int channel, const GpgAbstractKeyPtrList& restrict_keys,
                       QWidget* parent = nullptr);

  [[nodiscard]] auto GetEncryptionKeys() const -> GpgAbstractKeyPtrList;

 private slots:
  void update_confirm_button_state();

 protected:
  void showEvent(QShowEvent* event) override;

 private:
  int channel_;
  KeyTreeView* tree_view_;
  QPushButton* confirm_btn_ = nullptr;
};

}  // namespace GpgFrontend::UI
