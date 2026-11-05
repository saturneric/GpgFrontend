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

#include <QObject>

namespace GpgFrontend::Mobile {

/**
 * @brief Tells every controller of one kind that something they all show
 * changed, whichever of them changed it: each QML page makes its own
 * controller, and they read the same store.
 *
 * One per kind of change, kept where it is used (a static of the .cpp).
 */
class ChangeRelay : public QObject {
  Q_OBJECT

 signals:
  void SignalChanged();
};

/**
 * @brief The language changed, while the application runs (Settings): every
 * model that hands QML words it translated itself says its data again.
 * QQmlEngine::retranslate() reaches only the pages' own qsTr() bindings.
 */
auto LanguageRelay() -> ChangeRelay&;

}  // namespace GpgFrontend::Mobile
