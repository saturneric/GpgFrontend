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

#include "GpgPassphraseContext.h"

namespace GpgFrontend {

GpgPassphraseContext::GpgPassphraseContext(int channel, GpgAbstractKeyPtr key,
                                           const PassphraseState& state)
    : channel_(channel),
      passphrase_info_(state.info),
      key_(std::move(key)),
      prev_was_bad_(state.retry),
      ask_for_new_(state.ask_for_new),
      should_confirm_(state.should_confirm) {}

GpgPassphraseContext::GpgPassphraseContext() = default;

auto GpgPassphraseContext::GetPassphrase() const -> QString {
  return passphrase_;
}

void GpgPassphraseContext::SetPassphrase(const QString& passphrase) {
  passphrase_ = passphrase;
}

auto GpgPassphraseContext::GetChannel() const -> int { return channel_; }

auto GpgPassphraseContext::GetKey() const -> GpgAbstractKeyPtr { return key_; }

auto GpgPassphraseContext::GetPassphraseInfo() const -> QString {
  return passphrase_info_;
}

auto GpgPassphraseContext::IsPreWasBad() const -> bool { return prev_was_bad_; }

auto GpgPassphraseContext::IsAskForNew() const -> bool { return ask_for_new_; }

auto GpgPassphraseContext::ShouldConfirm() const -> bool {
  return should_confirm_;
}
}  // namespace GpgFrontend
