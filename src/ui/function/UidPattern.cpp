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

#include "ui/function/UidPattern.h"

#include <optional>

#include "core/utils/CommonUtils.h"
#include "core/utils/GpgUtils.h"

namespace GpgFrontend::UI {

namespace {

constexpr int kTokenDefaultLen = 8;
constexpr int kTokenMinLen = 4;
constexpr int kTokenMaxLen = 32;
constexpr int kDomainLen = 8;

/// RFC 2606 / RFC 6761 reserved: a label under these is nobody's domain.
const std::array<const char*, 3> kReservedTlds = {"test", "example", "invalid"};

/// The values one expansion shares, drawn once.
struct Draw {
  QString token;  ///< kTokenMaxLen long; shorter tokens are its prefix
  QString domain;
  QString tld;
  QDateTime now;  ///< local time, for {date}, {time} and {timestamp}
};

auto Base36(const UidPatternRandom& random, int len) -> QString {
  static const QString kDigits = u"0123456789abcdefghijklmnopqrstuvwxyz"_s;
  QString out;
  out.reserve(len);
  for (int i = 0; i < len; ++i) {
    out.append(kDigits.at(static_cast<qsizetype>(random() % 36)));
  }
  return out;
}

/// The value of one placeholder, or nothing when it is not one.
auto Placeholder(const QString& name, const Draw& draw)
    -> std::optional<QString> {
  if (name == u"token"_s) return draw.token.left(kTokenDefaultLen);
  if (name.startsWith(u"token:"_s)) {
    bool ok = false;
    const auto len = name.mid(6).toInt(&ok);
    if (!ok || len < kTokenMinLen || len > kTokenMaxLen) return std::nullopt;
    return draw.token.left(len);
  }
  if (name == u"domain"_s) return draw.domain;
  if (name == u"tld"_s) return draw.tld;
  if (name == u"date"_s) return draw.now.toString(u"yyyyMMdd"_s);
  if (name.startsWith(u"date:"_s)) {
    // Any Qt date and time format, in the C locale so a name of a month or a
    // day reads the same whatever the language; nothing to format is no
    // placeholder.
    const auto format = name.mid(5);
    if (format.trimmed().isEmpty()) return std::nullopt;
    return QLocale::c().toString(draw.now, format);
  }
  if (name == u"time"_s) return draw.now.toString(u"HHmmss"_s);
  if (name == u"timestamp"_s) {
    return QString::number(draw.now.toSecsSinceEpoch());
  }
  if (name == u"timestamp:ms"_s) {
    return QString::number(draw.now.toMSecsSinceEpoch());
  }
  return std::nullopt;
}

/**
 * @brief Fill one part in.
 *
 * @param problem set to what is wrong with its syntax, if it is not null
 */
auto Expand(const QString& text, const Draw& draw,
            UidPatternProblem* problem = nullptr) -> QString {
  QString out;
  for (qsizetype i = 0; i < text.size(); ++i) {
    const auto c = text.at(i);
    const auto next = i + 1 < text.size() ? text.at(i + 1) : QChar();

    if ((c == u'{' && next == u'{') || (c == u'}' && next == u'}')) {
      out.append(c);
      ++i;
      continue;
    }

    if (c == u'}') {
      if (problem != nullptr) *problem = UidPatternProblem::kUnbalancedBrace;
      out.append(c);
      continue;
    }

    if (c != u'{') {
      out.append(c);
      continue;
    }

    const auto close = text.indexOf(u'}', i + 1);
    if (close < 0) {
      if (problem != nullptr) *problem = UidPatternProblem::kUnbalancedBrace;
      out.append(text.mid(i));
      break;
    }

    const auto name = text.mid(i + 1, close - i - 1).trimmed();
    if (auto value = Placeholder(name, draw)) {
      out.append(*value);
    } else {
      if (problem != nullptr && *problem == UidPatternProblem::kNone) {
        *problem = UidPatternProblem::kUnknownPlaceholder;
      }
      out.append(text.mid(i, close - i + 1));
    }
    i = close;
  }
  return out;
}

auto MakeDraw(const UidPatternRandom& random, const QDateTime& now) -> Draw {
  Draw draw;
  draw.token = Base36(random, kTokenMaxLen);
  draw.domain = Base36(random, kDomainLen);
  draw.tld = QString::fromLatin1(kReservedTlds.at(
      static_cast<std::size_t>(random() % kReservedTlds.size())));
  draw.now = now.toLocalTime();
  return draw;
}

}  // namespace

auto SystemUidPatternRandom() -> UidPatternRandom {
  return []() { return QRandomGenerator::global()->generate64(); };
}

auto ExpandUidPattern(const UidPattern& pattern, const UidPatternRandom& random,
                      const QDateTime& now) -> ExpandedUid {
  const auto draw = MakeDraw(random, now);
  return {Expand(pattern.name, draw).trimmed(),
          Expand(pattern.email, draw).trimmed(),
          Expand(pattern.comment, draw).trimmed()};
}

auto CheckUidPattern(const UidPattern& pattern) -> UidPatternCheck {
  // Any draw shows a syntax problem; the result is judged on a fixed one, and
  // the placeholders only ever give letters, digits and reserved names.
  quint64 counter = 0;
  const auto draw =
      MakeDraw([&counter]() { return ++counter; },
               QDateTime(QDate(2000, 1, 1), QTime(12, 0), QTimeZone::UTC));

  UidPatternCheck check;

  const auto name = Expand(pattern.name, draw, &check.name).trimmed();
  if (check.name == UidPatternProblem::kNone) {
    if (name.isEmpty()) {
      check.name = UidPatternProblem::kNameEmpty;
    } else if (!IsValidUserIdComponent(name)) {
      check.name = UidPatternProblem::kInvalidText;
    }
  }

  const auto email = Expand(pattern.email, draw, &check.email).trimmed();
  if (check.email == UidPatternProblem::kNone && !email.isEmpty() &&
      !IsEmailAddress(email)) {
    check.email = UidPatternProblem::kInvalidEmail;
  }

  const auto comment = Expand(pattern.comment, draw, &check.comment).trimmed();
  if (check.comment == UidPatternProblem::kNone &&
      !IsValidUserIdComponent(comment)) {
    check.comment = UidPatternProblem::kInvalidText;
  }

  return check;
}

auto DescribeUidPatternProblem(UidPatternProblem problem) -> QString {
  switch (problem) {
    case UidPatternProblem::kNone:
      return {};
    case UidPatternProblem::kUnknownPlaceholder:
      return QObject::tr(
          "Unknown placeholder. Use {token}, {token:N}, {domain}, {tld}, "
          "{date}, {date:FORMAT}, {time}, {timestamp} or {timestamp:ms}.");
    case UidPatternProblem::kUnbalancedBrace:
      return QObject::tr(
          "A brace is not closed. Write {{ or }} for a literal brace.");
    case UidPatternProblem::kNameEmpty:
      return QObject::tr("The name must not be empty.");
    case UidPatternProblem::kInvalidText:
      return QObject::tr(
          "Must not contain '(', ')', '<', '>' or control characters.");
    case UidPatternProblem::kInvalidEmail:
      return QObject::tr("This does not make a valid email address.");
  }
  return {};
}

auto UidPatternFromJson(const QJsonObject& obj) -> UidPattern {
  UidPattern pattern;
  if (const auto v = obj["name"]; v.isString()) pattern.name = v.toString();
  if (const auto v = obj["email"]; v.isString()) pattern.email = v.toString();
  if (const auto v = obj["comment"]; v.isString()) {
    pattern.comment = v.toString();
  }
  return pattern;
}

auto UidPatternToJson(const UidPattern& pattern) -> QJsonObject {
  return {
      {"name", pattern.name},
      {"email", pattern.email},
      {"comment", pattern.comment},
  };
}

}  // namespace GpgFrontend::UI
