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

#include <QString>
#include <QStringList>

#include "GFSDKContext.h"
#include "GFSDKModuleApi.h"

namespace gf::runtime {

/**
 * @brief What this module knows about itself, copied out of the host's payload.
 *
 * The host's GFModuleBootstrapInfo is borrowed and dies when activate()
 * returns, so nothing may point into it afterwards. This is the owned copy,
 * and it is the only thing the rest of the runtime reads.
 */
struct RuntimeFacts {
  QString id;
  QString version;
  QString translation_context;
  QString locale;
  QStringList capabilities;
  QStringList events;
  QStringList commands;

  /// Whether a verified manifest backed the fields above.
  bool verified = false;

  /// @c id, encoded once, so a C string of it can be handed out for good.
  QByteArray id_utf8;
};

/**
 * @brief This module's facts, as the current activation established them.
 *
 * Empty before the first activation. Published whole and never written in
 * place: the module's own threads read these while a later activation may be
 * replacing them, and a reference taken here stays valid for the life of the
 * process.
 */
auto Facts() -> const RuntimeFacts&;

/// Make @p facts the current facts. At activation.
void PublishFacts(RuntimeFacts facts);

/**
 * @brief Build this module's SDK context from the table activate() received.
 *
 * Called once, before any hook runs. The runtime owns the context because the
 * runtime is what has a lifecycle: gf_sdk is stateless and reads whatever it
 * is handed.
 *
 * @param host the table the host minted for this module; borrowed and valid
 *        for the module's lifetime
 * @param module_id borrowed; copied, because the payload it comes from dies
 *        when activate() returns
 */
void AdoptHostApi(const GFHostApi* host, const char* module_id);

/**
 * @brief This module's SDK context, or nullptr before activation.
 *
 * The storage is a function-local static and is never destroyed. That is
 * deliberate: a module may still be running code on a thread it failed to
 * stop, and freeing this would turn a refusable call into a read of freed
 * memory. Nothing is reclaimed at teardown; the host revokes the grant
 * instead, and every primitive then refuses.
 *
 * Safe to call from any thread: after activation it is a plain load of a
 * pointer that never changes again.
 */
auto SdkContext() -> GFSDKContext*;

/**
 * @brief Copy a host bootstrap payload into owned storage.
 *
 * Tolerates @p info being null, which is what an older host passes -- the
 * facts are then unverified, and activation is refused. Reads no field the
 * payload's struct_size does not cover, so a host older than a field is
 * detected rather than trusted.
 *
 * @param info the borrowed payload, or nullptr
 * @param fallback_id identity to use when the payload carries none
 * @param fallback_version version to use when the payload carries none
 * @param fallback_context translation context compiled into the module
 * @return the owned copy
 */
auto AdoptBootstrapInfo(const GFModuleBootstrapInfo* info,
                        const char* fallback_id, const char* fallback_version,
                        const char* fallback_context) -> RuntimeFacts;

/// Whether a host table is usable: non-null, and big enough for every group
/// the runtime reaches into. The module side never checked this before.
auto HostApiIsUsable(const GFHostApi* host) -> bool;

}  // namespace gf::runtime
