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

#include "core/profile/ProfileAccessor.h"

#include "core/function/GFBufferFactory.h"
#include "core/utils/FilesystemUtils.h"

namespace GpgFrontend {

namespace {

/// The directory an area occupies under a filesystem root. Internal: a driver
/// that does not store in directories has no use for it, and nothing outside
/// should be assembling paths of its own anyway.
auto ProfileAreaDirName(ProfileArea area) -> QString {
  switch (area) {
    case ProfileArea::kRoot:
      return {};
    case ProfileArea::kConfig:
      return "config";
    case ProfileArea::kDataObjects:
      return "data_objs";
    case ProfileArea::kSecure:
      return "secure";
    case ProfileArea::kLogs:
      return "logs";
    case ProfileArea::kModules:
      return "mods";
    case ProfileArea::kWorkspace:
      return "workspace";
  }
  return {};
}

}  // namespace

FsProfileAccessor::FsProfileAccessor(QString root, QString settings_file)
    : root_(std::move(root)), settings_file_(std::move(settings_file)) {}

auto FsProfileAccessor::Driver() const -> QString { return "fs"; }

auto FsProfileAccessor::PathOf(ProfileArea area, const QString& name) const
    -> QString {
  const auto dir = ProfileAreaDirName(area);
  auto path = dir.isEmpty() ? root_ : root_ + "/" + dir;
  if (!name.isEmpty()) path += "/" + name;
  return path;
}

auto FsProfileAccessor::Ensure(ProfileArea area) -> bool {
  const auto path = PathOf(area);
  if (QDir(path).exists()) return true;
  if (QDir(path).mkpath(".")) return true;

  LOG_E() << "cannot create profile area:" << path;
  return false;
}

auto FsProfileAccessor::Read(ProfileArea area, const QString& name) const
    -> GFBufferOrNone {
  const auto path = PathOf(area, name);
  if (!QFileInfo::exists(path)) return {};
  return GFBufferFactory::FromFile(path);
}

auto FsProfileAccessor::Write(ProfileArea area, const QString& name,
                              const GFBuffer& value) -> bool {
  // Atomic: everything stored here is a whole-file replace, and a truncated
  // one reads back exactly like a file written with a key we no longer hold.
  return GFBufferFactory::ToFileAtomic(PathOf(area, name), value);
}

auto FsProfileAccessor::Remove(ProfileArea area, const QString& name) -> bool {
  const auto path = PathOf(area, name);
  if (!QFileInfo::exists(path)) return true;
  if (QFile::remove(path)) return true;

  LOG_W() << "cannot remove from profile area:" << path;
  return false;
}

auto FsProfileAccessor::Exists(ProfileArea area, const QString& name) const
    -> bool {
  return QFileInfo::exists(PathOf(area, name));
}

auto FsProfileAccessor::List(ProfileArea area, const QString& pattern) const
    -> QStringList {
  QDir dir(PathOf(area));
  if (!dir.exists()) return {};
  return dir.entryList({pattern}, QDir::Files | QDir::NoSymLinks);
}

auto FsProfileAccessor::TotalSize(ProfileArea area,
                                  const QString& pattern) const -> qint64 {
  return GetFileSizeByPath(PathOf(area), pattern);
}

auto FsProfileAccessor::Settings() const -> QSettings {
  if (settings_file_.isEmpty()) return {};
  return {settings_file_, QSettings::IniFormat};
}

}  // namespace GpgFrontend
