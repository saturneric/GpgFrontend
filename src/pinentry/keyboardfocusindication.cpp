/* keyboardfocusindication.cpp - Helper for extended keyboard focus indication.
 * Copyright (C) 2022 g10 Code GmbH
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

#include "keyboardfocusindication.h"

#include "focusframe.h"

#include <QApplication>

KeyboardFocusIndication::KeyboardFocusIndication(QObject *parent)
    : QObject{parent}
{
    connect(qApp, &QApplication::focusChanged, this, &KeyboardFocusIndication::updateFocusFrame);
}

void KeyboardFocusIndication::updateFocusFrame(QWidget *, QWidget *focusWidget)
{
    if (focusWidget && focusWidget->inherits("QLabel") && focusWidget->window()->testAttribute(Qt::WA_KeyboardFocusChange)) {
        if (!focusFrame) {
            focusFrame = new FocusFrame{focusWidget};
        }
        focusFrame->setWidget(focusWidget);
    } else if (focusFrame) {
        focusFrame->setWidget(nullptr);
    }
}
