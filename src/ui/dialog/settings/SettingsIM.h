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

class QLabel;
class QTimer;
class QPlainTextEdit;
class QPushButton;

namespace GpgFrontend::UI {

/**
 * @brief Settings tab for the instant-messaging feature.
 *
 * Holds the shared "Message Book Phrase", the secret that whitens every
 * instant-messaging token so it cannot be detected as a GpgFrontend/PGP
 * message. Both sides must set the same phrase; an empty phrase uses a shared
 * default that only hides the format from naive scanners.
 *
 * The phrase itself is stored in the encrypted durable cache, never in the
 * settings file; the tab reads and writes it through InstantMessageOperator.
 */
class InstantMessagingTab : public QWidget {
  Q_OBJECT

 public:
  explicit InstantMessagingTab(QWidget* parent = nullptr);

  /// Load current values from the secure store into the controls.
  void SetSettings();

  /// Persist the controls' values back to the secure store.
  void ApplySettings();

 private:
  /// Show the phrase itself, or a mask standing in for it.
  void set_revealed(bool revealed);

  /// Adopt @p phrase as the edited value and refresh everything that shows it.
  void set_phrase(const QString& phrase);

  /// Restart the debounce that recomputes the fingerprint of the phrase.
  void schedule_fingerprint_update();

  /// Derive the fingerprint of the current phrase off the UI thread.
  void start_fingerprint_update();

  /// Show @p fingerprint, or a placeholder while none is known.
  void set_fingerprint(const QString& fingerprint);

  QPlainTextEdit* phrase_edit_{};   ///< shows the phrase, or its mask
  QPushButton* reveal_button_{};    ///< toggles between the phrase and the mask
  QLabel* fingerprint_label_{};     ///< our fingerprint, e.g. "3F9A-1C4E"
  QLabel* phrase_state_label_{};    ///< "no phrase set" / phrase length
  QTimer* fingerprint_timer_{};     ///< debounce for typing
  QString phrase_;                  ///< the edited phrase, source of truth
  QString fingerprint_;             ///< last computed fingerprint, may be empty
  bool revealed_{false};            ///< whether the phrase is on screen
  bool syncing_{false};             ///< guards programmatic edits of the view
  quint64 fingerprint_request_{0};  ///< discards results of stale derivations
};

}  // namespace GpgFrontend::UI
