/*
 *      helppage.h
 *
 *      Copyright 2008 gpg4usb-team <gpg4usb@cpunk.de>
 *
 *      This file is part of gpg4usb.
 *
 *      Gpg4usb is free software: you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation, either version 3 of the License, or
 *      (at your option) any later version.
 *
 *      Gpg4usb is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with gpg4usb.  If not, see <http://www.gnu.org/licenses/>
 */

#ifndef HELPPAGE_H
#define HELPPAGE_H

#include <QWidget>
#include <QTextBrowser>
#include <QVBoxLayout>
#include <QSettings>
#include <QFile>
#include <QLocale>

class HelpPage : public QWidget
{
    Q_OBJECT
public:
    explicit HelpPage(const QString path, QWidget *parent = 0);
    QTextBrowser *getBrowser();

signals:

public slots:
    void slotOpenUrl(QUrl url);

private:
    QTextBrowser *browser; /** The textbrowser of the tab */
    QUrl localizedHelp(QUrl path);

};

#endif // HELPPAGE_H
