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

#include "core/model/KeyDatabaseInfo.h"
#include "core/struct/settings_object/KeyDatabaseItemSO.h"
#include "core/typedef/CoreTypedef.h"

namespace GpgFrontend {

/**
 * @brief A managed key database whose folder has to move, because it was
 * renamed and its name and folder are one fact.
 *
 * Carried out by ApplyKeyDatabaseRuntime() rather than when the rename is
 * typed: the folder can only move once nothing has it open, and only the
 * apply step can wait for that.
 */
struct KeyDatabaseMove {
  QString database_id;
  QString old_name;         ///< the name before the rename, for recovery
  QString old_stored_path;  ///< the stored spelling before, for recovery
  QString from;             ///< the directory now
  QString to;               ///< the directory it moves to
};

/**
 * @brief Say that @p moves are saved in the stored list but not yet made.
 *
 * Between the two, the stored list names folders that are not there yet and
 * the folders there carry names no entry has. Anything that reads the list
 * back from the filesystem (the macOS app sandbox) asks this, so it neither
 * drops the renamed entries nor gives the folders new identities.
 *
 * @param moves the moves about to be applied
 */
void GF_CORE_EXPORT
NotePendingKeyDatabaseMoves(const QList<KeyDatabaseMove>& moves);

/**
 * @brief The move of @p database_id was made or given up.
 */
void GF_CORE_EXPORT ForgetPendingKeyDatabaseMove(const QString& database_id);

/**
 * @brief The moves noted and not yet made.
 */
auto GF_CORE_EXPORT PendingKeyDatabaseMoves() -> QList<KeyDatabaseMove>;

/**
 * @brief Read a scan of the folders as it will be once @p moves are made.
 *
 * A scanned folder a pending move takes away is given the name and path it
 * moves to. Pure, so the rule can be asserted without a sandbox.
 *
 * @param discovered the folders found, as name and path
 * @param moves the pending moves
 * @return the scan, with every pending move applied
 */
auto GF_CORE_EXPORT ApplyPendingMovesToScan(QList<KeyDatabaseItemSO> discovered,
                                            const QList<KeyDatabaseMove>& moves)
    -> QList<KeyDatabaseItemSO>;

/**
 * @brief What applying the stored key database configuration did.
 */
struct KeyDatabaseApplyResult {
  bool published{false};    ///< a new Runtime is in use
  int from_generation{0};   ///< the Runtime it replaced
  int to_generation{0};     ///< the Runtime now in use, when published
  QStringList unavailable;  ///< databases that did not open
  QStringList errors;       ///< what went wrong, for the user
};

/**
 * @brief What every context of a Runtime is built with.
 */
struct KeyDatabaseRuntimeOptions {
  bool offline_mode{false};
  bool auto_import_missing_key{false};
  QString default_engine;  ///< `basic/default_engine`, upper-cased
  bool gnupg_supported{false};
  bool rpgp_supported{false};
};

/**
 * @brief A Runtime built off to the side and not yet in use.
 */
struct KeyDatabaseRuntimeCandidate {
  bool ok{false};          ///< its default database opened
  int generation{0};       ///< the generation its handles belong to
  int default_handle{-1};  ///< its default database's handle
};

/**
 * @brief Build a Runtime from @p key_dbs on fresh handles of a fresh
 * generation, without putting it in use.
 *
 * Every database gets a new context; one on a folder that is already open
 * shares that folder's database connection and agent. Nothing the current
 * Runtime uses is replaced, and nothing is published to the register table.
 * A candidate that is not published is discarded with
 * RetireRuntimeGeneration().
 *
 * @param key_dbs the databases, in list order
 * @param default_index which of them is the default
 * @param options what every context is built with
 * @param result where failures are reported
 * @return the candidate
 */
auto GF_CORE_EXPORT PrepareKeyDatabaseRuntime(
    const QList<KeyDatabaseInfo>& key_dbs, int default_index,
    const KeyDatabaseRuntimeOptions& options, KeyDatabaseApplyResult& result)
    -> KeyDatabaseRuntimeCandidate;

/**
 * @brief Bring the stored key database configuration into use, without a
 * restart.
 *
 * In order:
 *  1. each move is carried out once nothing has its folder open: admission to
 *     the folder is closed across every live Runtime, work already running on
 *     it is waited for, its session is closed, and only then does it move.
 *     A move that cannot be made is undone in the stored list, so the
 *     database comes back where it was;
 *  2. a new Runtime is built off to the side on handles no one has seen,
 *     reusing every open database and agent it can;
 *  3. it is published in one step, unless the stored configuration changed
 *     while it was being built, in which case it is discarded and built again
 *     from the newer one;
 *  4. the old Runtime is retired. Work it already accepted runs to the end on
 *     it; it is reclaimed once the last of that work lets go.
 *
 * When no new Runtime can be built, the databases a move closed are opened
 * again in the Runtime in use, from where the stored list now puts them.
 *
 * One apply runs at a time; a second waits for the first. Blocks for as long
 * as building takes, so it is run off the GUI thread.
 *
 * @param moves folders to move first
 * @return what happened
 */
auto GF_CORE_EXPORT ApplyKeyDatabaseRuntime(
    const QList<KeyDatabaseMove>& moves = {}) -> KeyDatabaseApplyResult;

/**
 * @brief Install what reclaims a retired channel once nothing holds it.
 *
 * Reclamation takes the channel's engine objects out of reach on the GUI
 * thread, the one every widget reads them from, so it never lands while a
 * widget is in the middle of using one; they are then destroyed on a thread
 * of their own, since closing a database and stopping its agent take seconds.
 */
void GF_CORE_EXPORT InstallKeyDatabaseRuntimeReclaimer();

}  // namespace GpgFrontend
