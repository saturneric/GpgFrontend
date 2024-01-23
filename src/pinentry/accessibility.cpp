/* accessibility.cpp - Helpers for making pinentry accessible
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

#include "accessibility.h"

#include <QString>
#include <QWidget>

namespace Accessibility {

void setDescription(QWidget *w, const QString &text) {
  if (w) {
#ifndef QT_NO_ACCESSIBILITY
    w->setAccessibleDescription(text);
#endif
  }
}

void setName(QWidget *w, const QString &text) {
  if (w) {
#ifndef QT_NO_ACCESSIBILITY
    w->setAccessibleName(text);
#endif
  }
}

}  // namespace Accessibility
