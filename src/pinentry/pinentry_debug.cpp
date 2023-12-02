/* pinentry_debug.h - Logging category for pinentry
 * Copyright (C) 2021 g10 Code GmbH
 *
 * Software engineering by Ingo Klöcker <dev@ingo-kloecker.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses/>.
 * SPDX-License-Identifier: GPL-2.0+
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "pinentry_debug.h"

#if QT_VERSION >= QT_VERSION_CHECK(5, 4, 0)
Q_LOGGING_CATEGORY(PINENTRY_LOG, "gpg.pinentry", QtWarningMsg)
#else
Q_LOGGING_CATEGORY(PINENTRY_LOG, "gpg.pinentry")
#endif
