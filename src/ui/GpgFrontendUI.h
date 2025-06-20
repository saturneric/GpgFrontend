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

extern "C" {
auto GF_UI_EXPORT GFUIValidateSymbol() -> int;
}

// qt
#include <QtWidgets>

// private declare area of ui
#ifdef GF_UI_PRIVATE

// declare logging category
Q_DECLARE_LOGGING_CATEGORY(ui)

#define LOG_D() qCDebug(ui)
#define LOG_I() qCInfo(ui)
#define LOG_W() qCWarning(ui)
#define LOG_E() qCCritical(ui)
#define LOG_F() qCFatal(ui)

#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
#define LOG_F(...) qCFatal(ui)
#else
#define LOG_F(...) qFatal()
#endif

#define FLOG_D(...) qCDebug(ui, __VA_ARGS__)
#define FLOG_I(...) qCInfo(ui, __VA_ARGS__)
#define FLOG_W(...) qCWarning(ui, __VA_ARGS__)
#define FLOG_E(...) qCCritical(ui, __VA_ARGS__)

#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
#define FLOG_F(...) qCFatal(ui, __VA_ARGS__)
#else
#define FLOG_F(...) qFatal(__VA_ARGS__)
#endif

#endif
