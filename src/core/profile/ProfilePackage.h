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

#include "core/model/GFBuffer.h"
#include "core/profile/ProfileAccessor.h"
#include "core/profile/ProfileAreaTraits.h"
#include "core/profile/ProfileMember.h"
#include "core/struct/settings_object/KeyDatabaseItemSO.h"
#include "core/typedef/CoreTypedef.h"

namespace GpgFrontend {

/// Outer magic. Eight bytes, so `file(1)` and the freedesktop MIME database
/// have something to match, and so "not a profile package" is distinguishable
/// from "wrong passphrase" without doing any key derivation first.
inline constexpr auto kProfilePackageMagic = "GFPROF1\n";
inline constexpr int kProfilePackageMagicLength = 8;

/// What was written, and what it takes to read it. Only the second is ever
/// grounds for a refusal: a writer that adds a field an older build can ignore
/// raises the first and leaves the second alone, and its packages keep opening
/// here.
inline constexpr int kProfilePackageFormatVersion = 2;
inline constexpr int kProfilePackageMinReader = 2;

/// The first version whose body is a stream of authenticated chunks rather than
/// one sealed block.
///
/// Version 1 held the whole payload and its ciphertext in memory at once, which
/// capped a package at a couple of hundred megabytes and put several full
/// copies of the plaintext -- the application key among them -- on the ordinary
/// heap. It is still read, because packages already exist; it is no longer
/// written, because keeping a second writer alive is how the plaintext key
/// survived in the export path for as long as it did.
inline constexpr int kProfilePackageStreamedFrom = 2;

/// Where the tree sits inside the archive; `manifest.json` sits beside it.
inline constexpr auto kProfilePackageTreePrefix = kProfileTreePrefix;

/// The extension every desktop shell registers the format under, dot included.
/// Short enough to survive an 8.3 filesystem, and spelled once: the argv scan,
/// three file dialogs, a freedesktop glob, a Windows ProgID and a macOS UTI tag
/// all have to agree, and a disagreement shows up as a file that opens from the
/// menu and not by double-click.
///
/// Deliberately not the same string as the `format` field inside the header,
/// which stays "gfprofile": the extension is a shorthand for users, the format
/// name is the format's identity.
inline constexpr auto kProfilePackageExtension = ".gfp";

/// The media type the format is registered under. Named three times outside
/// this repository -- a shared-mime-info entry, a Windows "Content Type" value
/// and a `public.mime-type` tag -- so it is spelled once here.
inline constexpr auto kProfilePackageMimeType =
    "application/x-gpgfrontend-profile";

/// The reverse-DNS identifier macOS Launch Services knows the format by. Not
/// flavour-suffixed: it names the *format*, which a testing build and a release
/// build produce identically.
inline constexpr auto kProfilePackageUti = "com.bktus.gpgfrontend.profile";

/**
 * @brief What protects a package's payload.
 *
 * A package travels between machines, so the system credential store is never
 * an option: a key wrapped by one machine's keychain cannot be opened on
 * another. That leaves a passphrase, or nothing at all.
 */
enum class ProfilePackageProtection : std::uint8_t {
  kPIN,   ///< XChaCha20-Poly1305 with an Argon2id-derived key
  kNONE,  ///< plain gzip'd tar; readable and alterable by anyone who has it
};

auto GF_CORE_EXPORT ProfilePackageProtectionToString(
    ProfilePackageProtection protection) -> QString;
auto GF_CORE_EXPORT ProfilePackageProtectionFromString(const QString &value)
    -> ProfilePackageProtection;

/**
 * @brief The plaintext routing header at the front of every package.
 *
 * Deliberately holds no profile name, key id or database name: those live in
 * the encrypted manifest. Everything here is also duplicated inside the
 * manifest so the two can be compared once the payload is open — the header is
 * attacker-controllable and may reject, but must never be believed.
 */
struct GF_CORE_EXPORT ProfilePackageHeader {
  int format_version = kProfilePackageFormatVersion;
  int min_reader = kProfilePackageMinReader;
  QString writer;              ///< application version that wrote it
  bool writer_stable = false;  ///< whether that build was a release
  QString created;             ///< ISO-8601, UTC
  ProfilePackageProtection protection = ProfilePackageProtection::kPIN;
};

/**
 * @brief Serialise a header, magic and length prefix included.
 *
 * @param header header to encode
 * @return the exact bytes that go at the front of the file
 */
auto GF_CORE_EXPORT
EncodeProfilePackageHeader(const ProfilePackageHeader &header) -> QByteArray;

/**
 * @brief Why a file could not be recognised as a package.
 */
enum class ProfilePackageHeaderStatus : std::uint8_t {
  kOK,
  kNOT_A_PACKAGE,  ///< the magic is not there
  kTRUNCATED,      ///< the magic is there but the header is not all present
  kMALFORMED,      ///< the header is present but not usable
  kTOO_NEW,        ///< a format this build does not know how to read
};

/**
 * @brief A parsed header plus the exact bytes it was parsed from.
 */
struct GF_CORE_EXPORT ProfilePackageHeaderView {
  ProfilePackageHeaderStatus status = ProfilePackageHeaderStatus::kOK;
  QString detail;

