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

class QLineEdit;
class QComboBox;
class QCheckBox;
class QGroupBox;

namespace GpgFrontend::UI {

/**
 * @brief Settings tab for the instant-messaging (forward-secret) feature.
 *
 * Holds the shared "Message Book Phrase" — an optional extra secret mixed into
 * every forward-secret session. Forward secrecy itself needs no configuration;
 * it is established automatically on first exchange.
 */
class InstantMessagingTab : public QWidget {
  Q_OBJECT

 public:
  explicit InstantMessagingTab(QWidget* parent = nullptr);

  /// Load current values from the global settings into the controls.
  void SetSettings();

  /// Persist the controls' values back to the global settings.
  void ApplySettings();

 private:
  QCheckBox* forward_secrecy_check_{};  ///< enable the Double Ratchet (PFS)
  QGroupBox* identity_box_{};           ///< Identity group (forward-secrecy only)
  QComboBox* identity_key_combo_{};     ///< OpenPGP key that signs IM handshakes
  QGroupBox* book_box_{};               ///< Message Book group (forward-secrecy only)
  QLineEdit* book_phrase_edit_{};       ///< shared Message Book Phrase

  /// Populate the identity-key combo with the user's private, signing keys.
  void populate_identity_keys();

  /// Enable the Identity and Message Book groups only when forward secrecy is
  /// on — they play no role in plain Normal mode.
  void update_fs_dependent_state();
};

}  // namespace GpgFrontend::UI
