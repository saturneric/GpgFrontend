/* pinlineedit.h - Modified QLineEdit widget.
 * Copyright (C) 2018 Damien Goutte-Gattat
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses/>.
 * SPDX-License-Identifier: GPL-2.0+
 */

#ifndef _PINLINEEDIT_H_
#define _PINLINEEDIT_H_

#include <QLineEdit>

#include <memory>

class PinLineEdit : public QLineEdit
{
    Q_OBJECT

public:
    explicit PinLineEdit(QWidget *parent = nullptr);
    ~PinLineEdit() override;

    void setPin(const QString &pin);
    QString pin() const;

public Q_SLOTS:
    void setFormattedPassphrase(bool on);
    void copy() const;
    void cut();

Q_SIGNALS:
    void backspacePressed();

protected:
    void keyPressEvent(QKeyEvent *) override;

private:
    using QLineEdit::setText;
    using QLineEdit::text;

private Q_SLOTS:
    void textEdited();

private:
    class Private;
    std::unique_ptr<Private> d;
};

#endif // _PINLINEEDIT_H_
