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

GpgPassphraseContext::GpgPassphraseContext(const QString& uids_info,
                                           const QString& passphrase_info,
                                           bool prev_was_bad, bool ask_for_new)
    : passphrase_info_(passphrase_info),
      uids_info_(uids_info),
      prev_was_bad_(prev_was_bad),
      ask_for_new_(ask_for_new) {}

GpgPassphraseContext::GpgPassphraseContext() = default;

auto GpgPassphraseContext::GetPassphrase() const -> QString {
  return passphrase_;
}

void GpgPassphraseContext::SetPassphrase(const QString& passphrase) {
  passphrase_ = passphrase;
}

auto GpgPassphraseContext::GetUidsInfo() const -> QString { return uids_info_; }

auto GpgPassphraseContext::GetPassphraseInfo() const -> QString {
  return passphrase_info_;
}

auto GpgPassphraseContext::IsPreWasBad() const -> bool { return prev_was_bad_; }

auto GpgPassphraseContext::IsAskForNew() const -> bool { return ask_for_new_; }
}  // namespace GpgFrontend
