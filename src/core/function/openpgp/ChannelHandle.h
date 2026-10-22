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
#include <functional>
#include <memory>
#include <optional>

#include "core/typedef/CoreTypedef.h"

namespace GpgFrontend {

/**
 * @brief What a channel handle currently names.
 *
 * A channel handle is the number a key database is addressed by for one run of
 * its context. Unlike the database's id it is not persistent; unlike a plain
 * index it is never handed to a different database once issued, so a caller
 * holding a stale one is told so instead of being quietly served another
 * database's keys.
 */
enum class ChannelHandleState : std::uint8_t {
  kUNKNOWN,      ///< never issued: no context was ever built for it
  kAVAILABLE,    ///< its context opened and serves operations
  kUNAVAILABLE,  ///< issued to a database whose context failed to open
  kRETIRED,      ///< issued once, and no longer in use; never reissued
};

/**
 * @brief Everything the registry knows about one handle.
 */
struct ChannelHandleInfo {
  int handle{-1};
  QString database_id;  ///< empty for a context built without one (tests)
  int generation{0};    ///< which Runtime issued it; 0 when unknown
  ChannelHandleState state{ChannelHandleState::kUNKNOWN};

  QString name;           ///< the database's name when the handle was issued
  QString path;           ///< the directory it opens
  QString backend_type;   ///< "gnupg" or "rpgp"
  int order{0};           ///< its place in the list, for showing in order
  int leases{0};          ///< how many holders keep it from being reclaimed
  int operations{0};      ///< how many of those are operations running on it
  bool reclaimed{false};  ///< its objects have been destroyed
};

/// The generation every handle issued at startup belongs to. Later Runtimes
/// issue handles of their own; until then there is only this one.
constexpr int kInitialRuntimeGeneration = 1;

/**
 * @brief Record that @p handle names the database @p database_id.
 *
 * Called when a context is built on the handle, whether or not it opened. A
 * handle may be registered again for the same database -- a context rebuilt on
 * it -- but never for a different one, and never after it was retired: that is
 * exactly the reuse that would let a stale caller reach the wrong database.
 *
 * @param handle the channel the context was built on
 * @param database_id the database it opens, possibly empty
 * @param available whether the context opened
 * @param generation the Runtime that issued the handle
 * @return false when the registration was refused
 */
auto GF_CORE_EXPORT
RegisterChannelHandle(int handle, const QString& database_id, bool available,
                      int generation = kInitialRuntimeGeneration) -> bool;

/**
 * @brief Take @p handle out of use for good.
 *
 * @param handle a registered handle
 * @return false when the handle was never registered
 */
auto GF_CORE_EXPORT RetireChannelHandle(int handle) -> bool;

/**
 * @brief What @p handle names, if anything.
 *
 * @param handle any number
 * @return its record; state kUNKNOWN when it was never registered
 */
auto GF_CORE_EXPORT LookupChannelHandle(int handle) -> ChannelHandleInfo;

/**
 * @brief Whether an operation may be run on @p handle right now.
 *
 * @param handle any number
 * @return true only for a handle whose context opened and is still in use
 */
auto GF_CORE_EXPORT IsLiveChannelHandle(int handle) -> bool;

/**
 * @brief Whether @p handle was taken out of use.
 *
 * Its objects are reclaimed once nothing holds it, and looking them up after
 * that only builds empty stand-ins. Something showing what is in a database
 * asks this before reading from it, rather than IsLiveChannelHandle(): a
 * handle still being opened at startup is not live yet, but not stale either.
 *
 * @param handle any number
 */
auto GF_CORE_EXPORT IsRetiredChannelHandle(int handle) -> bool;

/**
 * @brief A handle no database has ever been given.
 *
 * Above every handle registered so far, so a handle issued from here cannot
 * collide with one a stale caller may still hold.
 *
 * @return an unused handle
 */
auto GF_CORE_EXPORT AllocateChannelHandle() -> int;

/**
 * @brief Record which handle the default database was opened on.
 *
 * @param handle the default database's handle
 */
void GF_CORE_EXPORT SetDefaultChannelHandle(int handle);

/**
 * @brief The handle of the default database.
 *
 * Asked for rather than assumed to be 0: which database is the default is a
 * choice stored with the list, and where it was opened is whatever handle it
 * was given. Until one is recorded -- a test building its own contexts, say --
 * this is kGpgFrontendDefaultChannel, as it always was.
 *
 * @return the default database's handle
 */
auto GF_CORE_EXPORT DefaultChannelHandle() -> int;

/**
 * @brief A handle to start showing a key database on.
 *
 * The default database's, while it is open. While a folder move has it retired
 * -- or when it failed to open -- the first open database of the Runtime in
 * use, so a view opened meanwhile does not start out on a handle whose objects
 * are being reclaimed. DefaultChannelHandle() when nothing is open at all.
 */
auto GF_CORE_EXPORT UsableDefaultChannelHandle() -> int;

/**
 * @brief Record what a handle opens, for listings.
 */
void GF_CORE_EXPORT DescribeChannelHandle(int handle, const QString& name,
                                          const QString& path,
                                          const QString& backend_type,
                                          int order);

/**
 * @brief Why a handle could not be used.
 */
enum class EngineAccessError : std::uint8_t {
  kUNKNOWN,      ///< never issued
  kUNAVAILABLE,  ///< its database failed to open
  kRETIRED,      ///< replaced by a newer Runtime; never resolves again
};

/**
 * @brief Holding one of these keeps a channel handle's objects alive.
 *
 * A retired handle is reclaimed -- its context and every engine object on it
 * destroyed -- only once the last lease on it is gone. Copies share one
 * holding; the handle is released when the last copy goes.
 *
 * What a lease protects is the handle's lifetime, nothing more: it does not
 * serialise access, and it does not keep a retired handle usable for new work.
 */
class GF_CORE_EXPORT EngineLease {
 public:
  EngineLease() = default;

