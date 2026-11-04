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
#include <QVariantMap>
#include <functional>

#include "plat/common/PlatformInbox.h"

class QQmlEngine;
class QJSEngine;

namespace GpgFrontend::Mobile {

/**
 * @brief Hands what other applications shared with GpgFrontend to its
 * pages, once they are there to take it.
 *
 * The main page asks when it comes up, each time the application comes to
 * the foreground, and when a share arrives while it is already there
 * (SignalArrived): only after the profile is open is there a workspace to put
 * it in. The documents go into the FileWorkspace at once, as copies
 * (FileWorkspace::receiveFiles()); the text is handed over as it is, for the
 * page to place.
 */
class ShareInbox : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON

 public:
  using Source = std::function<SharedItems()>;

  /// Listens for shares that arrive while the application runs.
  explicit ShareInbox(QObject* parent = nullptr);

  static auto create(QQmlEngine*, QJSEngine*) -> ShareInbox*;

  /// Where shares come from: TakeSharedItems(), unless a test says else.
  static void SetSourceForTesting(Source source);

  /**
   * @brief Take what was shared since the last call.
   *
   * {"files": the ids of the copies, "text": the text, "refused": how many
   * documents could not be taken}; empty when nothing was shared.
   */
  Q_INVOKABLE QVariantMap take();

 signals:
  /// Something was shared while the application ran: take() it.
  void SignalArrived();
};

}  // namespace GpgFrontend::Mobile
