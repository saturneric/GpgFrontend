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

}  // namespace GpgFrontend
