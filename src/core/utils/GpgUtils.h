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

#include <optional>

#include "core/GFCoreRust.h"
#include "core/function/openpgp/OpenPGPContext.h"
#include "core/model/GpgAbstractKey.h"
#include "core/model/KeyDatabaseInfo.h"
#include "core/struct/settings_object/KeyDatabaseItemSO.h"
#include "core/typedef/CoreTypedef.h"
#include "core/typedef/GpgTypedef.h"

namespace GpgFrontend {

/**
 * @brief Log a warning if @p err is non-zero and return the error code.
 *
 * Strips the gpgme error source and returns only the error code portion.
 *
 * @param err gpgme error value
 * @return gpg_err_code_t error code (GPG_ERR_NO_ERROR on success)
 */
auto GF_CORE_EXPORT CheckGpgError(GpgError err) -> GpgError;

/**
 * @brief Log a warning with the error and an additional comment if @p err is
 * non-zero.
 *
 * @param gpgmeError gpgme error value
 * @param comment additional context string included in the log message
 * @return the original @p gpgmeError value unchanged
 */
auto GF_CORE_EXPORT CheckGpgError(GpgError gpgmeError, const QString& comment)
    -> GpgError;

/**
 * @brief Compare @p err against @p predict and log a warning if they differ.
 *
 * @param err gpgme error value to check
 * @param predict expected error code (default: GPG_ERR_NO_ERROR)
 * @return error code portion of @p err
 */
auto GF_CORE_EXPORT CheckGpgError2ErrCode(
    gpgme_error_t err, gpgme_error_t predict = GPG_ERR_NO_ERROR)
    -> gpg_err_code_t;

/**
 * @brief Return a (source, description) pair describing the given error.
 *
 * @param err gpgme error value
 * @return GpgErrorDesc with the error source name and human-readable
 * description
 */
auto GF_CORE_EXPORT DescribeGpgErrCode(GpgError err) -> GpgErrorDesc;

/**
 * @brief Return whether the given text contains PGP signature markers.
 *
 * @param text byte array reference to inspect
 * @return non-zero if a PGP signature is detected, 0 otherwise
 */
auto GF_CORE_EXPORT TextIsSigned(BypeArrayRef text) -> int;

/**
 * @brief Return the output file path with an extension appropriate for the
 * given operation.
 *
 * @param path input file path to derive the output path from
 * @param opera GPG operation type (encrypt, sign, etc.)
 * @param ascii true if ASCII-armored output is requested
 * @return output file path with the correct extension
 */
auto GF_CORE_EXPORT SetExtensionOfOutputFile(const QString& path,
                                             GpgOperation opera, bool ascii)
    -> QString;

/**
 * @brief Return the output file path with an archive-operation extension.
 *
 * @param path input file path
 * @param opera GPG operation type
 * @param ascii true if ASCII-armored output is requested
 * @return output file path with the correct archive extension
 */
auto GF_CORE_EXPORT SetExtensionOfOutputFileForArchive(const QString& path,
                                                       GpgOperation opera,
                                                       bool ascii) -> QString;

/**
 * @brief Resolve and canonicalize a key database path relative to @p app_path.
 *
 * @param app_path base application directory
 * @param path raw path from configuration
 * @return canonical absolute path to the key database directory
 */
/// Marks a key database path as relative to the profile that owns it. An
/// explicit token rather than a plain relative path because the existing
/// relative form is anchored to the executable directory, which is exactly what
/// a relocatable profile cannot depend on — and redefining it would silently
/// move every existing portable installation's keyring.
constexpr auto kProfilePathToken = "@profile";

/// The name of the channel-0 database every profile derives rather than stores.
/// Spelled once because it is what tells the derived database apart from one
/// the user made, and `basic/default_engine` is a statement about this one.
constexpr auto kDefaultKeyDatabaseName = "DEFAULT";

/**
 * @brief Whether a name belongs to the application rather than to the user.
 *
 * The DEFAULT database is derived, not stored: its path comes from the engine
 * and it is the one database `basic/default_engine` speaks for. A user-made
 * database wearing the same name is therefore not a duplicate to be tolerated
 * but a second thing answering to an identity only one thing may have -- it
 * would be found first by every lookup that goes by name, and it would take the
 * default engine along with the identity.
 *
 * Compared case-insensitively and after trimming, because the user is typing a
 * name that becomes a directory: "default" and "DEFAULT " are the same folder
 * on Windows and on macOS, and reserving only the exact spelling would reserve
 * nothing there.
 *
 * @param name a key database name as typed
 * @return true when the name is the application's to use
 */
auto GF_CORE_EXPORT IsReservedKeyDatabaseName(const QString& name) -> bool;

/**
 * @brief The DEFAULT key database as this computer's engine describes it.
 *
 * Derived, never stored: the path is whatever the OpenPGP engine already uses,
 * and the backend is whichever engine that is. A profile may hold one of these
 * and no more, and it is the only sanctioned way to obtain one -- the name is
 * reserved, so it cannot be typed.
 *
 * The reason it can be asked for at all: a package never carries a key database
 * that lived outside the profile it came from, and the DEFAULT one usually did.
 * A profile that arrives from another computer therefore has none, and needs a
 * way to adopt this computer's.
 *
 * Returns an entry whose path is empty when the engine names no database --
 * the one question this answers that MakeDefaultKeyDatabaseItem() may not, and
 * why the two are separate. Free of side effects, so a dialog may ask.
 *
 * @return the DEFAULT entry, with an empty path when there is none
 */
auto GF_CORE_EXPORT DefaultKeyDatabaseCandidate() -> KeyDatabaseItemSO;

/**
 * @brief Reduce a stored list to at most one DEFAULT database.
 *
 * The name is an identity, and only one thing may hold it: every lookup that
 * goes by name finds whichever comes first, and `basic/default_engine` speaks
 * for whichever that is. The dialog will not let a second one be made, but a
 * stored list is not only ever written by the dialog -- it can arrive in a
 * package, be hand-edited, or come from a build older than this rule.
 *
 * The first one in list order is kept, the rest are dropped. Only the settings
 * entry goes; the directory it named is left alone, so the keyring is still
 * there to be added back under a name of its own.
 *
 * Pure, and idempotent.
 *
 * @param databases the stored list
 * @return the list with any DEFAULT after the first removed, order preserved
 */
auto GF_CORE_EXPORT
DropDuplicateDefaultKeyDatabases(const QContainer<KeyDatabaseItemSO>& databases)
    -> QContainer<KeyDatabaseItemSO>;

/**
 * @brief Give every key database a unique, ascending channel.
 *
 * Sorted by the channel each entry already carries and then de-duplicated
 * upward, so the list keeps the order the profile means and the numbers become
 * usable. The sort is stable: entries can legitimately arrive sharing a channel
 * -- a list that has never been normalised, or one just given a DEFAULT at
 * channel 0 -- and which of them ends up as channel 0 has to be a fact about
 * the profile rather than something that changes between runs.
 *
 * @param key_dbs the list, renumbered and reordered in place
 */
void GF_CORE_EXPORT
NormalizeKeyDatabaseChannels(QContainer<KeyDatabaseItemSO>& key_dbs);

/**
 * @brief Point the DEFAULT entry at this computer's own default keyring.
 *
 * The DEFAULT database is derived, not stored: it names whatever keyring the
 * OpenPGP engine on *this* machine already uses. A stored path for it is
 * therefore a record of where that was, on whichever machine last wrote the
 * settings -- and a profile opened from a package was last written somewhere
 * else. "C:/Users/eric/AppData/Roaming/gnupg" is not a location Linux has, and
 * is not even a path Qt reads as absolute there.
 *
 * So the stored value is replaced rather than repaired. Repairing it is not
 * possible in any case: the sender's keyring never travelled, because a package
 * carries nothing from outside the profile root. What the recipient wants under
 * that name is their own default keyring, which is exactly what this supplies.
 *
 * The backend goes with the path, since which engine derived the keyring is
 * part of the same fact.
 *
 * Pure, so the local values are passed in rather than looked up. Entries that
 * are not the DEFAULT one are untouched, and a local default that is not there
 * -- an engine reporting none -- changes nothing.
 *
 * @param databases the stored list
 * @param local_path this computer's default keyring, empty if it has none
 * @param local_backend the engine that keyring belongs to
 * @return the list with the DEFAULT entry re-pointed, order preserved
 */
auto GF_CORE_EXPORT AdoptLocalDefaultKeyDatabase(
    const QContainer<KeyDatabaseItemSO>& databases, const QString& local_path,
    const QString& local_backend) -> QContainer<KeyDatabaseItemSO>;

/**
 * @brief Which of the three kinds a stored entry is.
 *
 * The reserved name decides first and on its own: the DEFAULT database is an
 * identity rather than a location, and AdoptLocalDefaultKeyDatabase() re-points
 * it at this computer's keyring on every read, so whatever path is stored for
 * it says nothing about what it is.
 *
 * Everything else is decided by where it lives. The path is tokenised against
 * the root first, so a database recorded the long way round is recognised as
 * the same thing as one recorded as `@profile/...` -- they are the same
 * directory, and which spelling reached the settings file is an accident of
 * which build wrote it. What remains is asked of IsManagedKeyDatabasePath(),
 * the one list that says which directories a package carries, so this cannot
 * drift from what RewriteKeyDatabaseListForPacking() actually packs.
 *
 * A path inside the profile but outside those directories -- under `workspace`,
 * say -- is external, and deliberately so: the package would not carry it, and
 * calling it managed here would promise a recipient a keyring that never
 * shipped.
 *
 * @param name the database's name
 * @param stored_path its path as stored, absolute or `@profile/...`
 * @param profile_root the root stored paths are read against
 * @return the kind it belongs to
 */
auto GF_CORE_EXPORT ClassifyKeyDatabase(const QString& name,
                                        const QString& stored_path,
                                        const QString& profile_root)
    -> KeyDatabaseKind;

/**
 * @brief Settle the kind of every entry that does not already carry one.
 *
 * The kind is recorded with the database from now on, because it is a statement
 * of what the user meant rather than a fact about where a directory currently
 * sits. A path can be re-anchored, moved, or arrive from another machine; what
 * the database *is* should survive all of that, and a package deciding what to
 * carry should be reading an intention rather than re-guessing one.
 *
 * An entry from a build before the field existed has none, and there is exactly
 * one honest thing to do with it: derive it from the path, which is what every
 * one of those builds did. That happens here, once, so no caller has to know
 * about the two vintages.
 *
 * The reserved name overrides whatever is recorded. Only one thing may answer
 * to it, and the code that acts on that -- DropDuplicateDefaultKeyDatabases(),
 * AdoptLocalDefaultKeyDatabase(), ChooseChannelZeroEngine() -- all go by the
 * name. A stored kind disagreeing with the name would split that agreement.
 *
 * Pure, and idempotent.
 *
 * @param databases the stored list
 * @param profile_root root to derive a missing kind against
 * @return the list with every kind settled, order preserved
 */
auto GF_CORE_EXPORT ResolveKeyDatabaseKinds(
    const QContainer<KeyDatabaseItemSO>& databases, const QString& profile_root)
    -> QContainer<KeyDatabaseItemSO>;

/**
 * @brief Where a managed key database of this name lives.
 *
 * The one place `<app-data>/dbs/<name>` is spelled. It was written out at each
 * site that needed it, which is how the dialog came to build the folder from
 * the trimmed name while storing the untrimmed one.
 *
 * The name is trimmed here rather than at the callers, so the folder a rename
 * moves to and the folder an add creates cannot disagree.
 *
 * @param app_data_path the profile's data directory
 * @param name the database name as typed
 * @return the directory that name denotes, empty when either input is empty
 */
auto GF_CORE_EXPORT ManagedKeyDatabasePath(const QString& app_data_path,
                                           const QString& name) -> QString;

/**
 * @brief Assemble the stored list from the three kinds, in the fixed order.
 *
 * DEFAULT first, then the managed databases, then the external ones, numbered
 * from zero as they land. The order is a rule rather than something the user
 * arranges, because channel 0 is not merely first: it is built synchronously at
 * startup, it is what the key list opens on, and `basic/default_engine` is a
 * statement about the DEFAULT database specifically. Letting an external
 * database drift into that position is how an engine setting meant for one
 * database came to be applied to another.
 *
 * Within each group the caller's order is kept -- that part is the user's.
 *
 * Pure, so the whole ordering rule can be tested without a settings file.
 *
 * @param default_db the DEFAULT database, absent when the user turned it off
 * @param managed the profile's own databases, in the order they should appear
 * @param external this computer's databases, in the order they should appear
 * @return one list, channels 0..n-1 in that order
 */
auto GF_CORE_EXPORT
ComposeKeyDatabaseList(const std::optional<KeyDatabaseItemSO>& default_db,
                       const QContainer<KeyDatabaseItemSO>& managed,
                       const QContainer<KeyDatabaseItemSO>& external)
    -> QContainer<KeyDatabaseItemSO>;

/// What renaming a managed database should do to its directory.
enum class ManagedRenameAction {
  kRENAME,           ///< move the directory to the new name
  kNOTHING_TO_MOVE,  ///< no directory there yet; the entry alone changes
  kTARGET_TAKEN,     ///< something already occupies the new name; refuse
};

/**
 * @brief Decide what a managed rename does on disk, before doing any of it.
 *
 * A managed database's name *is* its directory name -- the sandbox rescan reads
 * the list back out of the filesystem, so an entry whose name and folder
 * disagree is an entry that gets silently reverted. Renaming therefore has to
 * move the directory, and the interesting cases are the ones where it must not.
 *
 * kTARGET_TAKEN rather than a merge or an overwrite: whatever is already there
 * is a keyring, and the only safe thing to do with somebody's keyring is
 * nothing.
 *
 * Split out as a pure decision, like DecideKeyDatabasePathAction(), so the rule
 * is testable without a filesystem and the caller is left with the doing.
 *
 * @param old_exists the current directory is there
 * @param new_exists something already occupies the new directory
 * @return what the caller should do
 */
auto GF_CORE_EXPORT DecideManagedRename(bool old_exists, bool new_exists)
    -> ManagedRenameAction;

/**
 * @brief Record an absolute key database path relative to its profile.
 *
 * A path outside the profile is returned unchanged: it genuinely is somewhere
 * else, and pretending otherwise would fabricate a location.
 *
 * @param absolute_path the path as it exists on this machine
 * @param profile_root the profile that may contain it
 * @return the profile-relative form, or @p absolute_path unchanged
 */
auto GF_CORE_EXPORT ToProfileRelativeKeyDatabasePath(
    const QString& absolute_path, const QString& profile_root) -> QString;

/**
 * @brief Resolve a stored key database path against a profile root.
 *
 * Refuses anything that normalises outside the profile: a stored value can come
 * from a package written elsewhere, and "@profile/../.." would put a key
 * database outside the profile that is supposed to contain it.
 *
 * @param stored_path the value from settings or a manifest
 * @param profile_root the profile to resolve against
 * @return the absolute path, or empty when it escapes the profile
 */
auto GF_CORE_EXPORT FromProfileRelativeKeyDatabasePath(
    const QString& stored_path, const QString& profile_root) -> QString;

/**
 * @brief The profile-relative tail of a key database path written elsewhere.
 *
 * A stored path from another machine is not something this one can resolve, and
 * on a different platform it is not even recognisable as absolute: Qt reads
 * "C:/Users/..." as a relative path on Unix and "/Users/..." as an absolute one
 * on Windows, so both end up naming a directory that is not there. What is
 * still readable is the tail — the managed key database directory and whatever
 * the user called the database — and that tail is exactly what the local
 * profile holds.
 *
 * Only the tail is returned; whether the local profile really has it is the
 * caller's to check, and must be checked, because this says nothing about
 * where the path came from.
 *
 * Pure. Splits on both separators, since a Windows path reaching a POSIX build
 * is never cleaned into forward slashes by QDir.
 *
 * @param stored_path a key database path as stored, from anywhere
 * @return e.g. "dbs/Key DB 2", or empty when no component names a managed key
 * database directory or the path tries to climb out of one
 */
auto GF_CORE_EXPORT ForeignKeyDatabasePathTail(const QString& stored_path)
    -> QString;

/**
 * @brief Re-anchor a stored key database path to the profile that owns it now.
 *
 * Two cases, and both come down to a path written against a root that is no
 * longer the root. A profile that was moved, copied or unpacked somewhere else
 * keeps naming where it used to be; a profile that arrived in a package names
 * the sender's computer, on the sender's operating system, which the receiving
 * one cannot even recognise as absolute. Either way the keys are sitting in the
 * profile, under the name the user gave them, and only the prefix is wrong.
 *
 * Rewritten to the `@profile/` token rather than to today's absolute path, so
 * the next move needs no repair at all.
 *
 * Deliberately narrow: the recovery fires only when the stored path is not
 * there *and* the profile has the directory it names. A key database on a
 * volume that happens to be unmounted this morning keeps pointing at its
 * volume, and a database the user put somewhere by hand is never invented a
 * profile-local location for.
 *
 * @param stored_path the value from settings
 * @param profile_root the profile that is open, empty to leave it alone
 * @return the profile-relative form, or @p stored_path unchanged
 */
auto GF_CORE_EXPORT ReanchorKeyDatabasePath(const QString& stored_path,
                                            const QString& profile_root)
    -> QString;

auto GF_CORE_EXPORT GetCanonicalKeyDatabasePath(const QDir& app_path,
                                                const QString& path) -> QString;

/**
 * @brief Return all key database infos (both GPG and custom) from settings.
 *
 * @return list of KeyDatabaseInfo for every configured database
 */
auto GF_CORE_EXPORT GetAllKeyDatabaseInfoBySettings()
    -> QContainer<KeyDatabaseInfo>;

/**
 * @brief The key database list exactly as it is stored.
 *
 * A plain read: no healing, no re-anchoring, no channel renumbering, and
 * nothing written back. That matters because every other reader of this list
 * gets the reconciled form, and there was no way to ask what is actually on
 * disk -- which is the question a settings page has to answer before it offers
 * to overwrite it.
 *
 * @return the stored entries, in stored order, kinds unsettled where the
 * writing build had none
 */
auto GF_CORE_EXPORT LoadKeyDatabaseList() -> QContainer<KeyDatabaseItemSO>;

/**
 * @brief Make a stored list usable on this computer.
 *
 * The healing pass, in one place and in a fixed order: at most one DEFAULT, its
 * path and backend replaced by this computer's own, every kind settled, entries
 * naming nothing dropped, paths re-anchored to the profile that holds them now,
 * a fallback seeded if that leaves nothing, and channels renumbered.
 *
 * Every input is a parameter rather than something read here, so the whole pass
 * is exercisable without a profile session, a settings file or an engine --
 * which is what it was not, when it lived inside GetKeyDatabasesBySettings().
 * It is not pure: ReanchorKeyDatabasePath() asks the filesystem whether a
 * stored path is still there, because that is the question it answers.
 *
 * Idempotent, and it never writes. Persisting the result is
 * PersistKeyDatabaseList()'s job, and deliberately the caller's decision.
 *
 * @param stored the list as read, from LoadKeyDatabaseList() or a package
 * @param local_default this computer's DEFAULT database, from
 * DefaultKeyDatabaseCandidate(); an empty path leaves the stored one alone
 * @param fallback what to seed with when nothing survives; must name a real
 * database, so MakeDefaultKeyDatabaseItem() rather than the candidate
 * @param profile_root root to re-anchor against, empty to re-anchor nothing
 * @param app_data_path root that decides whether a path is managed
 * @return the usable list, channels 0..n-1, never empty
 */
auto GF_CORE_EXPORT ReconcileKeyDatabaseList(
    const QContainer<KeyDatabaseItemSO>& stored,
    const KeyDatabaseItemSO& local_default, const KeyDatabaseItemSO& fallback,
    const QString& profile_root, const QString& app_data_path)
    -> QContainer<KeyDatabaseItemSO>;

/**
 * @brief Write a key database list back to settings.
 *
 * Separate from the reconciliation that produces one so that a caller can heal
 * a list without committing to it, and so the one place that writes is
 * findable. The underlying SettingsObject still refuses to overwrite an object
 * that would not load, which is what keeps an unreadable list from being
 * replaced by the empty one a failed read falls back to.
 *
 * @param key_dbs the list to store
 */
void GF_CORE_EXPORT
PersistKeyDatabaseList(const QContainer<KeyDatabaseItemSO>& key_dbs);

/**
 * @brief The stored key database list, made usable, and written back.
 *
 * LoadKeyDatabaseList(), then ReconcileKeyDatabaseList(), then
 * PersistKeyDatabaseList(). The composition is kept because almost every caller
 * wants exactly this, and because the order is a fact about the program rather
 * than something a call site should restate.
 *
 * @note **Reading this writes.** The healed list goes back to settings before
 * it is returned, so a stored path that was re-anchored, a duplicate DEFAULT
 * that was dropped or a channel that was renumbered is committed by the act of
 * asking. Callers that must not write should use LoadKeyDatabaseList() and
 * reconcile for themselves.
 *
 * In the macOS app sandbox the filesystem is authoritative rather than the
 * settings, so the middle step is ReconcileSandboxKeyDatabaseList() instead.
 *
 * @return the usable list, channels 0..n-1, never empty
 */
auto GF_CORE_EXPORT GetKeyDatabasesBySettings()
    -> QContainer<KeyDatabaseItemSO>;

/**
 * @brief Reconcile the macOS app-sandbox key database list against the
 * filesystem.
 *
 * The filesystem scan (@p discovered) is authoritative for which user databases
 * exist and their paths; @p stored only supplies recoverable metadata (backend
 * type, channel/order) matched by name. The channel-0 @p default_db is always
 * kept. Backend types not present in @p supported_backends are replaced by a
 * supported default (gnupg when available, else rpgp).
 *
 * Exposed primarily for unit testing of the reconciliation logic.
 *
 * @param default_db the channel-0 DEFAULT database (path already resolved)
 * @param discovered name/path entries scanned from the fixed dbs/ directory
 * @param stored the persisted key database list (metadata source)
 * @param supported_backends backend types whose engine is available
 * @return the reconciled, channel-normalized key database list
 */
auto GF_CORE_EXPORT ReconcileSandboxKeyDatabaseList(
    KeyDatabaseItemSO default_db, QContainer<KeyDatabaseItemSO> discovered,
    const QContainer<KeyDatabaseItemSO>& stored,
    const QSet<QString>& supported_backends) -> QContainer<KeyDatabaseItemSO>;

/**
 * @brief How (or whether) to materialise a key database directory.
 */
enum class KeyDatabasePathAction : std::uint8_t {
  kUSE_AS_IS,    ///< the directory already exists
  kCREATE_LEAF,  ///< the parent exists; create the final component only
  kCREATE_FULL,  ///< under our own app data; create the whole chain
  kREJECT,       ///< a file is in the way, or the parent is gone off app data
};

/**
 * @brief Decide how to handle a key database directory that is not there.
 *
 * Recreating a leaf whose parent still exists recovers a database someone
 * deleted. Recreating a whole missing chain is a different matter: off our own
 * app data it almost always means an unmounted volume or a detached share, and
 * writing there would hand GnuPG a fresh empty keyring at the wrong location --
 * which the user reads as "the app lost all my keys" rather than "the database
 * is unavailable". So a full chain is only ever built inside app data.
 *
 * Exposed primarily for unit testing.
 *
 * @param exists_as_dir the target already exists and is a directory
 * @param exists_as_file the target exists but is not a directory
 * @param parent_exists the target's parent directory exists
 * @param inside_app_data the target lives under the application data path
 */
auto GF_CORE_EXPORT DecideKeyDatabasePathAction(bool exists_as_dir,
                                                bool exists_as_file,
                                                bool parent_exists,
                                                bool inside_app_data)
    -> KeyDatabasePathAction;

/**
 * @brief Keep only the databases that resolved to a real directory, re-seeding
 * the DEFAULT database when that leaves nothing behind.
 *
 * A stored list whose every entry has become invalid -- a removed directory, a
 * volume that is not mounted, a profile written by another installation -- used
 * to filter down to an empty list and abort startup outright. Falling back to a
 * freshly derived DEFAULT lets the application come up with an empty keyring
 * instead, which the user can actually do something about.
 *
 * The fallback is runtime-only and deliberately not persisted: an entry can be
 * invalid simply because a removable volume is absent right now, and rewriting
 * the stored list would turn a temporary condition into permanent
 * configuration loss.
 *
 * Exposed primarily for unit testing.
 *
 * @param all every configured database, valid or not, in order
 * @param fallback freshly derived DEFAULT entry; ignored when itself invalid
 * @return the valid subset in order, or just @p fallback, or empty
 */
auto GF_CORE_EXPORT SelectUsableKeyDatabases(
    const QContainer<KeyDatabaseInfo>& all, const KeyDatabaseInfo& fallback)
    -> QContainer<KeyDatabaseInfo>;

/**
 * @brief Return key database infos for custom (non-GPG) databases from
 * settings.
 *
 * @return list of KeyDatabaseInfo for custom databases
 */
auto GF_CORE_EXPORT GetKeyDatabaseInfoBySettings()
    -> QContainer<KeyDatabaseInfo>;

/**
 * @brief Return key database infos for all GPG key databases.
 *
 * @return list of KeyDatabaseInfo for GPG databases
 */
auto GF_CORE_EXPORT GetGpgKeyDatabaseInfos() -> QContainer<KeyDatabaseInfo>;

/**
 * @brief Return the name of the key database associated with @p channel.
 *
 * @param channel OpenPGP context channel
 * @return database name string
 */
auto GF_CORE_EXPORT GetGpgKeyDatabaseName(int channel) -> QString;

/**
 * @brief Extract key ID strings from a list of abstract keys for the given
 * channel.
 *
 * @param channel OpenPGP context channel
 * @param keys list of abstract key pointers
 * @return list of key ID strings
 */
auto GF_CORE_EXPORT ConvertKey2GpgKeyIdList(int channel,
                                            const GpgAbstractKeyPtrList& keys)
    -> KeyIdArgsList;

/**
 * @brief Convert a list of abstract keys to their underlying GpgKey objects.
 *
 * @param channel OpenPGP context channel
 * @param keys list of abstract key pointers
 * @return list of GpgKey pointers
 */
auto GF_CORE_EXPORT ConvertKey2GpgKeyList(int channel,
                                          const GpgAbstractKeyPtrList& keys)
    -> GpgKeyPtrList;

/**
 * @brief Convert a list of abstract keys to a container of GpgKey values.
 *
 * @param channel OpenPGP context channel
 * @param keys list of abstract key pointers
 * @return container of GpgKey values
 */
auto GF_CORE_EXPORT Convert2GpgKeyList(int channel,
                                       const GpgAbstractKeyPtrList& keys)
    -> QContainer<GpgKey>;

/**
 * @brief Return a string describing the usage flags of an abstract key (e.g.
 * "ESCA").
 *
 * @param key abstract key to inspect
 * @return usage string composed of capability letters
 */
auto GF_CORE_EXPORT GetUsagesByAbstractKey(const GpgAbstractKey* key)
    -> QString;

/**
 * @brief Look up the underlying GpgKey for the given abstract key.
 *
 * @param key abstract key pointer
 * @return corresponding GpgKey value
 */
auto GF_CORE_EXPORT GetGpgKeyByGpgAbstractKey(GpgAbstractKey* key) -> GpgKey;

/**
 * @brief Return whether the given key ID refers to a key group rather than a
 * single key.
 *
 * @param id key identifier to test
 * @return true if the ID refers to a key group
 */
auto GF_CORE_EXPORT IsKeyGroupID(const KeyId& id) -> bool;

/**
 * @brief Return whether the gpg-agent version on @p channel is greater than @p
 * version.
 *
 * @param channel OpenPGP context channel
 * @param version version string to compare against
 * @return true if the agent version is strictly greater
 */
auto GF_CORE_EXPORT GpgAgentVersionGreaterThan(int channel,
                                               const QString& version) -> bool;

/**
 * @brief Return the path to the appropriate pinentry program for the platform.
 *
 * @return absolute path to the pinentry binary, or empty if not found
 */
auto GF_CORE_EXPORT DecidePinentry() -> QString;

/**
 * @brief Return the GnuPG version string from the active context.
 *
 * @return GnuPG version string (e.g. "2.4.3")
 */
auto GF_CORE_EXPORT GnuPGVersion() -> QString;

/**
 * @brief Parse a raw user ID string (e.g. "Name <email>") into a GFUserId
 * structure.
 *
 * @param raw_id raw user ID string
 * @return parsed GFUserId with name, comment, and email fields
 */
auto GF_CORE_EXPORT ParseUserId(const QString& raw_id) -> GFUserId;

/**
 * @brief Assemble a user ID string from its components following the RFC 2822
 * mail name-addr convention referenced by RFC 9580 §5.12 / RFC 4880 §5.11.
 *
 * Produces "Name (Comment) <email>", or "Name <email>" when the comment is
 * empty. Components are trimmed and empty optional parts are omitted.
 *
 * @param name display name
 * @param comment optional comment
 * @param email email address
 * @return assembled user ID string
 */
auto GF_CORE_EXPORT AssembleUserId(const QString& name, const QString& comment,
                                   const QString& email) -> QString;

/**
 * @brief Check whether a user ID component (name or comment) is well-formed for
 * use in an RFC 2822 mail name-addr.
 *
 * Rejects the structural delimiters '(', ')', '<', '>' and any control
 * characters, which would otherwise produce a malformed or corrupted UID.
 *
 * @param component name or comment string to validate
 * @return true if the component contains no forbidden characters
 */
auto GF_CORE_EXPORT IsValidUserIdComponent(const QString& component) -> bool;

/**
 * @brief Convert an OpenPGPEngine enum value to its string name.
 *
 * @param type engine enum value
 * @return engine name string (e.g. "GPG")
 */
auto GF_CORE_EXPORT ConvertOpenPGPEngine2String(OpenPGPEngine type) -> QString;

/**
 * @brief Outcome of picking an engine for one key database.
 */
struct EngineChoice {
  bool ok = false;  ///< false when no engine is usable at all
  OpenPGPEngine engine = OpenPGPEngine::kRPGP;
};

/**
 * @brief Pick the engine for a key database, honouring the stated preference
 * only when that engine is actually available.
 *
 * The preference is a plain string carried in settings and in each stored
 * database entry, so it can name an engine this build does not have -- most
 * easily by being written by a build that did. Falling through to rPGP without
 * checking produced a context that could never work and a startup failure that
 * blamed the environment.
 *
 * Exposed primarily for unit testing.
 *
 * @param preferred "gnupg" or "rpgp", case- and whitespace-insensitive; empty
 * means no preference
 * @param gnupg_supported GnuPG is usable in this build
 * @param rpgp_supported rPGP is usable in this build
 */
auto GF_CORE_EXPORT ChooseOpenPGPEngine(const QString& preferred,
                                        bool gnupg_supported,
                                        bool rpgp_supported) -> EngineChoice;

/**
 * @brief Pick the engine for one key database, from that database.
 *
 * The engine belongs to the database, not to the channel it lands in.
 * `basic/default_engine` preselects the backend for a database the user is
 * *creating* and is the fallback for a stored entry that names none; it is not
 * a statement about whichever database happens to come first.
 *
 * Treating it as one is a real defect and not a hypothetical: channel 0 took
 * its engine from that setting while every other channel took it from the
 * database, so the two agreed only as long as the first database was the
 * DEFAULT one. An imported profile whose DEFAULT still names a path on the
 * machine it came from drops out as unusable, the next database moves up into
 * channel 0, and an rPGP keyring is opened with GnuPG.
 *
 * @param backend_type the database's own backend, empty if it names none
 * @param fallback_engine what to use when it does not, from settings
 * @param gnupg_supported GnuPG is usable in this build
 * @param rpgp_supported rPGP is usable in this build
 * @return the engine to open this database with, or a choice that is not ok
 */
auto GF_CORE_EXPORT ChooseKeyDatabaseEngine(const QString& backend_type,
                                            const QString& fallback_engine,
                                            bool gnupg_supported,
                                            bool rpgp_supported)
    -> EngineChoice;

/**
 * @brief Pick the engine for whichever database ends up in channel 0.
 *
 * Channel 0 is a position, not an identity: the databases that do not resolve
 * to a real directory are dropped, so the one that lands there is simply the
 * first that survived. `basic/default_engine` is a statement about the DEFAULT
 * database -- the one every profile derives rather than stores -- so it answers
 * only when DEFAULT is the database still standing there. For anything else,
 * the database's own backend does, exactly as it does for every other channel.
 *
 * A separate function from ChooseKeyDatabaseEngine() because *which* database
 * the setting is about is the thing that was wrong, and it was wrong silently:
 * an imported profile whose DEFAULT still names a path from the machine it came
 * from would open its first surviving keyring with the wrong engine.
 *
 * @param db_name name of the database that will occupy channel 0
 * @param backend_type that database's own backend, empty if it names none
 * @param default_engine the `basic/default_engine` setting
 * @param gnupg_supported GnuPG is usable in this build
 * @param rpgp_supported rPGP is usable in this build
 * @return the engine to open channel 0 with, or a choice that is not ok
 */
auto GF_CORE_EXPORT ChooseChannelZeroEngine(const QString& db_name,
                                            const QString& backend_type,
                                            const QString& default_engine,
                                            bool gnupg_supported,
                                            bool rpgp_supported)
    -> EngineChoice;

/**
 * @brief Convert a GpgComponentType enum value to its string name.
 *
 * @param type component type enum value
 * @return component name string
 */
auto GF_CORE_EXPORT ConvertComponentType2String(GpgComponentType type)
    -> QString;

/**
 * @brief Check whether a key carries no expiry at all.
 *
 * Both GnuPG and the rPGP key model report "no expiry" as the epoch, so an
 * expiration time of 0 must never be read as "expired in 1970".
 *
 * @param key key to inspect
 * @return true if the key never expires
 */
auto GF_CORE_EXPORT IsKeyNeverExpires(const GpgAbstractKey* key) -> bool;

/**
 * @brief Number of days ahead within which a key counts as expiring soon.
 *
 * Read from the "keys/expiring_soon_days" setting, clamped to a sane range.
 *
 * @return lookahead window in days
 */
auto GF_CORE_EXPORT GetKeyExpiringSoonDays() -> int;

/**
 * @brief Check whether a key is still usable but expires within the configured
 * lookahead window.
 *
 * Already expired, revoked and disabled keys are excluded: they need a
 * different remedy than a key that is merely about to lapse.
 *
 * @param key key to inspect
 * @return true if the key expires within the lookahead window
 */
auto GF_CORE_EXPORT IsKeyExpiringSoon(const GpgAbstractKey* key) -> bool;

/**
 * @brief The one condition worth reporting about a key, in severity order.
 */
enum class GpgKeyStatus {
  kOk,            ///< usable, and not about to lapse
  kExpiringSoon,  ///< still usable, expires within the lookahead window
  kExpired,       ///< past its expiry date
  kRevoked,       ///< revoked by its owner
  kDisabled,      ///< disabled locally
};

/**
 * @brief Reduce the four key conditions to the single one worth showing.
 *
 * A key can be several of these at once, so the precedence is the actual rule
 * and it is kept here, away from any key object, so it can be pinned down by a
 * test. It reproduces what the row tint has always done: disabled outranks
 * everything, then revoked or expired, then merely expiring soon.
 *
 * @param revoked key is revoked
 * @param disabled key is disabled
 * @param expired key is past its expiry
 * @param expiring_soon key lapses within the lookahead window
 * @return the single status to report
 */
auto GF_CORE_EXPORT ClassifyKeyStatus(bool revoked, bool disabled, bool expired,
                                      bool expiring_soon) -> GpgKeyStatus;

/**
 * @brief Sort rank for a status, healthy first.
 *
 * @param status status to rank
 * @return rank, ascending by severity
 */
auto GF_CORE_EXPORT KeyStatusSortRank(GpgKeyStatus status) -> int;

/**
 * @brief Translated one-word label for a status.
 *
 * @param status status to describe
 * @return label for the Status column
 */
auto GF_CORE_EXPORT DescribeKeyStatus(GpgKeyStatus status) -> QString;

/**
 * @brief Reduce several owner-trust levels to the one that ranks the group.
 *
 * The table shows a key group's trust as a single value, "*" when its members
 * disagree. Sorting needs the same reduction as a number. Members that
 * disagree, and a group with no members at all, rank below every real level —
 * neither tells you the group can be trusted.
 *
 * Kept as a free function over plain ints rather than over keys so the rule is
 * testable without a gpg context.
 *
 * @param levels owner-trust level of each member, as GpgKey::OwnerTrustLevel()
 * @return rank, ascending by trust; -1 when mixed or empty
 */
auto GF_CORE_EXPORT AggregateOwnerTrustRank(const QContainer<int>& levels)
    -> int;

/**
 * @brief Classify a key, reading the four conditions off the key itself.
 *
 * @param key key to inspect
 * @return the single status to report
 */
auto GF_CORE_EXPORT GetKeyStatus(const GpgAbstractKey* key) -> GpgKeyStatus;

}  // namespace GpgFrontend
