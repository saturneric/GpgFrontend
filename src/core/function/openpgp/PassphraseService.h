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

// State struct for passphrase request, can be extended in the future if needed
struct PassphraseState {
  QString info;  ///< Additional info to show in the passphrase dialog
  QString fpr;   ///< Fingerprint of the key related to the passphrase request
  bool retry = false;  ///< Indicates if this is a retry after a failed attempt
  bool ask_for_new = false;  ///< Indicates if the user should be prompted to
                             ///< set a new passphrase
};

class GF_CORE_EXPORT PassphraseService
    : public SingletonFunctionObject<PassphraseService> {
 public:
  /**
   * @brief Construct a new Passphrase Service object
   *
   * @param channel
   */
  explicit PassphraseService(int channel);

  /**
   * @brief
   *
   * @param state
   * @return QString
   */
  auto RequestPassphrase(const PassphraseState& state) -> QString;

 private:
  OpenPGPContext& ctx_ = OpenPGPContext::GetInstance(GetChannel());
};
}  // namespace GpgFrontend