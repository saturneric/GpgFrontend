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

#include <QtQml/qqmlregistration.h>

#include <QObject>
#include <QVariantList>

namespace GpgFrontend::Mobile {

/**
 * @brief What the desktop's About dialog tells of a build, for the phone's
 * About pages: the build information, the rPGP engine, the translators.
 *
 * Facts are [{label, value}], with {full} where the value shown is
 * shortened (the commit), in the desktop's order; one this build has no
 * value for (the GnuPG libraries, which a phone does not link) is left out.
 */
class AboutInfo : public QObject {
  Q_OBJECT
  QML_ELEMENT

 public:
  /// A build time as the pages show it: "2026-11-04 18:07:36 UTC".
  static auto BuildTimeUtc(const QDateTime& when) -> QString;

  explicit AboutInfo(QObject* parent = nullptr);

  /// The build: versions of what it is made of, its commit and when.
  [[nodiscard]] Q_INVOKABLE QVariantList build() const;

  /// How the Android package was built: its version code, the processor it
  /// is for, the Android versions it needs and aims at, the NDK and the
  /// compiler. Empty off Android.
  [[nodiscard]] Q_INVOKABLE QVariantList androidBuild() const;

  /// The rPGP engine compiled in: version, compiler, target, profile.
  [[nodiscard]] Q_INVOKABLE QVariantList engine() const;

  /// The engine's key dependencies, crate by crate.
  [[nodiscard]] Q_INVOKABLE QVariantList engineDependencies() const;

  /// Who translated GpgFrontend (the bundled TRANSLATORS); empty when the
  /// list is not there.
  [[nodiscard]] Q_INVOKABLE QString translators() const;
};

}  // namespace GpgFrontend::Mobile
