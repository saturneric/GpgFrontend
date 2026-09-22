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

#include <stddef.h>
#include <stdint.h>

#include "GFSDKHostApi.h"

/**
 * @file GFSDKContext.h
 * @brief What a module hands back to the SDK on every call that needs the host.
 *
 * ## The rule this type exists to enforce
 *
 * The SDK is stateless. There is no global "current module", no thread-local
 * attribution and no bound table anywhere in `gf_sdk`; every public function
 * that needs the host takes one of these as its first argument, and every
 * public function that does not need the host takes none.
 *
 * That is not tidiness. A module does most of its work on threads it started
 * itself, and an SDK that had to look up "who is calling" would be right
 * inside an event handler and wrong in the worker that handler spawned. An
 * argument is the same on every thread.
 *
 * ## One authoritative member
 *
 * @ref host is the table the host minted for this module, and it is the only
 * thing here that decides anything. The authorization token is read from it,
 * `host->context`, at the moment of the call:
 *
 * @code
 *   ctx->host->pgp->inspect(ctx->host->context, in, &json);
 * @endcode
 *
 * An earlier draft carried the token as a second member. Two fields that must
 * agree are two fields that can disagree, and a context holding one module's
 * token beside another's table would have been a shape the type permits and
 * nothing rejects. Reading the token from the table it is about to be used
 * with makes that pair unconstructible.
 *
 * ## Who owns it
 *
 * `gf_module_runtime` builds exactly one of these per module instance, in
 * `activate()`, before any hook can run. `gf_sdk` only ever reads it. The
 * storage is never freed: it is abandoned at teardown rather than destroyed,
 * because a module may still be running code on a thread it failed to stop
 * and freeing this would turn a refusable call into a read of freed memory.
 * A few dozen bytes per module, bounded by how many modules a process loads.
 *
 * ## Revocation is the host's business
 *
 * There is deliberately no "retired" flag here. A field written at teardown
 * and read from worker threads is a race, and an atomic one would only be a
 * faster route to an answer the host already gives: after the host releases a
 * module's grant, every primitive refuses the context with a status that says
 * it was revoked rather than merely denied. The host registry is the single
 * authority on whether a call may proceed.
 *
 * ## Growth
 *
 * Begins with `struct_size`, written by whichever side compiled it, and grows
 * by APPENDING only.
 */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct GFSDKContext {
  size_t struct_size;   /**< sizeof as the RUNTIME compiled it */
  uint32_t abi_version; /**< the ABI generation the runtime was built for */
  uint32_t reserved;    /**< keeps `host` aligned; must be 0 */

  /** The capability table minted for this module. The only member any
   *  decision is made from. A group the module was not granted is NULL. */
  const GFHostApi* host;

  /** DIAGNOSTIC ONLY. Never an input to an authorization decision on either
   *  side of the boundary: the host identifies the caller from the token it
   *  validates, not from anything the module could edit. It is here so a log
   *  line can name the module without a lookup. */
  const char* module_id;
} GFSDKContext;

#ifdef __cplusplus
}
#endif
