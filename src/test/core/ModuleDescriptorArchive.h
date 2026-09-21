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

#include <QByteArray>
#include <QFile>
#include <QPair>
#include <QString>
#include <QTemporaryDir>
#include <QVector>
#include <array>
#include <thread>

#include "core/function/ArchiveFileOperator.h"

namespace GpgFrontend::Test {

/**
 * @file ModuleDescriptorArchive.h
 * @brief Writing a `.gfmodule` archive from members, for tests only.
 *
 * Two kinds of test need this and neither can go through the builder:
 * tampering tests, which must NOT re-sign, and external-module tests, which
 * must sign with a key this build does not hold. One definition, so the two
 * cannot drift into disagreeing about what a descriptor looks like.
 */
inline auto WriteDescriptorArchive(
    const QString& destination,
    const QVector<QPair<QString, QByteArray>>& members) -> bool {
  QFile out(destination);
  if (!out.open(QIODevice::WriteOnly)) return false;

  auto exchanger = CreateStandardGFDataExchanger();
  GFError archive_error = 0;
  std::thread producer([&]() {
    qsizetype index = 0;
    archive_error = ArchiveFileOperator::NewArchiveFromMembersSync(
        [&](ArchiveMemberEntry& entry) {
          if (index >= members.size()) return false;
          const auto& m = members.at(index++);
          entry.relative_path = m.first;
          entry.bytes = GFBuffer(m.second);
          return true;
        },
        exchanger, ArchiveCompression::kNONE, ArchiveFormat::kZIP);
  });

  std::array<std::byte, 64 * 1024> chunk{};
  while (true) {
    const auto n = exchanger->Read(chunk.data(), chunk.size());
    if (n <= 0) break;
    out.write(reinterpret_cast<const char*>(chunk.data()), n);
  }
  producer.join();
  out.close();
  return archive_error == 0;
}

/// Read every member of a descriptor out, in order.
inline auto ReadDescriptorMembers(const QString& source,
                                  QVector<QPair<QString, QByteArray>>& members)
    -> bool {
  QTemporaryDir nowhere;
  if (!nowhere.isValid()) return false;
  const auto error = ArchiveFileOperator::ExtractArchiveFromFileSync(
      source, nowhere.path(), ArchiveExtractPolicy::Permissive(),
      [](const QString&) { return true; },
      [&](const QString& path, const GFBuffer& bytes) {
        members.append({path, bytes.ConvertToQByteArray()});
        return true;
      });
  return error == 0;
}

}  // namespace GpgFrontend::Test
