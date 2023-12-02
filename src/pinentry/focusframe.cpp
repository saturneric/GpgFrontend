/* focusframe.cpp - A focus indicator for labels.
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

#include "focusframe.h"

#if QT_CONFIG(graphicseffect)
#include <QGraphicsEffect>
#endif
#include <QStyleOptionFocusRect>
#include <QStylePainter>

static QRect effectiveWidgetRect(const QWidget *w)
{
    // based on QWidgetPrivate::effectiveRectFor
#if QT_CONFIG(graphicseffect)
    const auto* const graphicsEffect = w->graphicsEffect();
    if (graphicsEffect && graphicsEffect->isEnabled())
        return graphicsEffect->boundingRectFor(w->rect()).toAlignedRect();
#endif // QT_CONFIG(graphicseffect)
    return w->rect();
}

static QRect clipRect(const QWidget *w)
{
    // based on QWidgetPrivate::clipRect
    if (!w->isVisible()) {
        return QRect();
    }
    QRect r = effectiveWidgetRect(w);
    int ox = 0;
    int oy = 0;
    while (w && w->isVisible() && !w->isWindow() && w->parentWidget()) {
        ox -= w->x();
        oy -= w->y();
        w = w->parentWidget();
        r &= QRect(ox, oy, w->width(), w->height());
    }
    return r;
}

void FocusFrame::paintEvent(QPaintEvent *)
{
    if (!widget()) {
        return;
    }

    QStylePainter p{this};
    QStyleOptionFocusRect option;
    initStyleOption(&option);
    const int vmargin = style()->pixelMetric(QStyle::PM_FocusFrameVMargin, &option);
    const int hmargin = style()->pixelMetric(QStyle::PM_FocusFrameHMargin, &option);
    const QRect rect = clipRect(widget()).adjusted(0, 0, hmargin*2, vmargin*2);
    p.setClipRect(rect);
    p.drawPrimitive(QStyle::PE_FrameFocusRect, option);
}