  ProfilePackageHeader header;
  QByteArray header_bytes;  ///< what the digest in the manifest covers
  qint64 body_offset = 0;

  [[nodiscard]] auto Ok() const -> bool {
    return status == ProfilePackageHeaderStatus::kOK;
  }
};

/**
 * @brief Read the header of a package.
 *
 * Pure and total, so every rejection is assertable without a file on disk.
 * Rejecting a version this build cannot read happens here, before any key
 * derivation — that is the one thing the header is allowed to decide alone.
 *
 * @param bytes the start of the file; the whole file is fine too
 * @return the parse result
 */
auto GF_CORE_EXPORT ParseProfilePackageHeader(const QByteArray &bytes)
    -> ProfilePackageHeaderView;

/**
 * @brief A digest of the exact header bytes, recorded inside the payload.
 *
 * @param header_bytes bytes as they appear in the file
 * @return lowercase hex BLAKE2b-256
 */
auto GF_CORE_EXPORT ProfilePackageHeaderDigest(const QByteArray &header_bytes)
    -> QString;

/**
 * @brief One key database as it was at export time.
 */
struct GF_CORE_EXPORT ProfilePackageKeyDatabaseEntry {
  QString name;
  QString stored_path;  ///< `@profile/...` when it travelled, absolute if not
  QString backend_type;
  bool external = false;  ///< outside the profile, so its contents stayed home
};

/**
 * @brief What the package says about itself, from inside the payload.
 *
 * Reuses ProfileMarker's field vocabulary for the layout version pair so the
 * compatibility gate and the migration planner can be pointed straight at it —
 * that is what lets a package written by an older build open on a newer one.
 */
struct GF_CORE_EXPORT ProfilePackageManifest {
  int manifest_version = 1;

  /// Duplicated from the header, and compared against it after decryption.
  int format_version = kProfilePackageFormatVersion;
  int min_reader = kProfilePackageMinReader;
  QString protection;
  QString header_digest;

  /// The layout version of the tree inside, not of the container.
  int schema_version = 0;
  int min_reader_version = 0;

  QString app_profile;     ///< GetAppProfileName() of the writer
  QString display_name;    ///< what the profile was called
  QString profile_id;      ///< its folder name at home; only a suggestion here
  QString writer_version;  ///< application version
  QString created;         ///< ISO-8601, UTC
  QString package_id;      ///< 16 random bytes, hex; identity, never a path

  QString app_key_protection = "none";  ///< always: the package protects it
  bool workspace_included = false;

  /// Total size of everything the package carries, unpacked, in bytes.
  ///
  /// Zero on packages written before this was recorded, which is why every
  /// reader has to treat it as a hint rather than a fact: it decides how much
  /// room a session is provisioned with, not how much is extracted.
  ///
  /// The point of recording it is that room is otherwise guessed from the
  /// *compressed* size, and a profile that compresses unusually well is then
  /// handed storage too small to open in -- discovered part way through
  /// unpacking, or worse, at the close that has to write it back.
  qint64 uncompressed_bytes = 0;
  bool self_contained = false;

  QList<ProfilePackageKeyDatabaseEntry> key_databases;

