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

#include <functional>

#include "core/function/openpgp/PassphraseService.h"
#include "core/model/GFBuffer.h"

namespace GpgFrontend {

using PasswordFetcher = std::function<GFBuffer(const PassphraseState&)>;

/**
 * @brief Register a custom password fetcher for a specific channel.
 *
 * When set, the RPGP engine's FetchPasswordCallback will call this fetcher
 * directly for the given channel, bypassing the UI-based PassphraseService.
 * Pass an empty function to clear and restore the default UI behaviour.
 *
 * @param channel the channel to register the fetcher for
 * @param fetcher callback that returns a passphrase given a PassphraseState
 */
GF_CORE_EXPORT void SetChannelPasswordFetcher(int channel,
                                              PasswordFetcher fetcher);

/**
 * @brief Drop every passphrase cached by the RPGP engine, for every channel and
 * key.
 *
 * Needed when the keyring underneath the cache is replaced wholesale: an entry
 * cached for a fingerprint would otherwise keep serving a passphrase that no
 * longer unlocks the key now carrying that fingerprint.
 */
GF_CORE_EXPORT void ClearRpgpPasswordCache();

/**
 * @brief Answers the RPGP engine's request for a new symmetric password on the
 * calling thread from a passphrase given up front, for as long as it lives.
 *
 * The engine asks for the password synchronously on the thread that called
 * into it, so holding this around one symmetric encryption call hands it the
 * passphrase without a prompt. Only a request for a new password with no key
 * behind it is answered: the passphrase never unlocks a key. It is consulted
 * before the channel's PasswordFetcher and the interactive prompt.
 *
 * The passphrase is referenced, not copied; it must outlive the scope.
 */
class GF_CORE_EXPORT ScopedSymmetricPassphrase {
 public:
  explicit ScopedSymmetricPassphrase(const GFBuffer& passphrase);
  ~ScopedSymmetricPassphrase();

  ScopedSymmetricPassphrase(const ScopedSymmetricPassphrase&) = delete;
  auto operator=(const ScopedSymmetricPassphrase&)
      -> ScopedSymmetricPassphrase& = delete;

 private:
  const GFBuffer* previous_;
};

}  // namespace GpgFrontend
