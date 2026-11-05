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

#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <array>
#include <utility>

#include "core/GFConstants.h"
#include "core/model/GFBuffer.h"
#include "core/utils/GpgUtils.h"
#include "core/utils/RustUtils.h"
#include "shell/rules/FileNameRules.h"

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

namespace {

/// How much of a file is read to tell what it is: an armor header, or the
/// first packet's header and the start of its body.
constexpr qint64 kHeadBytes = 16 * 1024;

/// ClassifyText()'s kinds as a file's.
auto FileKindOfText(const QString& kind) -> QString {
  if (kind == QStringLiteral("message") || kind == QStringLiteral("signed")) {
    return QStringLiteral("message");
  }
  if (kind == QStringLiteral("signature")) return QStringLiteral("signature");
  if (kind == QStringLiteral("public_key") ||
      kind == QStringLiteral("private_key")) {
    return QStringLiteral("key");
  }
  return {};
}

/// What data starting with a packet of @p tag is (RFC 9580 tags).
auto FileKindOfTag(int tag) -> QString {
  switch (tag) {
    case 1:   // public-key encrypted session key
    case 3:   // symmetric-key encrypted session key
    case 4:   // one-pass signature: signed data follows
    case 8:   // compressed data
    case 9:   // symmetrically encrypted data
    case 11:  // literal data
    case 18:  // symmetrically encrypted and integrity protected data
    case 20:  // AEAD encrypted data
      return QStringLiteral("message");
    case 2:
      return QStringLiteral("signature");
    case 5:  // secret key
    case 6:  // public key
      return QStringLiteral("key");
    default:
      return {};
  }
}

}  // namespace

auto ClassifyFileHead(const QString& name, const QByteArray& head) -> QString {
  const auto by_name = ClassifyFileName(name);
  if (head.isEmpty()) return by_name;

  // Armored: its header says what it is, whatever it is called.
  const auto text_kind =
      FileKindOfText(ClassifyText(QString::fromUtf8(head.left(4096))));
  if (!text_kind.isEmpty()) return text_kind;

  // Binary, and named as OpenPGP data: its first packet confirms it.
  if (by_name.isEmpty()) return {};
  const auto doc = QJsonDocument::fromJson(InspectOpenPGPData(GFBuffer(head)));
  if (!doc.isObject()) return by_name;  // no inspector here: the name alone
  const auto blocks = doc.object().value(QStringLiteral("blocks")).toArray();
  if (blocks.isEmpty()) return {};
  const auto packets =
      blocks.first().toObject().value(QStringLiteral("packets")).toArray();
  if (packets.isEmpty()) return {};
  // A packet the inspector could not read refutes the name, unless it is
  // one of the bulk data packets that run past what was read.
  const auto first = packets.first().toObject();
  const auto tag = first.value(QStringLiteral("tag")).toInt();
  const auto bulk = tag == 8 || tag == 9 || tag == 11 || tag == 18 || tag == 20;
  const auto error = first.value(QStringLiteral("error"));
  if (!bulk && !error.isNull() && !error.isUndefined()) return {};
  return FileKindOfTag(tag);
}

auto ClassifyFile(const QString& name, const QString& path) -> QString {
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly)) return ClassifyFileName(name);
  return ClassifyFileHead(name, file.read(kHeadBytes));
}

auto ContentKind::textKind(const QString& text) const -> QString {
  return ClassifyText(text);
}

}  // namespace GpgFrontend::Mobile
