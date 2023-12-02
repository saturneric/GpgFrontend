/* capslock_unix.cpp - Helper to check whether Caps Lock is on
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
#include "config.h"
#endif

#include "capslock.h"

#ifdef PINENTRY_QT_WAYLAND
#include <KWayland/Client/connection_thread.h>
#include <KWayland/Client/keyboard.h>
#include <KWayland/Client/registry.h>
#include <KWayland/Client/seat.h>
#endif

#include <QGuiApplication>

#ifdef PINENTRY_QT_X11
#include <X11/XKBlib.h>

#include <QX11Info>
#undef Status
#endif

#include <QDebug>

#ifdef PINENTRY_QT_WAYLAND
using namespace KWayland::Client;
#endif

#ifdef PINENTRY_QT_WAYLAND
static bool watchingWayland = false;
#endif

LockState capsLockState() {
  static bool reportUnsupportedPlatform = true;
#ifdef PINENTRY_QT_X11
  if (qApp->platformName() == QLatin1String("xcb")) {
    unsigned int state;
    XkbGetIndicatorState(QX11Info::display(), XkbUseCoreKbd, &state);
    return (state & 0x01) == 1 ? LockState::On : LockState::Off;
  }
#endif
#ifdef PINENTRY_QT_WAYLAND
  if (qApp->platformName() == QLatin1String("wayland")) {
    if (!watchingWayland && reportUnsupportedPlatform) {
      qDebug() << "Use CapsLockWatcher for checking for Caps Lock on Wayland";
    }
  } else
#endif
      if (reportUnsupportedPlatform) {
    qWarning() << "Checking for Caps Lock not possible on unsupported platform:"
               << qApp->platformName();
  }
  reportUnsupportedPlatform = false;
  return LockState::Unknown;
}

#ifdef PINENTRY_QT_WAYLAND
void CapsLockWatcher::Private::watchWayland() {
  watchingWayland = true;
  auto connection = ConnectionThread::fromApplication(q);
  if (!connection) {
    qWarning() << "Failed to get connection to Wayland server from QPA";
    return;
  }
  registry = new Registry{q};
  registry->create(connection);
  if (!registry->isValid()) {
    qWarning() << "Failed to create valid KWayland registry";
    return;
  }
  registry->setup();

  connect(registry, &Registry::seatAnnounced, q,
          [this](quint32 name, quint32 version) {
            registry_seatAnnounced(name, version);
          });
}

void CapsLockWatcher::Private::registry_seatAnnounced(quint32 name,
                                                      quint32 version) {
  Q_ASSERT(registry);
  seat = registry->createSeat(name, version, q);
  if (!seat->isValid()) {
    qWarning() << "Failed to create valid KWayland seat";
    return;
  }

  connect(seat, &Seat::hasKeyboardChanged, q,
          [this](bool hasKeyboard) { seat_hasKeyboardChanged(hasKeyboard); });
}

void CapsLockWatcher::Private::seat_hasKeyboardChanged(bool hasKeyboard) {
  Q_ASSERT(seat);

  if (!hasKeyboard) {
    qDebug() << "Seat has no keyboard";
    return;
  }

  auto keyboard = seat->createKeyboard(q);
  if (!keyboard->isValid()) {
    qWarning() << "Failed to create valid KWayland keyboard";
    return;
  }

  connect(keyboard, &Keyboard::modifiersChanged, q,
          [this](quint32, quint32, quint32 locked, quint32) {
            keyboard_modifiersChanged(locked);
          });
}

void CapsLockWatcher::Private::keyboard_modifiersChanged(quint32 locked) {
  const bool capsLockIsLocked = (locked & 2u) != 0;
  qDebug() << "Caps Lock is locked:" << capsLockIsLocked;
  Q_EMIT q->stateChanged(capsLockIsLocked);
}
#endif
