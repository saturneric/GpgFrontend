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

#include "core/function/openpgp/OpenPGPContext.h"

namespace GpgFrontend {

/**
 * @brief Context passed to PassphraseService::RequestPassphrase describing the
 * request.
 */
struct PassphraseState {
  // Additional informational text to display in the passphrase dialog.
  QString info;

  // Fingerprint of the key the passphrase is needed for; empty if no existing
  // key.
  QString fpr;

  // True if this is a retry after a previously incorrect passphrase.
  bool retry = false;

  // True if the user should be prompted to enter a new passphrase rather than
  // unlock an existing key. Must be true when fpr is empty.
  bool ask_for_new = false;

  // True if the user should be asked to confirm the passphrase (e.g. when
  // setting a new one).
  bool should_confirm = false;

  ~PassphraseState() {
    // Clear sensitive data
    info.fill('X');
    info.clear();
    fpr.fill('X');
    fpr.clear();
    retry = false;
    ask_for_new = false;
    should_confirm = false;
  }
};

/**
 * @brief Singleton service that prompts the user for a passphrase.
 *
 * Delegates to the UI layer via the OpenPGP context's passphrase callback.
 * One instance per channel; each channel has its own associated context.
 */
class GF_CORE_EXPORT PassphraseService
    : public SingletonFunctionObject<PassphraseService> {
 public:
  /**
   * @brief Construct the service for the given singleton channel.
   *
   * @param channel singleton channel identifier
   */
  explicit PassphraseService(int channel);

  /**
   * @brief Prompt the user for a passphrase and return the entered value.
   *
   * Blocks until the user provides input or cancels. Returns an empty string
   * if the user cancels.
   *
   * @param state request context (key fingerprint, retry flag, confirmation
   * flag, etc.)
   * @return passphrase entered by the user, or empty string on cancellation
   */
  auto RequestPassphrase(const PassphraseState& state) -> GFBuffer;

 private:
  // OpenPGP context for this channel, used to route the passphrase callback.
  OpenPGPContext& ctx_ = OpenPGPContext::GetInstance(GetChannel());
};
}  // namespace GpgFrontend
