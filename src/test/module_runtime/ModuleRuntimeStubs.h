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

#include <GFSDKHostApi.h>
#include <GFSDKTypes.h>

#include <QList>
#include <QMap>
#include <QString>
#include <QStringList>

namespace stubs {

/// What the fake host was asked to do.
struct Recorder {
  QStringList listened;
  QString listened_as;
  QList<QMap<QString, QString>> answers;
  QString translator_registered_for;
  GFTranslatorDataReader translator_reader = nullptr;
  QStringList warnings;
  QStringList errors;
  int allocations = 0;
  int frees = 0;

  /// Calls that arrived on a thread other than the one that built the table.
  /// The point of the context is that they are served all the same.
  int calls_off_thread = 0;

  void Reset();
};

Recorder& Rec();

/**
 * @brief A host table the runtime can be activated with.
 *
 * @param granted GF_HOST_CAP_* bits. A group whose bit is clear is left NULL,
 *        exactly as the real mint leaves it, so a test can watch an SDK
 *        wrapper refuse rather than call through.
 *
 * The returned table points at statics and stays valid for the process.
 */
auto MakeHostApi(uint32_t granted) -> GFHostApi;

/// The module id the fake host attributes every call to.
auto FakeModuleId() -> const char*;

}  // namespace stubs
