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

#include <functional>

#include "core/model/DataObject.h"
#include "core/model/GFBuffer.h"
#include "core/model/GFDataExchanger.h"

namespace GpgFrontend {

/**
 * @brief Compression filter applied when building an archive.
 *
 * Compression belongs to libarchive rather than to a separate pass: an
 * encrypted stream does not compress, so anything that has to be both
 * compressed and encrypted must be compressed here, first.
 */
enum class ArchiveCompression {
  kNONE,  ///< store only, the historical behaviour
  kGZIP,  ///< gzip filter, used by profile packages
};

/**
 * @brief Limits and permissions applied while extracting an archive.
 *
 * Two callers with genuinely different needs share one extractor. Decrypting a
 * directory the user encrypted themselves has to reproduce whatever they put
 * in it, links included. A profile package is a format this application
 * defines, so it can refuse everything it never emits. Encoding that as a
 * policy rather than a flag keeps both callers honest about which one they
 * are.
 */
struct GF_CORE_EXPORT ArchiveExtractPolicy {
  /// Symbolic links are extracted rather than refused.
  bool allow_symlinks = true;

  /// Hard links are extracted rather than refused.
  bool allow_hardlinks = true;

  /// Ceiling on the sum of all extracted file data; -1 disables the check.
  qint64 max_total_bytes = -1;

  /// Ceiling on any single extracted file; -1 disables the check.
  qint64 max_entry_bytes = -1;

  /// Ceiling on the number of entries; -1 disables the check.
  int max_entries = -1;

  /// Ceiling on the length of an entry path, in UTF-16 code units.
  int max_path_length = 4096;

  /// Ceiling on how many path components an entry may have.
  int max_depth = 64;

  /**
   * @brief Refuse to extract unless the destination is a new empty directory.
   *
   * Merging an untrusted tree into a populated one is how an archive silently
   * replaces a file the caller never meant to expose. When this is set the
   * extractor also removes the destination on failure, which it may only do
   * because it knows it created nothing else there.
   */
  bool require_empty_destination = false;

  /**
   * @brief The historical behaviour, for archives the user built themselves.
   *
   * @return a policy with no limits and links allowed
   */
  [[nodiscard]] static auto Permissive() -> ArchiveExtractPolicy;

  /**
   * @brief The policy for profile packages.
   *
   * Links are refused outright rather than validated. "The link target stays
   * inside the tree" is a check that is repeatedly got wrong, including via a
   * race between the check and the extraction; a format that never emits links
   * can refuse the entry type instead and be done with the whole class of bug.
   *
   * @param max_total_bytes ceiling on extracted data, from the manifest plus
   * headroom; -1 for no ceiling
   * @param max_entries ceiling on entry count; -1 for no ceiling
   * @return a policy that refuses links and requires an empty destination
   */
  [[nodiscard]] static auto Strict(qint64 max_total_bytes = -1,
                                   int max_entries = -1)
      -> ArchiveExtractPolicy;
};

/**
 * @brief Why an archive entry was refused.
 */
enum class ArchiveEntryVerdict {
  kACCEPT,                ///< the entry may be extracted
  kREJECT_ABSOLUTE,       ///< the entry name is an absolute path
  kREJECT_DOTDOT,         ///< the entry name escapes the destination
  kREJECT_EMPTY,          ///< the entry name is empty or resolves to nothing
  kREJECT_PATH_TOO_LONG,  ///< the entry name exceeds max_path_length
  kREJECT_TOO_DEEP,       ///< the entry has more components than max_depth
};

/**
 * @brief Decide whether an entry name may be extracted, and normalise it.
 *
 * Pure, so every refusal can be asserted without building an archive. The
 * returned path is relative, forward-slash separated, and free of `.`
 * components; it is empty whenever the verdict is not kACCEPT.
 *
 * Windows drive letters and UNC prefixes count as absolute, and a backslash is
 * treated as a separator on every platform: an archive written on Windows must
 * not become a traversal vector when extracted on Linux.
 *
 * @param path_name entry name exactly as the archive carries it
 * @param policy limits to apply
 * @param[out] normalised cleaned relative path, empty unless kACCEPT
 * @return why the entry was refused, or kACCEPT
 */
auto GF_CORE_EXPORT ValidateArchiveEntryPath(const QString &path_name,
                                             const ArchiveExtractPolicy &policy,
                                             QString &normalised)
    -> ArchiveEntryVerdict;

/**
 * @brief Human-readable spelling of a verdict, for logs and messages.
 *
 * @param v verdict to spell
 * @return a short static string
 */
auto GF_CORE_EXPORT ArchiveEntryVerdictToString(ArchiveEntryVerdict v) -> const
    char *;

/**
 * @brief Decide whether a path is included in an archive being built.
 *
 * Receives the path relative to the archive root, using forward slashes and
 * with no trailing slash on directories. Returning false for a directory
 * prunes its entire subtree, which is what makes an exclusion cheap.
 */
using ArchiveEntryFilter = std::function<bool(const QString &relative_path)>;

/**
 * @brief Take an entry's bytes instead of the filesystem.
 *
 * Paired with an ArchiveEntryFilter on the extract side: an entry the filter
 * claims is read into memory and handed here, and `archive_write_header()` is
 * never called for it, so those bytes never reach any filesystem at all.
 *
 * Returning false aborts the extraction, exactly as a failed write does.
 *
 * The path is the same normalised, archive-relative form the filter saw.
 */
using ArchiveEntrySink =
    std::function<bool(const QString &relative_path, const GFBuffer &bytes)>;

/**
 * @brief One entry a caller wants written into an archive.
 *
 * Either a file to copy or bytes in hand, never both. A caller that has to
 * archive somebody's workspace cannot hold it in memory, and one synthesising a
 * manifest has no file to point at.
 */
struct GF_CORE_EXPORT ArchiveMemberEntry {
  QString relative_path;  ///< where it sits in the archive
  QString source_file;    ///< read from here, when set
  GFBuffer bytes;         ///< otherwise, these

