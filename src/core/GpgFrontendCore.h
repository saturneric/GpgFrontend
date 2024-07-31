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

// project base header
#include "GpgFrontend.h"

// symbol exports header
#include "core/GpgFrontendCoreExport.h"

// qt
#include <QtCore>

// private declare area of core
#ifdef GF_CORE_PRIVATE

// declare logging category
Q_DECLARE_LOGGING_CATEGORY(core)

#define LOG_D() qCDebug(core)
#define LOG_I() qCInfo(core)
#define LOG_W() qCWarning(core)
#define LOG_E() qCCritical(core)
#define LOG_F() qCFatal(core)

#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
#define LOG_F(...) qCFatal(core)
#else
#define LOG_F(...) qFatal()
#endif

#define FLOG_D(...) qCDebug(core, __VA_ARGS__)
#define FLOG_I(...) qCInfo(core, __VA_ARGS__)
#define FLOG_W(...) qCWarning(core, __VA_ARGS__)
#define FLOG_E(...) qCCritical(core, __VA_ARGS__)

#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
#define FLOG_F(...) qCFatal(core, __VA_ARGS__)
#else
#define FLOG_F(...) qFatal(__VA_ARGS__)
#endif

#endif