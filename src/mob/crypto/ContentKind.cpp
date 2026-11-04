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

#include "crypto/ContentKind.h"

#include <QFileInfo>
#include <array>
#include <utility>

#include "core/GFConstants.h"
#include "core/utils/GpgUtils.h"

namespace GpgFrontend::Mobile {

auto ClassifyText(const QString& text) -> QString {
  if (text.trimmed().isEmpty()) return QStringLiteral("empty");

  const std::array<std::pair<const char*, const char*>, 5> kinds{{
      {kPgpMessageBegin, "message"},
      {kPgpSignedBegin, "signed"},
      {kPgpSignatureBegin, "signature"},
      {kPgpPublicKeyBegin, "public_key"},
      {kPgpPrivateKeyBegin, "private_key"},
  }};
  // The first header at the start of a line is what the text is.
  qsizetype first = -1;
  QString kind = QStringLiteral("plain");
  for (const auto& [header, name] : kinds) {
    const auto marker = QLatin1StringView(header);
    for (qsizetype at = text.indexOf(marker); at >= 0;
         at = text.indexOf(marker, at + 1)) {
      if (at > 0 && text.at(at - 1) != QLatin1Char('\n')) continue;
      if (first < 0 || at < first) {
        first = at;
        kind = QString::fromLatin1(name);
      }
      break;
    }
  }
  return kind;
}

auto ClassifyFileName(const QString& name) -> QString {
  const QFileInfo info(name);
  if (IsOpenPGPSignatureFile(info)) return QStringLiteral("signature");
  if (IsOpenPGPMessageFile(info)) return QStringLiteral("message");
  return {};
}

auto ContentKind::textKind(const QString& text) const -> QString {
  return ClassifyText(text);
}

}  // namespace GpgFrontend::Mobile
