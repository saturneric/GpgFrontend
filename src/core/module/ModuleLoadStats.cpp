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

#include "ModuleLoadStats.h"

#include <QDateTime>

namespace GpgFrontend::Module {

auto ModuleLoadStats::GetInstance() -> ModuleLoadStats& {
  static ModuleLoadStats instance;
  return instance;
}

void ModuleLoadStats::AddHashedBytes(qint64 bytes) {
  if (bytes > 0) hashed_bytes_.fetch_add(bytes, std::memory_order_relaxed);
}

void ModuleLoadStats::AddLoadedModule() {
  loaded_.fetch_add(1, std::memory_order_relaxed);
}

void ModuleLoadStats::AddRefusedModule() {
  refused_.fetch_add(1, std::memory_order_relaxed);
}

void ModuleLoadStats::Begin() {
  began_ms_.store(QDateTime::currentMSecsSinceEpoch(),
                  std::memory_order_relaxed);
}

auto ModuleLoadStats::Summary() const -> QString {
  const auto began = began_ms_.load(std::memory_order_relaxed);
  if (began == 0) return {};

  const auto elapsed = QDateTime::currentMSecsSinceEpoch() - began;
  const auto hashed = hashed_bytes_.load(std::memory_order_relaxed);

  return QString("loaded %1 module(s), refused %2, hashed %3 MiB, %4 ms")
      .arg(loaded_.load(std::memory_order_relaxed))
      .arg(refused_.load(std::memory_order_relaxed))
      .arg(QString::number(static_cast<double>(hashed) / (1024.0 * 1024.0),
                           'f', 1))
      .arg(elapsed);
}

}  // namespace GpgFrontend::Module
