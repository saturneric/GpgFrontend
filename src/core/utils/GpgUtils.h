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
 * @brief Return the raw settings objects for all configured key databases.
 *
 * @return list of KeyDatabaseItemSO settings objects
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
 * @brief Longest key database path GnuPG can still put its sockets in.
 *
 * gpg-agent binds its sockets inside the home directory, and a unix domain
 * socket address is capped by sockaddr_un::sun_path -- 104 bytes on Darwin, 108
 * on Linux, including the terminating NUL. Exceed it and gpg-agent refuses the
 * name and exits, so the socket never appears and every later operation fails
 * with GPG_ERR_ENOTSOCK. That is silent unless somebody checks the length up
 * front, which is what this exists for.
 *
 * The budget is measured against "/S.gpg-agent.browser", which is the longest
 * name gpg-agent builds -- and it builds it even though we never pass
 * --enable-browser-socket. It validates the name before deciding whether to
 * bind it, and a name that does not fit makes it exit(2) outright, so a home
 * directory where only that one socket is over is still a home directory GnuPG
 * refuses to run in.
 *
 * @return the maximum home directory length in bytes, or -1 where no such limit
 *         applies (Windows, which does not address these sockets by path)
 */
auto GF_CORE_EXPORT GnuPGHomePathByteBudget() -> int;

/**
 * @brief Whether GnuPG can host its agent sockets in @p home_path.
 *
 * Measured in UTF-8 bytes rather than characters: sun_path is a byte buffer, so
 * a non-ASCII user name costs more room than its character count suggests.
 *
 * @param home_path the GnuPG home directory (the key database directory)
 * @return true when the sockets fit, and always true where no limit applies
 */
auto GF_CORE_EXPORT GnuPGHomePathFitsSocketBudget(const QString& home_path)
    -> bool;

/**
 * @brief The home directory to hand GnuPG for @p home_path.
 *
 * Returns @p home_path itself whenever it already fits, so nothing new appears
 * on disk for the overwhelming majority of installations. When it does not fit,
 * returns a short symlink pointing at it.
 *
 * The redirection works because bind() resolves the symlink when it creates the
 * socket -- the socket file still lands in the real directory -- while the
 * string that has to fit in sun_path is the short one. Only GnuPG is given the
 * alias; storage, settings and packaging keep the real path, so a profile stays
 * self-contained and a package still describes itself in terms of its own
 * contents.
 *
 * Shortening the layout cannot replace this. The budget is fixed but the path
 * is not: the user name alone moves it, so any fixed layout is only ever a few
 * characters from failing again on somebody else's machine.
 *
 * @param home_path the real key database directory
 * @param alias_root where to keep links, or empty for the default under the
 *                   user's home directory; exists for testing, so a test never
 *                   writes into the real one
 * @return the path to give GnuPG, or empty when no usable one could be made
 */
auto GF_CORE_EXPORT ResolveGnuPGEngineHomePath(const QString& home_path,
                                               const QString& alias_root = {})
    -> QString;

/**
 * @brief Record that a GnuPG home directory cannot host the agent sockets.
 *
 * The reason has to reach the interface for the same cause the system secret
 * store's does: the failure is invisible at the default log level, and what the
 * user sees instead is an application that simply cannot reach GnuPG. Passing
 * an empty reason clears it.
 *
 * @param reason technical, untranslated, safe to paste into a bug report; must
 *               never contain secrets or a user's file paths -- report the
 *               arithmetic, not the path
 */
void GF_CORE_EXPORT RegisterGnuPGHomePathUnusable(QString reason);

/**
 * @brief Explain why GnuPG cannot use its home directory.
 *
 * Untranslated on purpose: it carries the measured length and the platform
 * limit, which is exactly what a maintainer needs read back verbatim. Callers
 * pair it with translated prose rather than showing it alone.
 *
 * @return the reason, or empty when the home directory is usable
 */
auto GF_CORE_EXPORT GnuPGHomePathUnusableReason() -> QString;

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