  /// A directory entry, carrying neither. The paths of file entries create
  /// every directory that has something in it, so this is for the empty ones,
  /// which would otherwise not survive the round trip.
  bool directory = false;

  [[nodiscard]] auto FromFile() const -> bool {
    return !directory && !source_file.isEmpty();
  }
};

/**
 * @brief Yields the entries of an archive, in order.
 *
 * Returning false ends the archive. Lets a caller build one from whatever it
 * has -- a live directory, bytes it just produced, or both -- instead of having
 * to assemble a directory on disk first purely so that something can walk it.
 */
using ArchiveMemberProvider = std::function<bool(ArchiveMemberEntry &)>;

class GF_CORE_EXPORT ArchiveFileOperator {
 public:
  /**
   * @brief Log the contents of an archive on disk.
   *
   * @param archive_path path of the archive
   */
  static void ListArchive(const QString &archive_path);

  /**
   * @brief Pack a directory tree into a stream.
   *
   * Directories are emitted as entries in their own right. They used to be
   * dropped, because an entry was written only when the path could be opened
   * for reading and that fails on a directory — so an empty directory
   * disappeared from the archive and never came back on extraction.
   *
   * @param target_directory directory to pack; becomes the archive root
   * @param exchanger stream to write the archive into
   * @param cb completion callback
   * @param compression compression filter to apply
   * @param filter optional predicate deciding what is included
   */
  static void NewArchive2DataExchanger(
      const QString &target_directory, const QSharedPointer<GFDataExchanger> &,
      const OperationCallback &cb,
      ArchiveCompression compression = ArchiveCompression::kNONE,
      const ArchiveEntryFilter &filter = {});

  /**
   * @brief Pack a directory tree into a stream, on the calling thread.
   *
   * What NewArchive2DataExchanger() runs on the I/O runner. Exposed because a
   * caller that both packs and consumes the result cannot use the asynchronous
   * form: its completion callback is delivered to the thread that created the
   * task, so a caller that is itself busy draining the stream would be waiting
   * for a message it is not in a position to receive.
   *
   * Someone else must be reading the stream concurrently — the exchanger holds
   * a few megabytes and then blocks the writer.
   *
   * @param target_directory directory to pack; becomes the archive root
   * @param exchanger stream to write the archive into
   * @param compression compression filter to apply
   * @param filter optional predicate deciding what is included
   * @return 0 on success, negative on failure
   */
  static auto NewArchive2DataExchangerSync(
      const QString &target_directory, const QSharedPointer<GFDataExchanger> &,
      ArchiveCompression compression = ArchiveCompression::kNONE,
      const ArchiveEntryFilter &filter = {}) -> GFError;

  /**
   * @brief Unpack a stream into a directory.
   *
   * Every entry is validated against @p policy before anything is written, and
   * the limits are enforced as the data streams rather than after it lands, so
   * an archive that claims to be small and is not dies before it fills the
   * disk.
   *
   * @param fd stream to read the archive from
   * @param target_path directory to extract into
   * @param cb completion callback
   * @param policy limits and permissions to enforce
   */
  static void ExtractArchiveFromDataExchanger(
      const QSharedPointer<GFDataExchanger> &fd, const QString &target_path,
      const OperationCallback &cb,
      const ArchiveExtractPolicy &policy = ArchiveExtractPolicy::Permissive());

  /**
   * @brief Unpack a stream into a directory, on the calling thread.
   *
   * The synchronous half of ExtractArchiveFromDataExchanger(); see
   * NewArchive2DataExchangerSync() for why a self-contained caller needs it.
   * Someone else must be filling the stream concurrently.
   *
   * @param fd stream to read the archive from
   * @param target_path directory to extract into
   * @param policy limits and permissions to enforce
   * @param divert entries this claims are handed to @p sink instead of being
   * written; a directory entry it claims is dropped, since there is nothing to
   * store. Diversion happens only when both @p divert and @p sink are set.
   * @param sink where diverted bytes go; it receives the buffer rather than a
   * copy of it, so a sink that keeps the bytes keeps that very buffer
   * @return 0 on success, non-zero on failure
   */
  static auto ExtractArchiveFromDataExchangerSync(
      const QSharedPointer<GFDataExchanger> &fd, const QString &target_path,
      const ArchiveExtractPolicy &policy = ArchiveExtractPolicy::Permissive(),
      const ArchiveEntryFilter &divert = {}, const ArchiveEntrySink &sink = {})
      -> GFError;

  /**
   * @brief Pack entries from a provider into a stream.
   *
   * The counterpart of NewArchive2DataExchangerSync() for callers whose
   * contents are not a directory. Directories are created implicitly by the
   * paths of the entries, so a provider need only yield those it wants to exist
   * on their own -- an empty one has no file to imply it.
   *
   * The provider is trusted: entry paths are written as given, and nothing here
   * checks them for `..` or for being absolute the way extraction checks what
   * it reads. A caller assembling paths from anything it did not choose itself
   * has to validate them first.
   *
   * @param next yields entries until it returns false
   * @param exchanger stream to write the archive into
   * @param compression whether to gzip
   * @return 0 on success, non-zero on failure
   */
  static auto NewArchiveFromMembersSync(
      const ArchiveMemberProvider &next,
      const QSharedPointer<GFDataExchanger> &exchanger,
      ArchiveCompression compression) -> GFError;
};
}  // namespace GpgFrontend
