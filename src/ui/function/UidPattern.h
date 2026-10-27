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

namespace GpgFrontend::UI {

/**
 * @brief How a quick user id is made up: one template per part.
 *
 * Each part is literal text with placeholders in braces:
 *
 * - `{token}`: 8 random letters and digits; `{token:N}` for N of them (4-32).
 *   Every `{token}` in one user id is the same, so the name and the email can
 *   share it; a shorter one is the start of a longer one.
 * - `{domain}`: a random 8-character domain label.
 * - `{tld}`: one of the reserved top-level domains (test, example, invalid),
 *   so a made-up address can never reach anybody.
 * - `{date}`: today, as yyyyMMdd; `{date:FORMAT}` in any Qt date and time
 *   format, such as `{date:yyyy-MM-dd}` or `{date:yyMMdd-HHmm}`.
 * - `{time}`: the time of day, as HHmmss.
 * - `{timestamp}`: seconds since 1970-01-01 UTC; `{timestamp:ms}` in
 *   milliseconds.
 * - `{{` and `}}`: a literal brace.
 *
 * Deliberately nothing that names this computer or its user: a public key is
 * published, and a user id made from the login name would say who made it.
 */
struct GF_UI_EXPORT UidPattern {
  QString name = u"anon-{token}"_s;
  QString email = u"anon-{token}@{domain}.{tld}"_s;
  QString comment;

  auto operator==(const UidPattern& o) const -> bool {
    return name == o.name && email == o.email && comment == o.comment;
  }
};

/**
 * @brief A user id made from a pattern.
 */
struct GF_UI_EXPORT ExpandedUid {
  QString name;
  QString email;
  QString comment;
};

/// Where the randomness comes from; any 64 bits will do.
using UidPatternRandom = std::function<quint64()>;

/**
 * @brief The system's random source, for real use.
 */
auto GF_UI_EXPORT SystemUidPatternRandom() -> UidPatternRandom;

/**
 * @brief Fill a pattern in.
 *
 * An unknown placeholder is left as it is written; CheckUidPattern() is what
 * says it is wrong.
 *
 * @param pattern the pattern
 * @param random the random source
 * @param now what {date}, {time} and {timestamp} are; read in local time
 */
auto GF_UI_EXPORT ExpandUidPattern(const UidPattern& pattern,
                                   const UidPatternRandom& random,
                                   const QDateTime& now) -> ExpandedUid;

/**
 * @brief What is wrong with one part of a pattern.
 */
enum class UidPatternProblem {
  kNone,
  kUnknownPlaceholder,
  kUnbalancedBrace,
  kNameEmpty,     ///< the name part gives an empty name
  kInvalidText,   ///< the result holds '(', ')', '<', '>' or control chars
  kInvalidEmail,  ///< the result is not an email address
};

/**
 * @brief What is wrong with each part of a pattern.
 */
struct GF_UI_EXPORT UidPatternCheck {
  UidPatternProblem name = UidPatternProblem::kNone;
  UidPatternProblem email = UidPatternProblem::kNone;
  UidPatternProblem comment = UidPatternProblem::kNone;

  [[nodiscard]] auto Ok() const -> bool {
    return name == UidPatternProblem::kNone &&
           email == UidPatternProblem::kNone &&
           comment == UidPatternProblem::kNone;
  }
};

/**
 * @brief Whether a pattern makes a usable user id, whatever the random draw.
 *
 * The email part may be empty: a user id needs no address. The comment is
 * usually empty.
 */
auto GF_UI_EXPORT CheckUidPattern(const UidPattern& pattern) -> UidPatternCheck;

/**
 * @brief The translated sentence for a problem; empty for kNone.
 */
auto GF_UI_EXPORT DescribeUidPatternProblem(UidPatternProblem problem)
    -> QString;

/**
 * @brief Read a stored pattern; missing parts take their defaults.
 */
auto GF_UI_EXPORT UidPatternFromJson(const QJsonObject& obj) -> UidPattern;

/**
 * @brief The stored form of a pattern.
 */
auto GF_UI_EXPORT UidPatternToJson(const UidPattern& pattern) -> QJsonObject;

}  // namespace GpgFrontend::UI
