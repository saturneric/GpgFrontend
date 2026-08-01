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

#include "core/function/CoreSignalStation.h"

#include "core/model/GpgPassphraseContext.h"

std::unique_ptr<GpgFrontend::CoreSignalStation>
    GpgFrontend::CoreSignalStation::instance = nullptr;

auto GpgFrontend::CoreSignalStation::GetInstance()
    -> GpgFrontend::CoreSignalStation* {
  if (instance == nullptr) {
    // Registered here rather than at UI startup because the first connection to
    // SignalBadOpenPGPEnv can be made before that point. The name must match
    // the parameter spelling moc writes into the signal signature exactly,
    // which is why the signal declares the fully qualified type.
    qRegisterMetaType<GpgFrontend::BadOpenPGPEnvReason>(
        "GpgFrontend::BadOpenPGPEnvReason");

    // The passphrase signals always cross a thread boundary: they are emitted
    // from whichever task runner needs the passphrase and handled on the GUI
    // thread. Registered here, next to the signals themselves, because core
    // owns them — leaving it to UI startup means a core-only build (the test
    // binary, for one) has its passphrase requests dropped by Qt with only a
    // console warning to show for it.
    qRegisterMetaType<QSharedPointer<GpgPassphraseContext> >(
        "QSharedPointer<GpgPassphraseContext>");

    instance = std::make_unique<CoreSignalStation>();
  }
  return instance.get();
}