  /**
   * @brief Keys this build did not recognise, preserved verbatim.
   *
   * The same promise ProfileMarker makes, for the same reason: a package
   * written by a newer build can be opened here, and importing it and
   * exporting it again must not quietly destroy what that build put in.
   */
  QJsonObject unknown_fields;
};

auto GF_CORE_EXPORT EncodeProfilePackageManifest(
    const ProfilePackageManifest &manifest) -> QByteArray;
auto GF_CORE_EXPORT ParseProfilePackageManifest(const QByteArray &bytes)
    -> std::optional<ProfilePackageManifest>;

/**
 * @brief Compare the plaintext header against the manifest that was sealed.
 *
 * Anyone can edit the header of a file they hold; nobody can edit the manifest
 * without the passphrase. A disagreement is therefore tampering, and is
 * reported as tampering rather than as a format problem.
 *
 * @param header header as parsed from the file
 * @param header_bytes the exact bytes it was parsed from
 * @param manifest manifest recovered from the payload
 * @return an explanation, or an empty string when they agree
 */
auto GF_CORE_EXPORT CheckPackageHeaderAgainstManifest(
    const ProfilePackageHeader &header, const QByteArray &header_bytes,
    const ProfilePackageManifest &manifest) -> QString;

/**
 * @brief Rewrite stored key database paths so they travel.
 *
 * A path inside the profile becomes `@profile/...`, which resolves against
 * whatever root the profile is opened at next. A path outside it is left
 * exactly as it is: the database is not ours to move, its contents are not in
 * the package, and inventing a path would produce an empty database wearing the
 * name of a real one.
 *
 * Pure, and idempotent — a list that already travelled comes back unchanged.
 *
 * @param databases the stored list
 * @param profile_root root to make paths relative to
 * @return the rewritten list, in the same order
 */
auto GF_CORE_EXPORT RewriteKeyDatabaseListForPacking(
    const QContainer<KeyDatabaseItemSO> &databases, const QString &profile_root)
    -> QContainer<KeyDatabaseItemSO>;

/**
 * @brief Describe a stored key database list for the manifest.
 *
 * @param databases the list, already rewritten by
 * RewriteKeyDatabaseListForPacking()
 * @return one entry per database, the ones that cannot travel marked
 */
auto GF_CORE_EXPORT
DescribeKeyDatabasesForManifest(const QContainer<KeyDatabaseItemSO> &databases)
    -> QList<ProfilePackageKeyDatabaseEntry>;

/**
 * @brief Sizes of the areas a package would carry, in bytes.
 *
 * Per area rather than one total, because refusing an export is only useful if
 * it can say which part is the problem — in practice almost always the
 * workspace, which is the one directory with no bound.
 *
 * @param profile_root root to measure
 * @return area name to size; keys are `config`, `data_objs`, `secure`,
 * `key_databases` and `workspace`
 */
auto GF_CORE_EXPORT MeasureProfileAreas(const QString &profile_root)
    -> QMap<QString, qint64>;

/**
 * @brief The largest payload this machine can pack in one piece.
 *
 * Encryption is one-shot: the payload and its ciphertext are both live at once
 * and both sit in secure, page-locked memory, so the ceiling is the process's
 * locked-memory allowance rather than its RAM. Detected at runtime so the
 * refusal can name a real number instead of a guess.
 *
 * @return the cap in bytes
 */
auto GF_CORE_EXPORT ProfilePackagePayloadCap() -> qint64;

/**
 * @brief Outcome of building a staging tree.
 */
struct GF_CORE_EXPORT ProfileStagingResult {
  bool ok = false;
  QString error;
  qint64 bytes = 0;

  /// Top-level names in the profile folder that the package does not carry,
  /// sorted. Inclusion is an allow-list, so leaving something out is silent by
  /// default -- and a sender who kept notes beside their profile should learn
  /// that here rather than from the copy on somebody else's machine.
  QStringList skipped;
};

/**
 * @brief Copy a profile into a tree that is ready to be packed.
 *
 * Copies rather than packs in place so the plaintext application key, the
 * rewritten settings and the manifest never touch the live profile, and so the
 * existing directory-to-archive path is all that is needed afterwards.
 *
 * Logs and modules are never copied, nor lock files, quarantined data objects
 * or agent sockets. A file that vanishes mid-copy is skipped rather than
 * failing the export.
 *
 * A nested `profiles/` directory is never copied either: a root profile has the
 * profiles root inside it, holding the other profiles on this machine and the
 * scratch directory of this very export, and following it would not terminate.
 *
 * @param profile_root profile to copy
 * @param staging_dir destination; created, and must not already exist
 * @param include_workspace whether the user's own files come too
 * @return the outcome, with the copied size
 */
auto GF_CORE_EXPORT StageProfileTree(const QString &profile_root,
                                     const QString &staging_dir,
                                     bool include_workspace)
    -> ProfileStagingResult;

/**
 * @brief Copy every setting out of a live store.
 *
 * Taken as a snapshot rather than read where it is needed, because the packing
 * itself runs on a worker thread and QSettings is not something two threads may
 * share. Cheap: settings are a few kilobytes.
 *
 * @param settings the live settings
 * @return every key and its value
 */
auto GF_CORE_EXPORT SnapshotSettings(QSettings &settings)
    -> QMap<QString, QVariant>;

/**
 * @brief Outcome of writing a package.
 */
struct GF_CORE_EXPORT ProfilePackageWriteResult {
  bool ok = false;
  QString error;
  qint64 bytes = 0;

