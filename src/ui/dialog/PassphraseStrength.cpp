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

#include "PassphraseStrength.h"

namespace {

// Penalizes repeated characters (e.g. "aaaa") and sequential runs
// (e.g. "abcd", "4321") of three or more characters, which the
// per-character-class score would otherwise rate as strong.
auto CalculatePatternPenalty(const QString& text) -> int {
  const int length = static_cast<int>(text.size());
  if (length < 3) {
    return 0;
  }

  auto penalty = 0;
  auto repeat_run = 1;
  auto sequence_run = 1;

  for (int i = 1; i < length; ++i) {
    const auto prev = text[i - 1].unicode();
    const auto curr = text[i].unicode();

    repeat_run = curr == prev ? repeat_run + 1 : 1;
    if (repeat_run >= 3) {
      penalty += 6;
    }

    const auto step = curr - prev;
    if (i >= 2 && step == prev - text[i - 2].unicode() &&
        (step == 1 || step == -1)) {
      sequence_run += 1;
    } else {
      sequence_run = 1;
    }
    if (sequence_run >= 3) {
      penalty += 6;
    }
  }

  return penalty;
}

}  // namespace

namespace GpgFrontend::UI {

auto PassphraseStrengthDescription(int strength) -> QString {
  if (strength < 20) {
    return QObject::tr("Very weak");
  }
  if (strength < 40) {
    return QObject::tr("Weak");
  }
  if (strength < 60) {
    return QObject::tr("Fair");
  }
  if (strength < 80) {
    return QObject::tr("Good");
  }
  return QObject::tr("Strong");
}

auto PassphraseStrengthColor(int strength) -> QString {
  if (strength < 20) {
    return QStringLiteral("#e53935");
  }
  if (strength < 40) {
    return QStringLiteral("#fb8c00");
  }
  if (strength < 60) {
    return QStringLiteral("#fbc02d");
  }
  if (strength < 80) {
    return QStringLiteral("#7cb342");
  }
  return QStringLiteral("#43a047");
}

auto CalculatePassphraseStrength(const QString& text) -> int {
  if (text.isEmpty()) {
    return 0;
  }

  const int length = static_cast<int>(text.size());

  bool has_lower = false;
  bool has_upper = false;
  bool has_digit = false;
  bool has_symbol = false;

  for (const auto ch : text) {
    if (ch.isLower()) {
      has_lower = true;
    } else if (ch.isUpper()) {
      has_upper = true;
    } else if (ch.isDigit()) {
      has_digit = true;
    } else {
      has_symbol = true;
    }
  }

  auto score = 0;
  score += std::min(length * 5, 35);
  score += has_lower ? 10 : 0;
  score += has_upper ? 15 : 0;
  score += has_digit ? 15 : 0;
  score += has_symbol ? 20 : 0;

  if (length >= 12) {
    score += 5;
  }
  if (length >= 16) {
    score += 5;
  }

  score -= CalculatePatternPenalty(text);

  return std::clamp(score, 0, 100);
}

}  // namespace GpgFrontend::UI
