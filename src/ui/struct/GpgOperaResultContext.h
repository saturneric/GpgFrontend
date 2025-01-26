#include <utility>

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

#include "ui/UserInterfaceUtils.h"
#include "ui/struct/GpgOperaResult.h"

namespace GpgFrontend::UI {

struct GpgOperaCategory {
  QStringList paths;
  QStringList o_paths;
};

struct GpgOperaContext {
  QContainer<OperaWaitingCb>& operas;
  QContainer<GpgOperaResult>& opera_results;
  GpgKeyList& keys;
  GpgKeyList& singer_keys;
  QStringList& unknown_fprs;
  bool ascii;

  QStringList paths;
  QStringList o_paths;

  GpgOperaContext(QContainer<OperaWaitingCb>& operas,
                  QContainer<GpgOperaResult>& opera_results, GpgKeyList& keys,
                  GpgKeyList& singer_keys, QStringList& unknown_fprs)
      : operas(operas),
        opera_results(opera_results),
        keys(keys),
        singer_keys(singer_keys),
        unknown_fprs(unknown_fprs) {}
};

struct GpgOperaContexts {
  QContainer<OperaWaitingCb> operas;
  QContainer<GpgOperaResult> opera_results;
  GpgKeyList keys;
  GpgKeyList singer_keys;
  QStringList unknown_fprs;
  bool ascii;

  QMap<int, GpgOperaCategory> categories;

  auto GetContextPath(int category) -> QStringList& {
    if (!categories.contains(category)) categories[category] = {};
    return categories[category].paths;
  }

  auto GetContextOutPath(int category) -> QStringList& {
    if (!categories.contains(category)) categories[category] = {};
    return categories[category].o_paths;
  }

  auto GetAllPath() -> QStringList {
    QStringList res;

    for (auto& category : categories) {
      res.append(category.paths);
    }
    return res;
  }

  auto GetAllOutPath() -> QStringList {
    QStringList res;

    for (auto& category : categories) {
      res.append(category.o_paths);
    }
    return res;
  }

  auto GetContext(int category) -> QSharedPointer<GpgOperaContext> {
    if (GetContextPath(category).empty()) return nullptr;

    auto context = QSharedPointer<GpgOperaContext>::create(
        operas, opera_results, keys, singer_keys, unknown_fprs);
    context->ascii = ascii;

    context->paths = GetContextPath(category);
    context->o_paths = GetContextOutPath(category);
    return context;
  }
};

}  // namespace GpgFrontend::UI