  /**
   * @brief The handle held, or -1 for an empty lease.
   */
  [[nodiscard]] auto Handle() const -> int;

  /**
   * @brief Whether this lease holds a handle.
   */
  [[nodiscard]] auto IsHeld() const -> bool;

  /**
   * @brief Let go early. Equivalent to destroying this copy.
   */
  void Release();

  struct Token;

 private:
  friend auto TryAcquireEngineLease(int handle, EngineAccessError* why,
                                    bool operation)
      -> std::optional<EngineLease>;

  std::shared_ptr<Token> token_;
};

/**
 * @brief Take a lease on @p handle, for new work on it.
 *
 * Granted only for a live handle of any Runtime that has not been retired.
 * Checked and counted under the same lock that retires handles, so no lease is
 * ever granted on a handle being retired at the same moment.
 *
 * @param handle any number
 * @param why set to the reason when refused, when not null
 * @param operation whether it is held by an operation running on the handle,
 * rather than by something that only shows what is in it; counted apart, so
 * a change can say what work it would leave finishing on the old database
 * @return the lease, or nothing when refused
 */
auto GF_CORE_EXPORT TryAcquireEngineLease(int handle,
                                          EngineAccessError* why = nullptr,
                                          bool operation = false)
    -> std::optional<EngineLease>;

/**
 * @brief Set what reclaims a retired handle once nothing holds it.
 *
 * Called once per handle, from whichever thread let go of the last lease, or
 * from the one that retired it when nothing held it at that moment. Expected to
 * hand the work to a thread of its own choosing; it runs outside every
 * registry lock.
 *
 * @param reclaimer the reclaimer, or an empty function to stop reclaiming
 * @return the reclaimer it replaces
 */
auto GF_CORE_EXPORT
SetRetiredChannelReclaimer(std::function<void(int handle)> reclaimer)
    -> std::function<void(int handle)>;

/**
 * @brief The Runtime generation new work currently resolves against.
 */
auto GF_CORE_EXPORT CurrentRuntimeGeneration() -> int;

/**
 * @brief Make @p generation the current Runtime.
 *
 * In one step under the registry lock: every handle of the previous current
 * generation is retired, @p generation becomes current, and @p default_handle
 * becomes the default. A lease asked for before this step was granted on the
 * old handle and keeps it alive; one asked for after is refused on it. No new
 * work can resolve onto the old Runtime once this returns.
 *
 * @param generation the generation whose handles are now in use
 * @param default_handle that generation's default database handle
 * @return the handles that were retired
 */
auto GF_CORE_EXPORT PublishRuntimeGeneration(int generation, int default_handle)
    -> QContainer<int>;

/**
 * @brief Retire every handle of @p generation, without changing which one is
 * current.
 *
 * What PublishRuntimeGeneration() does to the previous Runtime, on its own: for
 * a candidate Runtime that is discarded rather than published.
 *
 * @param generation the generation to retire
 * @return the handles that were retired
 */
auto GF_CORE_EXPORT RetireRuntimeGeneration(int generation) -> QContainer<int>;

/**
 * @brief A generation number no Runtime has used yet.
 */
auto GF_CORE_EXPORT AllocateRuntimeGeneration() -> int;

/**
 * @brief The handles of one generation, in list order.
 *
 * @param generation the generation; the current one when negative
 * @return every handle issued for it, available or not
 */
auto GF_CORE_EXPORT ListRuntimeChannels(int generation = -1)
    -> QContainer<ChannelHandleInfo>;

/**
 * @brief The current Runtime's handle for the database @p database_id.
 *
 * @param database_id a database id; empty for the default database
 * @return its live handle, or -1 when the current Runtime has none
 */
auto GF_CORE_EXPORT ResolveDatabaseHandle(const QString& database_id) -> int;

/**
 * @brief Rewrite the register table's `gpgme.ctx.list` from the current
 * Runtime.
 *
 * Removes every entry first, so a retired handle does not linger in what
 * modules read, then writes one per database that opened.
 */
void GF_CORE_EXPORT PublishRuntimeToRegisterTable();

/**
 * @brief Record that a retired handle's objects have been destroyed.
 */
void GF_CORE_EXPORT MarkChannelReclaimed(int handle);

/**
 * @brief Put a reclaimed handle up for reclamation once more.
 *
 * For an object a stale caller rebuilt on a handle whose objects were already
 * destroyed. Nothing happens for a handle that is not retired, still leased,
 * or not reclaimed yet -- a pending reclamation picks the new object up.
 *
 * @param handle any number
 * @return true when the handle was put up again
 */
auto GF_CORE_EXPORT ReclaimChannelAgainIfReclaimed(int handle) -> bool;

/**
 * @brief Retire every handle, of any generation, opening the directory at
 * @p path.
 *
 * Closes admission for the resource: no new work resolves onto it, while work
 * already holding a lease runs on and keeps it alive.
 *
 * @param path a key database directory
 * @return the handles that were still in use and are now retired
 */
auto GF_CORE_EXPORT RetireChannelsAtLocation(const QString& path)
    -> QContainer<int>;

/**
 * @brief Whether anything at @p path is still open or not yet reclaimed.
 *
 * True while a handle on that directory has not been reclaimed, or its shared
 * database connection is still open.
 *
 * @param path a key database directory
 */
auto GF_CORE_EXPORT IsKeyDatabaseLocationInUse(const QString& path) -> bool;

}  // namespace GpgFrontend