  /// Top-level names in the profile folder the package does not carry, sorted.
  /// Inclusion is an allow-list, so leaving something out is silent by default.
  QStringList skipped;
};

/**
 * @brief Why a package could not be read.
 */
enum class ProfilePackageReadStatus : std::uint8_t {
  kOK,
  kNOT_A_PACKAGE,
  kTOO_NEW,
  kBAD_PASSPHRASE,
  kTAMPERED,   ///< the header and the sealed manifest disagree
  kMALFORMED,  ///< the payload is not a profile tree
  /// The storage filled up part way through unpacking. Distinguished from
  /// kIO_FAILED because "the package's contents could not be unpacked" sends a
  /// user hunting for corruption in a file that is perfectly fine.
  kNO_SPACE,
  /// The file is larger than this build can open. Reading one means holding it
  /// whole, several times over, so the refusal has to come before the first
  /// read rather than as an allocation failure part way through.
  kTOO_LARGE,
  kIO_FAILED,
};

/**
 * @brief Outcome of reading a package.
 */
struct GF_CORE_EXPORT ProfilePackageReadResult {
  ProfilePackageReadStatus status = ProfilePackageReadStatus::kOK;
  QString detail;

  ProfilePackageHeader header;
  ProfilePackageManifest manifest;  ///< only meaningful after a full read

  [[nodiscard]] auto Ok() const -> bool {
    return status == ProfilePackageReadStatus::kOK;
  }
};

/**
 * @brief Read a package's header without touching its payload.
 *
 * Cheap on purpose: it answers "is this a package, and does it need a
 * passphrase" before anything spends time on key derivation.
 *
 * @param package_path file to inspect
 * @return the outcome, header filled in on success
 */
auto GF_CORE_EXPORT InspectProfilePackage(const QString &package_path)
    -> ProfilePackageReadResult;

/**
 * @brief Unpack a package into a staging directory.
 *
 * Extraction is strict: no symbolic or hard links, no device nodes, nothing
 * outside the destination, and limits enforced as the data streams so an
 * archive that claims to be small and is not dies early. The destination is
 * created fresh and is removed entirely if anything goes wrong — a
 * half-extracted tree that later got adopted would be worse than no profile.
 *
 * Synchronous, and must not be called on the I/O task runner.
 *
 * @param package_path file to read
 * @param staging_dir destination; created, and must not already exist
 * @param passphrase passphrase, ignored for an unprotected package
 * @return the outcome, with the manifest on success
 */
/// Areas an unpacker keeps out of the filesystem, and where their bytes go
/// instead. Built from the session's storage: an area the driver holds in
/// memory is one the extraction must never write down.
struct GF_CORE_EXPORT ProfileExtractionRouting {
  /// Profile-relative directory prefixes to divert, e.g. "secure".
  QStringList resident_dirs;

  /// Called with a profile-relative path and the entry's bytes.
  std::function<bool(const QString &, const GFBuffer &)> store;

  [[nodiscard]] auto Active() const -> bool {
    return store && !resident_dirs.isEmpty();
  }
};

/**
 * @brief Read a package's manifest without unpacking anything.
 *
 * The manifest is the first member of a streamed archive, so this stops as soon
 * as it has one -- a chunk or two of the file, whatever its size.
 *
 * It exists so a session can be sized before it is provisioned. Storage has to
 * be chosen before there is anywhere to unpack into, and the alternative was
 * guessing from the package's compressed size; a profile that compresses well
 * was then handed storage too small to open in, which surfaces part way through
 * unpacking or, worse, at the close that has to write it back.
 *
 * A version 1 package is not looked inside at all: its manifest never carried
 * a size, so a pass over one would cost a full decryption to learn nothing --
 * and version 1 makes no promise the manifest comes first, so that pass could
 * pull an arbitrary member into memory on the way. Such a package comes back
 * with a header and an empty manifest, which is a size of zero, which is the
 * hint every reader already has to tolerate.
 *
 * @param package_path the `.gfp` to look inside
 * @param passphrase passphrase, ignored for an unprotected package
 * @return the header and manifest; nothing is written anywhere
 */
auto GF_CORE_EXPORT PeekProfilePackageManifest(const QString &package_path,
                                               const GFBuffer &passphrase)
    -> ProfilePackageReadResult;

auto GF_CORE_EXPORT ReadProfilePackage(
    const QString &package_path, const QString &staging_dir,
    const GFBuffer &passphrase, const ProfileExtractionRouting &routing = {})
    -> ProfilePackageReadResult;

/**
 * @brief Everything an export needs, gathered before it leaves the main thread.
 *
 * The application key and the settings are read where they live and carried
 * here as values: the packing runs on a worker, and neither the key manager nor
 * QSettings is something two threads may share.
 */
struct GF_CORE_EXPORT ProfileExportRequest {
  QString profile_root;

