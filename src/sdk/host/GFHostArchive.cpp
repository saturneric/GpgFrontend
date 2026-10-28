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

/**
 * @file GFHostArchive.cpp
 * @brief The archive group: ZIP in memory over ArchiveFileOperator, so a
 * module gets the Host's hardened extraction rules rather than its own.
 */

#include "GFHostImpl.h"
#include "GFSDKHostApi.h"
#include "core/function/ArchiveFileOperator.h"
#include "core/model/GFBuffer.h"
#include "private/GFHostGate.h"

namespace gf_sdk_internal {

namespace {

auto ZipWrite(GFHostContextRef ctx, const GFArchiveEntry* entries, size_t count,
              GFBufferRef* out) -> int {
  GATE(ctx, 0, "archive.zip_write", -1);
  if (out == nullptr || (entries == nullptr && count != 0)) return -1;
  *out = nullptr;

  // The writer trusts its paths, so they are held to the rule the reader
  // enforces: an archive this builds is one this would accept.
  const auto policy = GpgFrontend::ArchiveExtractPolicy::Strict();
  QList<GpgFrontend::ArchiveMemberEntry> members;
  for (size_t i = 0; i < count; ++i) {
    const auto& e = entries[i];
    if (e.path == nullptr) return -1;
    GpgFrontend::ArchiveMemberEntry member;
    if (GpgFrontend::ValidateArchiveEntryPath(QString::fromUtf8(e.path), policy,
                                              member.relative_path) !=
        GpgFrontend::ArchiveEntryVerdict::kACCEPT) {
      return -1;
    }
    member.directory = e.directory != 0;
    if (!member.directory) member.bytes = gf_host::GFBufferViewToCore(e.data);
    members.append(std::move(member));
  }

  qsizetype index = 0;
  GpgFrontend::GFBuffer zip;
  const auto err =
      GpgFrontend::ArchiveFileOperator::NewArchiveFromMembersToSink(
          [&](GpgFrontend::ArchiveMemberEntry& entry) {
            if (index >= members.size()) return false;
            entry = std::move(members[index++]);
            return true;
          },
          [&](const char* data, qint64 size) {
            zip.Append(data, size);
            return true;
          },
          GpgFrontend::ArchiveCompression::kNONE,
          GpgFrontend::ArchiveFormat::kZIP);

  if (err != 0) return -1;
  *out = gf_host::GFBufferNewFromBytes(zip.Data(), zip.Size());
  return *out == nullptr ? -1 : 0;
}

auto ZipRead(GFHostContextRef ctx, GFBufferView in,
             const GFArchiveLimits* limits, GFArchiveEntryFn fn, void* user,
             GFBufferRef* reason) -> int {
  GATE(ctx, 0, "archive.zip_read", -1);
  if (reason != nullptr) *reason = nullptr;
  if (fn == nullptr || limits == nullptr ||
      limits->struct_size < sizeof(GFArchiveLimits)) {
    return -1;
  }

  auto policy = GpgFrontend::ArchiveExtractPolicy::Strict(
      limits->max_total_bytes, limits->max_entries);
  policy.max_entry_bytes = limits->max_entry_bytes;
  policy.reject_duplicate_paths = true;
  policy.reject_case_colliding_paths = true;
  policy.require_utf8_paths = true;
  // The caller's absolute budgets instead of a ratio: a ratio refuses
  // legitimately repetitive text.
  policy.max_compression_ratio = -1;

  // Borrowed for the call, as ReadArchiveMembersSync() reads it in place.
  const auto* data = static_cast<const char*>(gf_host::GFBufferData(in));
  const auto bytes = QByteArray::fromRawData(
      data,
      data == nullptr ? 0 : static_cast<qsizetype>(gf_host::GFBufferSize(in)));
  QString why;
  bool stopped = false;
  const auto err = GpgFrontend::ArchiveFileOperator::ReadArchiveMembersSync(
      bytes, policy,
      [&](const QString& path, const QByteArray& body) -> bool {
        auto* view =
            gf_host::GFBufferNewFromBytes(body.constData(), body.size());
        const auto rc = fn(user, path.toUtf8().constData(), view);
        gf_host::GFBufferRelease(view);
        if (rc != 0) stopped = true;
        return rc == 0;
      },
      &why);

  if (err == 0 && !stopped) return 0;
  if (reason != nullptr) {
    if (why.isEmpty()) why = stopped ? "stopped by the reader" : "unreadable";
    const auto text = why.toUtf8();
    *reason = gf_host::GFBufferNewFromBytes(text.constData(), text.size());
  }
  return -1;
}

}  // namespace

const GFHostArchiveApi kArchiveApi = {
    sizeof(GFHostArchiveApi),
    &ZipWrite,
    &ZipRead,
};

}  // namespace gf_sdk_internal
