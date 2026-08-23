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

#include "core/model/GFBuffer.h"

namespace GpgFrontend {

/// Plaintext chunk size. Every chunk is authenticated on its own, so this is
/// also the granularity at which a damaged package is detected — and, with the
/// exchanger's ring, the whole of the working set at any package size.
constexpr qint64 kProfilePackageChunkBytes = 1LL * 1024 * 1024;

/// How much of a chunk header a reader must see before it can size the read.
constexpr int kProfilePackageChunkPrefixBytes = 4;

/**
 * @brief Writes a package body as a stream of authenticated chunks.
 *
 * The point of the shape. The one-shot container it replaces held the whole
 * payload and its ciphertext in memory at once, which capped a package at a
 * couple of hundred megabytes, put several full copies of the plaintext on the
 * ordinary heap, and made the profile's own key transit a multi-megabyte buffer
 * on its way to being written down.
 *
 * Layout, after the plaintext routing header the caller has already written:
 *
 *     Argon2id salt | secretstream header | [u32 length | chunk] ...
 *
 * The final chunk carries the stream's FINAL tag, so truncation is detected
 * rather than read as a short package.
 *
 * An unprotected package has no salt and no chunk framing: its body is the
 * payload itself, written straight through. There is nothing to authenticate
 * with and pretending otherwise would only cost.
 */
class GF_CORE_EXPORT ProfilePackageStreamWriter {
 public:
  /**
   * @brief Begin a body.
   *
   * @param sink called with each run of bytes to append to the file
   * @param passphrase empty for an unprotected package
   */
  ProfilePackageStreamWriter(std::function<bool(const char *, qint64)> sink,
                             GFBuffer passphrase, bool is_protected);

  ~ProfilePackageStreamWriter();

  ProfilePackageStreamWriter(const ProfilePackageStreamWriter &) = delete;
  auto operator=(const ProfilePackageStreamWriter &)
      -> ProfilePackageStreamWriter & = delete;
  ProfilePackageStreamWriter(ProfilePackageStreamWriter &&) = delete;
  auto operator=(ProfilePackageStreamWriter &&)
      -> ProfilePackageStreamWriter & = delete;

  /**
   * @brief Write the salt and stream header. Call once, before any payload.
   *
   * @return false when the key could not be derived or the sink refused
   */
  auto Begin() -> bool;

  /**
   * @brief Add payload bytes. Buffered until a whole chunk is ready.
   *
   * @param data bytes to add
   * @param length how many
   * @return false when the sink refused
   */
  auto Write(const char *data, qint64 length) -> bool;

  /**
   * @brief Flush the last chunk with the stream's final tag.
   *
   * @return false when the sink refused
   */
  auto Finish() -> bool;

 private:
  auto FlushChunk(bool final_chunk) -> bool;

  struct Impl;
  std::unique_ptr<Impl> impl_;
};

/**
 * @brief Reads a body written by ProfilePackageStreamWriter.
 *
 * Feeds plaintext out a chunk at a time, so a package is never held whole.
 */
class GF_CORE_EXPORT ProfilePackageStreamReader {
 public:
  /**
   * @brief Open a body.
   *
   * @param source called to read the next @p n bytes of the file, returning
   * how many it actually produced
   * @param passphrase the package's passphrase; ignored when @p is_protected
   * is false
   * @param is_protected what the package's own header declares, never inferred
   * from whether a passphrase was supplied. The two are not the same question:
   * an empty passphrase handed to a protected package would otherwise be read
   * as "this package is not protected", and the ciphertext would go to the
   * archive reader as if it were a plain body -- reported to the user as a
   * corrupt package when all they did was leave the box blank.
   */
  ProfilePackageStreamReader(std::function<qint64(char *, qint64)> source,
                             GFBuffer passphrase, bool is_protected);

  ~ProfilePackageStreamReader();

  ProfilePackageStreamReader(const ProfilePackageStreamReader &) = delete;
  auto operator=(const ProfilePackageStreamReader &)
      -> ProfilePackageStreamReader & = delete;
  ProfilePackageStreamReader(ProfilePackageStreamReader &&) = delete;
  auto operator=(ProfilePackageStreamReader &&)
      -> ProfilePackageStreamReader & = delete;

  /**
   * @brief Read the salt and stream header, and derive the key.
   *
   * @return false when the passphrase is wrong or the body is malformed; ask
   * Authentic() which
   */
  auto Begin() -> bool;

  /**
   * @brief Produce the next run of plaintext.
   *
   * A GFBuffer rather than a QByteArray: this is the decrypted profile, the
   * application key among it, and it is wiped rather than merely released --
   * both between chunks and when the reader goes away. Handing it out on the
   * ordinary heap would put the plaintext this format exists to protect back
   * where the format it replaced used to leave it.
   *
   * @param out filled with the chunk; empty when the stream ended cleanly
   * @return false on a failed authentication or a malformed body
   */
  auto Next(GFBuffer &out) -> bool;

  /**
   * @brief Whether the stream ended on its final tag.
   *
   * False after a clean-looking read means the file was truncated, which a
   * length-prefixed format cannot otherwise tell from a short one.
   *
   * @return true when the end was the real end
   */
  [[nodiscard]] auto Complete() const -> bool;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace GpgFrontend
