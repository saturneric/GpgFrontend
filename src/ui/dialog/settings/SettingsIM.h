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

namespace GpgFrontend::UI {

/**
 * @brief Settings tab for the instant-messaging feature.
 *
 * Holds the shared "Message Book Phrase" — the secret that whitens every
 * instant-messaging token so it cannot be detected as a GpgFrontend/PGP message.
 * Both sides must set the same phrase; an empty phrase uses a shared default
 * that only hides the format from naive scanners.
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
  QLineEdit* book_phrase_edit_{};  ///< shared Message Book Phrase
};

}  // namespace GpgFrontend::UI
