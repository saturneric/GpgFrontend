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

#include <cstdint>

#include "GFCoreExport.h"

namespace GpgFrontend::Module {

/**
 * @file ModuleEventRegistry.h
 * @brief What every event the Host fires MEANS, written down once.
 *
 * ## Why a registry and not a list of strings
 *
 * An event id used to be a string literal at a trigger site and a matching
 * string literal in a module. Everything else about the event -- whether the
 * Host reads the reply, whether a module may answer later from another
 * thread, whether the module is being informed or being asked to do the work
 * -- could only be learned by reading the call site. Two of those facts are
 * load-bearing:
 *
 *   - `TAB_ACTIVATED` reports a fact and ignores the reply. A module that
 *     "returns" something there is talking to nobody.
 *   - `EDIT_TAB_TYPE_EMAIL_OP_DECRYPT` has its reply written straight into
 *     the user's document. A module that gets that wrong changes what the
 *     user is looking at.
 *
 * Those are not the same kind of event and they never were; the difference
 * simply had nowhere to live.
 *
 * ## What it is used for
 *
 * Three things, all of which used to be impossible:
 *
 *   1. Host-side subscription control. `ModuleManager::ListenEvent`
 *      refuses an id no registry entry describes, so a typo is a refusal
 *      naming the module rather than a subscription that never fires.
 *   2. Saying which events may influence the Host. @ref kEXTEND is a small,
 *      deliberate list. Everything else is observation.
 *   3. Documentation that cannot rot, because the Host dispatches through it.
 */

/// Whether a module's reply may change what the Host does.
enum class ModuleEventSemantics {
  /// The module is being told. Its reply is not read, and a Host that starts
  /// reading one has changed the contract and must say so here.
  kOBSERVE,
  /// A deliberate extension point: the reply, or the module's action on a
  /// borrowed object, changes the outcome.
  kEXTEND,
};

/// Which layer owns the event and fires it.
enum class ModuleEventLayer { kCORE, kUI };

/// The id is a PATTERN with a `<...>` placeholder rather than a literal.
#define GF_EVENT_PATTERN (1U << 0)
/// The trigger site actually reads the module's reply.
#define GF_EVENT_REPLY_CONSUMED (1U << 1)
/// A handler may return kDEFERRED and answer later, possibly from another
/// thread. Not universal: a trigger whose caller has already moved on cannot
/// use a late answer.
#define GF_EVENT_DEFERRABLE (1U << 2)
// Bit 3 was GF_EVENT_GUI_HANDLES: an event that lent a module Host GUI
// objects. No event does any more, and the bit is not reused.

/**
 * @brief One event's contract.
 *
 * Grows by appending, like everything else that describes a boundary.
 */
struct GF_CORE_EXPORT ModuleEventSpec {
  const char* id;
  ModuleEventLayer layer;
  ModuleEventSemantics semantics;
  uint32_t flags;
  const char* summary;  ///< one line, for the controller UI and the docs
};

/// Every event this Host build fires, in id order.
auto GF_CORE_EXPORT ModuleEventCatalog() -> const QList<ModuleEventSpec>&;

/**
 * @brief The contract for @p event_id, matching patterns too.
 *
 * @return nullptr when nothing describes it, which is a refusal rather than
 *         a permissive default: an id the Host never fires is a subscription
 *         that would silently never arrive.
 */
auto GF_CORE_EXPORT FindModuleEventSpec(const QString& event_id)
    -> const ModuleEventSpec*;

/// Whether @p event_id is one this Host build knows how to fire.
auto GF_CORE_EXPORT IsKnownModuleEvent(const QString& event_id) -> bool;

/// Whether a module's reply to @p event_id may change Host behaviour.
auto GF_CORE_EXPORT IsModuleEventExtensionPoint(const QString& event_id)
    -> bool;

}  // namespace GpgFrontend::Module
