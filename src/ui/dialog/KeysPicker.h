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
 * @brief Dialog to pick key(s) or a particular subkey for one purpose.
 *
 * Expanding a key lets the user pin a specific subkey; the chosen subkey is
 * carried to the engine through the `<fpr>!` armor-block marker. What may be
 * picked, and every word shown, comes from the subclass, so its translations
 * stay its own.
 */
class KeysPicker : public GeneralDialog {
  Q_OBJECT

 public:
  /// The words a picker shows, already translated.
  struct Texts {
    QString window_title;
    QString heading;
    QString description;
    QString confirm;
    QString cancel;
    QString none_selected_title;
    QString none_selected;
  };

  using Filter = std::function<bool(const GpgAbstractKey*)>;

  /**
   * @param restrict_keys when non-empty, only these keys (and their subkeys)
   * are shown: used when the user has already checked specific keys in the key
   * toolbox before triggering the operation.
   * @param row_filter whether a key or subkey row may be offered at all
   * @param key_filter whether a primary key qualifies; subkeys pass on
   * @p row_filter alone
   */
  KeysPicker(const QString& name, int channel,
             const GpgAbstractKeyPtrList& restrict_keys, const Texts& texts,
             const Filter& row_filter, const Filter& key_filter,
             QWidget* parent);

  /// The checked keys and subkeys, each pinned with its `!` marker.
  [[nodiscard]] auto GetKeys() const -> GpgAbstractKeyPtrList;

 private slots:
  void update_confirm_button_state();

 protected:
  void showEvent(QShowEvent* event) override;

 private:
  int channel_;
  KeyTreeView* tree_view_;
  QPushButton* confirm_btn_ = nullptr;
};

/// Recipient key(s) or encryption subkey(s). Revoked subkeys are not offered.
class EncryptionKeysPicker : public KeysPicker {
  Q_OBJECT

 public:
  EncryptionKeysPicker(int channel, const GpgAbstractKeyPtrList& restrict_keys,
                       QWidget* parent = nullptr);
};

/// Private key(s) or signing subkey(s).
class SigningKeysPicker : public KeysPicker {
  Q_OBJECT

 public:
  explicit SigningKeysPicker(int channel, QWidget* parent = nullptr);

  SigningKeysPicker(int channel, const GpgAbstractKeyPtrList& restrict_keys,
                    QWidget* parent = nullptr);
};

}  // namespace GpgFrontend::UI
