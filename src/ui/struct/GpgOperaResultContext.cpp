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

#include "GpgOperaResultContext.h"

#include <utility>

namespace GpgFrontend::UI {

GpgOperaContext::GpgOperaContext(QSharedPointer<GpgOperaContextBasement> base)
    : base(std::move(base)) {}

auto GpgOperaContextBasement::GetContextPath(int category) -> QStringList& {
  if (!categories.contains(category)) categories[category] = {};
  return categories[category].paths;
}

auto GpgOperaContextBasement::GetContextOutPath(int category) -> QStringList& {
  if (!categories.contains(category)) categories[category] = {};
  return categories[category].o_paths;
}

auto GpgOperaContextBasement::GetAllPath() -> QStringList {
  QStringList res;

  for (auto& category : categories) {
    res.append(category.paths);
  }
  return res;
}

auto GpgOperaContextBasement::GetAllOutPath() -> QStringList {
  QStringList res;

  for (auto& category : categories) {
    res.append(category.o_paths);
  }
  return res;
}

auto GetGpgOperaContextFromBasement(
    const QSharedPointer<GpgOperaContextBasement>& base,
    int category) -> QSharedPointer<GpgOperaContext> {
  if (base->GetContextPath(category).empty()) return nullptr;

  auto context = QSharedPointer<GpgOperaContext>::create(base);

  context->paths = base->GetContextPath(category);
  context->o_paths = base->GetContextOutPath(category);
  return context;
}
}  // namespace GpgFrontend::UI