  QString dest_path;  ///< the `.gfp` to write

  bool include_workspace = false;
  ProfilePackageProtection protection = ProfilePackageProtection::kPIN;

  /// The secure area, already resolved to the bytes that travel.
  ///
  /// Built by ResolveSecureAreaMembers(), which is where the difference between
  /// the application key and the rotated keys is decided -- the first comes
  /// from the plaintext key in hand because the stored form may be sealed by
  /// this machine's credential store, the rest are carried exactly as stored.
  /// Resolved here rather than during packing because it reads the session's
  /// accessor, and the packing runs on a worker thread.
  ///
  /// Empty means there is no application key, which is not an exportable
  /// profile.
  QList<ProfileMember> secure_members;

  GFBuffer passphrase;  ///< ignored when protection is kNONE

  QMap<QString, QVariant> settings;
  ProfilePackageManifest manifest;  ///< identity fields; the rest is filled in
};

/**
 * @brief Pack a profile into a `.gfp`, start to finish.
 *
 * Stages the profile into a scratch directory, seals it, and removes the
 * scratch directory afterwards whatever happens — it holds an unprotected copy
 * of the application key for as long as it exists, so it is not left behind on
 * failure either.
 *
 * Synchronous and thread-safe with respect to the caller's own state; must not
 * be called on the I/O task runner.
 *
 * @param request everything gathered from the running profile
 * @return the outcome
 */
auto GF_CORE_EXPORT ExportProfilePackage(ProfileExportRequest request)
    -> ProfilePackageWriteResult;

/**
 * @brief Move one file or directory, even across a filesystem boundary.
 *
 * A rename is tried first and is what almost always happens. The copy is for
 * the one case that genuinely crosses: an import, whose staging sits wherever
 * the session storage was provisioned while the profile it produces belongs in
 * the profiles folder. QDir::rename cannot cross a mount point, and without the
 * fallback an import from protected storage fails claiming the package is
 * unreadable.
 *
 * A failed copy leaves nothing behind: half a profile would be adopted as a
 * whole one.
 *
 * @param source what to move
 * @param destination where it goes; must not already exist
 * @return true when the destination holds it and the source is gone
 */
auto GF_CORE_EXPORT MoveTreeAcrossFilesystems(const QString &source,
                                              const QString &destination)
    -> bool;

/**
 * @brief How AdoptExtractedProfile() moves each entry into place.
 *
 * A seam rather than a hard call, because the crossing case cannot be produced
 * in a unit test: there is no portable way to make a rename fail with EXDEV on
 * demand, and a test that needed a second filesystem would not run anywhere.
 */
using ProfileTreeMover = std::function<bool(const QString &, const QString &)>;

/**
 * @brief Turn an extracted tree into a profile this machine owns.
 *
 * A package is a copy, not the same profile: the imported root gets a fresh
 * identity, so its credential-store account cannot collide with the one it came
 * from, and its application key stays unprotected until the user says
 * otherwise — the source machine may well have recorded "keychain", which here
 * would only strand it.
 *
 * @param staging_dir extracted tree, as produced by ReadProfilePackage()
 * @param profile_root where the profile should end up
 * @param id profile id to record
 * @param display_name name to record
 * @param manifest the package's manifest
 * @param mover how to move each entry; empty means
 * MoveTreeAcrossFilesystems()
 * @return an error message, empty on success
 */
auto GF_CORE_EXPORT AdoptExtractedProfile(
    const QString &staging_dir, const QString &profile_root, const QString &id,
    const QString &display_name, const ProfilePackageManifest &manifest,
    const ProfileTreeMover &mover = {}) -> QString;

/**
 * @brief A staging directory name the profile scan will never adopt.
 *
 * Dot-prefixed for exactly that reason: a half-built or half-extracted tree
 * must not be able to show up as a profile.
 *
 * @param profiles_root where profiles live
 * @param purpose short tag, e.g. "staging" or "extract"
 * @return an absolute path that does not exist yet
 */
auto GF_CORE_EXPORT MakeProfilePackageScratchDir(const QString &profiles_root,
                                                 const QString &purpose)
    -> QString;

/**
 * @brief The root a package runs at when it is opened temporarily.
 *
 * Derived from the package's own path rather than minted at random, so that
 * "is this package already open in another window" is one lock probe on a path
 * both processes can work out for themselves — no sidecar file, no scan, and
 * no way for the answer to drift.
 *
 * Dot-prefixed like every transient root: a session is disposable, and the
 * profile scan must never adopt one as a profile this machine owns.
 *
 * @param profiles_root where profiles live
 * @param package_path the `.gfp`; need not exist yet
 * @return an absolute path, or an empty string when the package has no path
 */
auto GF_CORE_EXPORT ProfileSessionRoot(const QString &profiles_root,
                                       const QString &package_path) -> QString;

/**
 * @brief Remove session roots left behind by processes that are gone.
 *
 * A session holds an ordinary profile lock on its own root, so the lock is the
 * liveness test and a crashed session is collected on the next start with no
 * bookkeeping of its own.
 *
 * Only roots that carry a `profile.json` are considered, which is exactly the
 * set of adopted sessions. Staging and extraction scratch is dot-prefixed too
 * but never holds a lock, and deleting one would break an export that is
 * running in another window right now.
 *
 * @param profiles_root where profiles live
 * @param keep_root a root to leave alone, typically this process's own
 * @return how many were removed
 */
auto GF_CORE_EXPORT SweepTransientProfileRoots(const QString &profiles_root,
                                               const QString &keep_root) -> int;

/**
 * @brief Outcome of opening a package as a temporary session.
 */
struct GF_CORE_EXPORT ProfileSessionOpenResult {
  ProfilePackageReadStatus status = ProfilePackageReadStatus::kOK;
  QString detail;

