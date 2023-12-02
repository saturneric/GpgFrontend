/* pinentryconfirm.h - A QMessageBox with a timeout
 *
 * Copyright (C) 2011 Ben Kibbey <bjk@luxsci.net>
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

#ifndef PINENTRYCONFIRM_H
#define PINENTRYCONFIRM_H

#include <QAccessible>
#include <QMessageBox>
#include <QTimer>

class PinentryConfirm : public QMessageBox
#ifndef QT_NO_ACCESSIBILITY
    , public QAccessible::ActivationObserver
#endif
{
    Q_OBJECT
public:
    PinentryConfirm(Icon icon, const QString &title, const QString &text,
                    StandardButtons buttons = NoButton, QWidget *parent = nullptr,
                    Qt::WindowFlags flags = Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
    ~PinentryConfirm() override;

    void setTimeout(std::chrono::seconds timeout);
    std::chrono::seconds timeout() const;

    bool timedOut() const;

protected:
    void showEvent(QShowEvent *event) override;

private Q_SLOTS:
    void slotTimeout();

private:
#ifndef QT_NO_ACCESSIBILITY
    void accessibilityActiveChanged(bool active) override;
#endif

private:
    QTimer _timer;
    bool _timed_out = false;
};

#endif
