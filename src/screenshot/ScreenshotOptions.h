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

#include <QDateTime>
#include <QSize>
#include <QString>
#include <QStringList>
#include <cstdint>
#include <optional>

/**
 * @file ScreenshotOptions.h
 * @brief What the hidden screenshot mode is asked to do, and how it reports.
 *
 * Kept free of widgets and of the running application so that every rule
 * here can be tested on its own.
 */

namespace GpgFrontend::Screenshot {

/**
 * @brief Process exit codes. The manual's pipeline keys its decisions on
 *        these, so their meaning is fixed.
 */
enum ExitCode : std::uint8_t {
  kEXIT_OK = 0,
  kEXIT_USAGE = 2,             ///< bad arguments or a failed precondition
  kEXIT_TIMEOUT = 3,           ///< readiness timeout or an unstable frame
  kEXIT_OUTPUT = 4,            ///< the image or its JSON cannot be written
  kEXIT_UNEXPECTED_MODAL = 5,  ///< a window the scenario did not expect
  kEXIT_ENVIRONMENT = 6,       ///< a rendering environment gate failed
};

enum class Theme : std::uint8_t { kLIGHT, kDARK };

/**
 * @brief The parsed command line of one screenshot run.
 */
struct Options {
  QString scenario;  ///< empty for an environment probe
  bool env_probe = false;
  QString locale = QStringLiteral("en_US");
  QString out;  ///< the PNG, or the env record JSON for a probe
  std::optional<QSize>
      size;  ///< logical pixels; the scenario's default if unset
  Theme theme = Theme::kLIGHT;
  QStringList font_families;
  QDateTime clock;  ///< UTC
  QString demo_gnupghome;
  QString fixtures;
  int timeout_ms = 30000;
};

/**
 * @brief Why a run cannot go ahead. @c reason is a stable token the pipeline
 *        may match on; @c detail is for people.
 */
struct Failure {
  int exit_code = kEXIT_USAGE;
  QString reason;
  QString detail;
};

constexpr auto kDefaultClock = "2026-01-15T10:00:00Z";
constexpr auto kDefaultFontFamilies = "Noto Sans,Noto Sans CJK SC";

/**
 * @brief Parse "WxH" with both sides positive.
 */
auto ParseSize(const QString& text) -> std::optional<QSize>;

auto ParseTheme(const QString& text) -> std::optional<Theme>;

auto ThemeName(Theme theme) -> QString;

/**
 * @brief Parse exactly "yyyy-MM-ddTHH:mm:ssZ". Anything looser is refused, so
 *        a clock can never be read in local time by accident.
 */
auto ParseClock(const QString& text) -> std::optional<QDateTime>;

/**
 * @brief Split a comma separated family list, dropping blanks.
 */
auto ParseFontFamilies(const QString& text) -> QStringList;

/**
 * @brief The clock as GnuPG's faked-system-time wants it, frozen.
 */
auto GpgFakedSystemTime(const QDateTime& clock) -> QString;

/**
 * @brief Where the geometry JSON of a render goes: the PNG's stem with a
 *        ".json" extension.
 */
auto GeometryJsonPath(const QString& png_path) -> QString;

}  // namespace GpgFrontend::Screenshot
