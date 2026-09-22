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
#include <cstdint>

#include "GFSDKHostApi.h"

/**
 * @file GFHostContext.h
 * @brief Minting one module's host api table, and deciding what it may do.
 *
 * PRIVATE. Not installed, not includable by a module, and deliberately not
 * reachable by name from one: the whole point of a per-module table is that
 * there is no way to ask for a bigger one. gf_core reaches these two
 * functions only through ModuleSdkBridge, which carries them as type-erased
 * pointers.
 */
namespace gf_sdk_internal {

/**
 * @brief Create the table @p module_id will be handed at activation.
 *
 * Minting a module that is still live with the same grant returns the table
 * it already holds. Otherwise the module gets a new table and context, and
 * any previous context for the id is revoked; a table already handed out is
 * never rewritten.
 *
 * @param module_id borrowed; copied into the record
 * @param granted GF_HOST_CAP_* bits. A group whose bit is clear is NULL in
 *        the table AND unknown to the capability check, so reaching its
 *        primitives some other way does not help.
 * @return borrowed; valid until ReleaseHostApi() for the same id
 */
auto MintHostApi(const char* module_id, uint32_t granted) -> const GFHostApi*;

/// Revoke @p module_id's context. Calls arriving afterwards are refused on
/// every thread; calls already past the gate keep running, see
/// WaitHostApiIdle().
void ReleaseHostApi(const char* module_id);

/**
 * @brief Wait until no gated call is running with any of @p module_id's
 *        contexts, other than calls on the waiting thread itself.
 *
 * Revoking a context stops new calls; this waits out the ones already inside.
 * Only after both is it safe to free what those calls may be using, such as
 * the module's outstanding handles.
 *
 * @return false on timeout, in which case the caller must not free anything
 *         the running calls could still reach
 */
auto WaitHostApiIdle(const char* module_id, int timeout_ms) -> bool;

/**
 * @brief Why a context was accepted or refused.
 *
 * The distinction that matters is the last two: a module that has been torn
 * down and one that simply never had the capability are different situations
 * and a log line should say which. It exists here, on the host side, because
 * that is where the authoritative answer lives; the SDK deliberately keeps no
 * retirement flag of its own, since a field written at teardown and read from
 * worker threads is a race and an atomic one would only be a faster route to
 * this same answer.
 */
enum class HostContextStatus {
  kOK,       ///< live, and holds the capability asked about
  kUNKNOWN,  ///< never minted here, or invented; nothing was dereferenced
  kREVOKED,  ///< minted, and released when its module unloaded
  kDENIED,   ///< live, but its module was not granted this capability
};

/// @brief The status of @p ctx with respect to @p capability. No logging.
auto ContextStatusOf(GFHostContextRef ctx, uint32_t capability)
    -> HostContextStatus;

/// The module @p ctx belongs to, live or revoked, or an empty string when
/// @p ctx is unknown. For log lines and key scoping, never for authorization.
auto ContextModuleId(GFHostContextRef ctx) -> QString;

/**
 * @brief One authorized call, from BeginCall() to EndCall().
 *
 * Empty (null @ref record) when the call was refused.
 */
struct CallTicket {
  const void* record = nullptr;
  /// The module id, stable for the life of the process, so
  /// ScopedContextAttribution can use it without copying.
  const char* attribution = nullptr;
};

/**
 * @brief Authorize a call and count it as running, under one lock.
 *
 * The one authorization decision in the SDK. @p ctx is validated against the
 * registry BEFORE anything is read through it, so an unknown, stale or
 * invented pointer is refused rather than dereferenced. A refusal is logged.
 *
 * Correct on any thread, which is the reason the context travels in an
 * argument at all: a module calls the SDK from threads it started itself,
 * where the host's thread-local record of "whose code is running" does not
 * exist.
 *
 * Doing the check, the attribution lookup and the in-flight count together is
 * what stops a release from landing between them.
 *
 * @param ctx the context from the module's own GFHostApi
 * @param capability one GF_HOST_CAP_* bit, or 0 to require only that @p ctx
 *        is live, which is all the always-granted groups need
 * @param entry_point the SDK name to blame in the log
 */
auto BeginCall(GFHostContextRef ctx, uint32_t capability,
               const char* entry_point) -> CallTicket;

/// End a call BeginCall() authorized. A no-op for an empty ticket.
void EndCall(const CallTicket& ticket);

/**
 * @brief Point @p table's group pointers at the real thunk tables.
 *
 * Implemented in GFHostApiTables.cpp, beside the thunks themselves, so that
 * "which groups exist" and "what is in them" cannot drift apart. A group
 * whose bit is clear in @p granted is left NULL.
 */
void FillHostApiGroups(GFHostApi& table, uint32_t granted);

/**
 * @brief Attribute this thread to @p stable_id for the duration of a scope.
 *
 * Unlike GFSdkEnterModule(), this does NOT push onto the thread's attribution
 * stack: @p stable_id must outlive the scope, which for a context record it
 * does. That matters because the group thunks bracket EVERY call, including
 * the ones a module makes in a tight loop on its own thread, and a stack that
 * only ever grows would make attribution a memory leak proportional to how
 * much work the module does.
 */
class ScopedContextAttribution {
 public:
  explicit ScopedContextAttribution(const char* stable_id);
  ~ScopedContextAttribution();

  ScopedContextAttribution(const ScopedContextAttribution&) = delete;
  auto operator=(const ScopedContextAttribution&)
      -> ScopedContextAttribution& = delete;

 private:
  const char* previous_;
};

}  // namespace gf_sdk_internal
