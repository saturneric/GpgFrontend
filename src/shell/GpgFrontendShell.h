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

// qt: what both interfaces have, and never a widget or a QML type
#include <QtCore>
#include <QtGui>
#include <QtNetwork>

// private declare area of shell
#ifdef GF_SHELL_PRIVATE

// declare logging category
Q_DECLARE_LOGGING_CATEGORY(shell)

#define LOG_D() qCDebug(shell)
#define LOG_I() qCInfo(shell)
#define LOG_W() qCWarning(shell)
#define LOG_E() qCCritical(shell)
#define LOG_F(...) qCFatal(shell)

#define FLOG_D(...) qCDebug(shell, __VA_ARGS__)
#define FLOG_I(...) qCInfo(shell, __VA_ARGS__)
#define FLOG_W(...) qCWarning(shell, __VA_ARGS__)
#define FLOG_E(...) qCCritical(shell, __VA_ARGS__)
#define FLOG_F(...) qCFatal(shell, __VA_ARGS__)

#endif
