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

#include "shell/rules/FileOperationRules.h"

#include "shell/rules/FileNameRules.h"

namespace GpgFrontend::Shell {

auto FileOperationAllowed(FileOperation op, const QList<FileOperand>& operands)
    -> bool {
  if (operands.isEmpty()) return false;

  const auto all = [&operands](auto&& pred) {
    return std::all_of(operands.cbegin(), operands.cend(), pred);
  };
  const auto related = [](const FileOperand& o) {
    return IsOpenPGPRelatedFile(QFileInfo(o.name));
  };

  switch (op) {
    case FileOperation::kEncrypt:
      return all([&](const FileOperand& o) {
        return (o.is_dir || o.is_file) && !related(o);
      });
    case FileOperation::kSign:
      return all(
          [&](const FileOperand& o) { return o.is_file && !related(o); });
    case FileOperation::kDecrypt:
      return all([](const FileOperand& o) {
        return o.is_file && IsOpenPGPMessageFile(QFileInfo(o.name));
      });
    case FileOperation::kVerify:
      return all([&](const FileOperand& o) { return o.is_file && related(o); });
  }
  return false;
}

auto MatchVerifyFile(const QString& name) -> VerifyFileMatch {
  const auto lower = name.toLower();
  if (lower.endsWith(u".gpg"_s) || lower.endsWith(u".pgp"_s)) {
    return {.role = VerifyFileRole::kCombined};
  }
  if (lower.endsWith(u".sig"_s) || lower.endsWith(u".asc"_s)) {
    return {.role = VerifyFileRole::kSignature,
            .partner_names = {name.chopped(4)}};
  }
  return {.role = VerifyFileRole::kData,
          .partner_names = {name + u".sig"_s, name + u".asc"_s}};
}

}  // namespace GpgFrontend::Shell
