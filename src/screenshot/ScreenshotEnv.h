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
#include <QDateTime>
#include <QJsonObject>
#include <QList>
#include <QSize>
#include <QString>
#include <QStringList>
#include <optional>

#include "screenshot/ScreenshotOptions.h"

/**
 * @file ScreenshotEnv.h
 * @brief The rendering environment: what it is, whether it is acceptable,
 *        and a hash that changes exactly when a render could.
 *
 * The hashed subset and its serialization are a contract with the manual's
 * pipeline, which recomputes the same hash in Python. Change them together
 * or not at all.
 */

namespace GpgFrontend::Screenshot {

/**
 * @brief Compact JSON with keys in sorted order. QJsonObject keeps its keys
 *        sorted, so this is Python's json.dumps(sort_keys=True,
 *        separators=(",", ":"), ensure_ascii=False) for the ASCII keys and
 *        integer numbers the record uses.
 */
auto CanonicalJson(const QJsonObject& object) -> QByteArray;

/**
 * @brief The subset of an env record that decides how a render looks.
 *
 * Paths, display names, the screen position, the build commit and the
 * kernel version are left out: they differ between runs that render the same.
 */
auto EnvHashSubset(const QJsonObject& env) -> QJsonObject;

/**
 * @brief sha256 hex of the canonical hashed subset.
 */
auto EnvHashCore(const QJsonObject& env) -> QString;

/**
 * @brief Read one component's verdict from `gpgconf --check-options`.
 *
 * gpgconf exits 0 even when an option is unknown; the verdict is the fifth
 * colon-separated field of the component's line, "1" for accepted.
 *
 * @return nullopt when there is no parsable line for @p component
 */
auto ParseCheckOptions(const QString& output, const QString& component)
    -> std::optional<bool>;

/**
 * @brief One demo key as resource/screenshot/demo-keys.json describes it.
 */
struct DemoKey {
  QString name;
  QString fingerprint;
  qint64 created = 0;
  std::optional<qint64> expires;
  bool revoked = false;
};

/**
 * @brief An extra demo key database a scenario can ask for.
 */
struct DemoDatabase {
  QString id;       ///< as scenarios name it: "work", "lab"
  QString name;     ///< what the application shows
  QString backend;  ///< "gnupg": a committed homedir; "rpgp": keys to import
  QStringList imports;  ///< armored files imported into an rpgp database
  QList<DemoKey> keys;
};

struct DemoData {
  QList<DemoKey> keys;  ///< the built-in database
  QList<DemoDatabase> databases;
  qint64 newest_timestamp = 0;

  [[nodiscard]] auto Database(const QString& id) const -> const DemoDatabase*;
};

auto ParseDemoData(const QByteArray& json, QString* error)
    -> std::optional<DemoData>;

/**
 * @brief What GnuPG should say about a demo key at @p clock.
 */
struct ExpectedKeyState {
  QString name;
  QString fingerprint;
  bool expired = false;
  bool revoked = false;
};

auto ExpectedKeyStates(const QList<DemoKey>& keys, const QDateTime& clock)
    -> QList<ExpectedKeyState>;

/**
 * @brief The measured facts the gates judge.
 */
struct GateInputs {
  qreal dpr = 0;
  int logical_dpi = 0;
  int tz_offset_seconds = 0;
  QStringList requested_families;
  QStringList resolved_families;
};

/**
 * @brief Every gate that fails, as stable tokens ("dpr", "logical-dpi",
 *        "timezone", "font:<family>"). Empty when the environment is fit.
 */
auto EvaluateGates(const GateInputs& inputs) -> QStringList;

/**
 * @brief The size gate on its own: it can only be judged once the target
 *        is on screen.
 */
auto SizeGateFailure(const QSize& requested, const QSize& actual)
    -> std::optional<QString>;

}  // namespace GpgFrontend::Screenshot
