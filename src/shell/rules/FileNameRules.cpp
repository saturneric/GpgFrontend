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

#include "shell/rules/FileNameRules.h"

#include "core/typedef/GpgTypedef.h"

namespace GpgFrontend {

namespace {

auto ChopSuffixIfEndsWith(QString path, const QString& suffix)
    -> std::optional<QString> {
  if (!path.endsWith(suffix, Qt::CaseInsensitive)) {
    return std::nullopt;
  }

  path.chop(suffix.size());
  return path;
}

auto RemoveKnownArchiveEncryptedSuffix(const QString& input_path) -> QString {
  if (auto out = ChopSuffixIfEndsWith(input_path, u".tar.gpg"_s)) {
    return *out;
  }

  if (auto out = ChopSuffixIfEndsWith(input_path, u".tar.asc"_s)) {
    return *out;
  }

  if (auto out = ChopSuffixIfEndsWith(input_path, u".tar.pgp"_s)) {
    return *out;
  }

  if (auto out = ChopSuffixIfEndsWith(input_path, u".gpg"_s)) {
    if (out->endsWith(u".tar"_s, Qt::CaseInsensitive)) {
      out->chop(u".tar"_s.size());
    }
    return *out;
  }

  if (auto out = ChopSuffixIfEndsWith(input_path, u".asc"_s)) {
    if (out->endsWith(u".tar"_s, Qt::CaseInsensitive)) {
      out->chop(u".tar"_s.size());
    }
    return *out;
  }

  if (auto out = ChopSuffixIfEndsWith(input_path, u".pgp"_s)) {
    if (out->endsWith(u".tar"_s, Qt::CaseInsensitive)) {
      out->chop(u".tar"_s.size());
    }
    return *out;
  }

  return input_path + u".out"_s;
}

}  // namespace

auto IsOpenPGPMessageFile(const QFileInfo& info) -> bool {
  // Case insensitive: a file named KEY.ASC is the same thing as key.asc.
  const auto suffix = info.suffix().toLower();
  return suffix == u"gpg"_s || suffix == u"pgp"_s || suffix == u"asc"_s;
}

auto IsOpenPGPRelatedFile(const QFileInfo& info) -> bool {
  return IsOpenPGPMessageFile(info) || IsOpenPGPSignatureFile(info);
}

auto IsOpenPGPSignatureFile(const QFileInfo& info) -> bool {
  return info.suffix().toLower() == u"sig"_s;
}

auto IsOpenPGPArchiveFile(const QFileInfo& info) -> bool {
  return IsOpenPGPMessageFile(info) &&
         QFileInfo(info.completeBaseName()).suffix().toLower() == u"tar"_s;
}

auto SetExtensionOfOutputFile(const QString& path, GpgOperation opera,
                              bool ascii) -> QString {
  const QFileInfo file_info(path);
  const auto input_path = file_info.absoluteFilePath();

  switch (opera) {
    case kENCRYPT:
    case kENCRYPT_SIGN:
      return input_path + (ascii ? u".asc"_s : u".gpg"_s);

    case kSIGN:
      return input_path + (ascii ? u".asc"_s : u".sig"_s);

    case kDECRYPT:
    case kDECRYPT_VERIFY: {
      auto out_path = input_path;

      if (out_path.endsWith(u".tar.gpg"_s, Qt::CaseInsensitive)) {
        out_path.chop(u".gpg"_s.size());
      } else if (out_path.endsWith(u".tar.asc"_s, Qt::CaseInsensitive)) {
        out_path.chop(u".asc"_s.size());
      } else if (out_path.endsWith(u".gpg"_s, Qt::CaseInsensitive)) {
        out_path.chop(u".gpg"_s.size());
      } else if (out_path.endsWith(u".pgp"_s, Qt::CaseInsensitive)) {
        out_path.chop(u".pgp"_s.size());
      } else if (out_path.endsWith(u".asc"_s, Qt::CaseInsensitive)) {
        out_path.chop(u".asc"_s.size());
      } else {
        out_path += u".out"_s;
      }

      if (out_path == input_path || out_path.isEmpty()) {
        out_path = input_path + u".out"_s;
      }

      return out_path;
    }

    default:
      return input_path + u".out"_s;
  }
}

auto SetExtensionOfOutputFileForArchive(const QString& path, GpgOperation opera,
                                        bool ascii) -> QString {
  const QFileInfo file_info(path);
  const auto input_path = file_info.absoluteFilePath();

  switch (opera) {
    case kENCRYPT:
    case kENCRYPT_SIGN: {
      QString out_path = input_path;

      if (!file_info.fileName().endsWith(u".tar"_s, Qt::CaseInsensitive)) {
        out_path += u".tar"_s;
      }

      out_path += ascii ? u".asc"_s : u".gpg"_s;
      return out_path;
    }

    case kDECRYPT:
    case kDECRYPT_VERIFY: {
      return RemoveKnownArchiveEncryptedSuffix(input_path);
    }

    default:
      return input_path + u".out"_s;
  }
}

}  // namespace GpgFrontend
