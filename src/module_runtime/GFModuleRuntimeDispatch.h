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
#include <QHash>
#include <QMap>
#include <QString>

#include "GFSDKTypes.h"
#include "include/GFModuleEvent.h"

/// One delivered event, owned. Values are octets, never decoded as text here.
struct GFEventData {
  QString id;
  QString trigger_id;
  QMap<QString, QByteArray> params;
};

/**
 * @brief Builds GFEvent values. The only thing allowed to.
 *
 * GFEvent's data member is private so a handler cannot fabricate an event and
 * answer it, which would answer a trigger id the host never issued.
 */
class GFEventFactory {
 public:
  /// Consume a delivered event, taking ownership of every buffer in it.
  ///
  /// The host allocates the event, its two ids and every parameter node with
  /// the SDK allocator and hands them over; freeing them is the module side's
  /// obligation and always was. This is the one place that happens now,
  /// instead of once per module.
  static auto Consume(GFModuleEvent* event) -> GFEvent;
};

namespace gf::runtime {

/// Send one answer for a trigger. Used by GFEventAnswer and by the runtime's
/// own auto-answer path, so the two cannot drift.
void SendAnswer(const QString& event_id, const QString& trigger_id,
                const QMap<QString, QString>& params);

/// Turn a handler verdict into the parameters that go on the wire.
///
/// kOK becomes {"ret","0"}; everything else becomes {"ret","-1"} plus "err",
/// which is what every Host-side consumer of an answer reads.
auto ResultToParams(const GFEventResult& result) -> QMap<QString, QString>;

/// The table the module handed over, indexed for dispatch. Built once.
auto HookTable() -> QHash<QString, GFEventHook>&;

}  // namespace gf::runtime
