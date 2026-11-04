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

#include "model/AboutInfo.h"

#include <QFile>
#include <QLocale>
#include <QSysInfo>

#include "core/utils/BuildInfoUtils.h"
#include "core/utils/RustUtils.h"

#ifdef Q_OS_ANDROID
#include <android/ndk-version.h>

#include <array>
#endif

namespace GpgFrontend::Mobile {

namespace {

/// The facts with a value, as rows.
auto Rows(const QList<QPair<QString, QString>>& facts) -> QVariantList {
  QVariantList rows;
  for (const auto& [label, value] : facts) {
    if (value.isEmpty()) continue;
    rows.append(QVariantMap{{QStringLiteral("label"), label},
                            {QStringLiteral("value"), value}});
  }
  return rows;
}

#ifdef Q_OS_ANDROID

/// "Android 9 (API 28)": the release an API level shipped with, as the
/// platform names them; the level alone past those known here.
auto AndroidRelease(int api) -> QString {
  struct Release {
    int api;
    const char* name;
  };
  static constexpr std::array<Release, 9> kReleases{{
      {28, "9"},
      {29, "10"},
      {30, "11"},
      {31, "12"},
      {32, "12L"},
      {33, "13"},
      {34, "14"},
      {35, "15"},
      {36, "16"},
  }};
  for (const auto& release : kReleases) {
    if (release.api == api) {
      return AboutInfo::tr("Android %1 (API %2)")
          .arg(QString::fromLatin1(release.name))
          .arg(api);
    }
  }
  return AboutInfo::tr("API %1").arg(api);
}

#endif

}  // namespace

AboutInfo::AboutInfo(QObject* parent) : QObject(parent) {}

auto AboutInfo::build() const -> QVariantList {
  // The desktop's list and captions (BuildInfoFacts), shown without the
  // colon the desktop's form puts after them.
  QList<QPair<QString, QString>> facts;
  for (const auto& fact : BuildInfoFacts()) {
    auto label = fact.Caption().trimmed();
    if (label.endsWith(u':') || label.endsWith(QChar(0xFF1A))) label.chop(1);
    facts.append({label, fact.value});
  }
  auto rows = Rows(facts);
  // The commit as git abbreviates it, beside its name like the others; the
  // whole of it is what is copied.
  const auto commit = GetProjectBuildGitCommitHash();
  for (auto& row : rows) {
    auto fact = row.toMap();
    if (fact.value(QStringLiteral("value")).toString() != commit) continue;
    fact.insert(QStringLiteral("full"), commit);
    fact.insert(QStringLiteral("value"), commit.left(12));
    row = fact;
  }
  return rows;
}

auto AboutInfo::androidBuild() const -> QVariantList {
#ifdef Q_OS_ANDROID
  return Rows({
#ifdef GF_ANDROID_VERSION_CODE
      {tr("Version code"), QString::number(GF_ANDROID_VERSION_CODE)},
#endif
      {tr("Processor"), QSysInfo::buildCpuArchitecture()},
#ifdef GF_ANDROID_MIN_SDK
      {tr("Needs at least"), AndroidRelease(GF_ANDROID_MIN_SDK)},
#endif
#ifdef GF_ANDROID_TARGET_SDK
      {tr("Made for"), AndroidRelease(GF_ANDROID_TARGET_SDK)},
#endif
      {tr("NDK"), QStringLiteral("%1.%2.%3")
                      .arg(__NDK_MAJOR__)
                      .arg(__NDK_MINOR__)
                      .arg(__NDK_BUILD__)},
      // The version alone: the full string names the toolchain's sources.
      {tr("Compiler"), QStringLiteral("Clang %1.%2.%3")
                           .arg(__clang_major__)
                           .arg(__clang_minor__)
                           .arg(__clang_patchlevel__)},
  });
#else
  return {};
#endif
}

auto AboutInfo::engine() const -> QVariantList {
  const auto info = RustEngineBuildInfo();
  const auto or_unknown = [](const QString& value) {
    return value.isEmpty() ? tr("Unknown") : value;
  };
  return Rows({
      {tr("Engine version"), or_unknown(info.engine_version)},
      {tr("Rust compiler"), or_unknown(info.rustc_version)},
      {tr("Target"), info.target},
      {tr("Build profile"), info.profile},
  });
}

auto AboutInfo::engineDependencies() const -> QVariantList {
  return Rows(RustEngineBuildInfo().dependencies);
}

auto AboutInfo::translators() const -> QString {
  QFile file(QStringLiteral(":/TRANSLATORS"));
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return {};
  return QString::fromUtf8(file.readAll()).trimmed();
}

}  // namespace GpgFrontend::Mobile
