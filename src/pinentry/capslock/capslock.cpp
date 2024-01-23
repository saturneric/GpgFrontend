/* capslock.cpp - Helper to check whether Caps Lock is on
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

#include <QDebug>
#include <QGuiApplication>

#include "capslock.h"

CapsLockWatcher::Private::Private(CapsLockWatcher *q) : q{q} {
#ifdef PINENTRY_QT_WAYLAND
  if (qApp->platformName() == QLatin1String("wayland")) {
    watchWayland();
  }
#endif
}

CapsLockWatcher::CapsLockWatcher(QObject *parent)
    : QObject{parent}, d{new Private{this}} {
  if (qApp->platformName() == QLatin1String("wayland")) {
#ifndef PINENTRY_QT_WAYLAND
    qWarning() << "CapsLockWatcher was compiled without support for Wayland";
#endif
  }
}