  QString session_root;
  ProfilePackageManifest manifest;

  [[nodiscard]] auto Ok() const -> bool {
    return status == ProfilePackageReadStatus::kOK;
  }
};

/**
 * @brief Extract a package into a session root and make it usable as a profile.
 *
 * The compatibility gate runs before anything is adopted, never after: a
 * package describing a layout this build cannot read must leave no trace.
 *
 * The caller must already hold the profile lock on the session root — two
 * windows writing one package back is how this loses data. Anything found at
 * the session root therefore belonged to a process that is gone, and is
 * replaced.
 *
 * Synchronous, and must not be called on the I/O task runner.
 *
 * @param package_path package to open
 * @param storage the session's storage driver, already provisioned. Both the
 * destination and the staging come from it, so extraction never crosses a
 * filesystem and the tree never touches anywhere the driver did not choose.
 * @param passphrase passphrase, ignored for an unprotected package
 * @param this_schema_version the layout version this build speaks
 * @return the outcome, with the session root and manifest on success
 */
/**
 * @brief Record where a session's storage went, next to its lock.
 *
 * The anchor is in the profiles folder and the storage may be anywhere — in
 * memory, in an encrypted volume, in a temporary directory. A process that dies
 * leaves this pointer and nothing else, and without it the tree is stranded
 * somewhere no later sweep would think to look.
 *
 * Written before extraction rather than after, for the same reason.
 *
 * @param anchor the locked directory in the profiles folder
 * @param state the driver's AnchorState()
 * @return true when the pointer is on disk
 */
auto GF_CORE_EXPORT WriteSessionPointer(const QString &anchor,
                                        const QJsonObject &state) -> bool;

/**
 * @brief Read back a session pointer, if there is one.
 *
 * @param anchor the directory to look in
 * @return the recorded state, or an empty object when this is not an anchor
 */
auto GF_CORE_EXPORT ReadSessionPointer(const QString &anchor) -> QJsonObject;

auto GF_CORE_EXPORT OpenPackageSession(const QString &package_path,
                                       ProfileAccessor &storage,
                                       const GFBuffer &passphrase,
                                       int this_schema_version)
    -> ProfileSessionOpenResult;

}  // namespace GpgFrontend
