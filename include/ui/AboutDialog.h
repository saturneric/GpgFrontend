/*
 *      aboutdialog.h
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

#ifndef __ABOUTDIALOG_H__
#define __ABOUTDIALOG_H__

#include <GPG4USB.h>

#include "gpg/GpgContext.h"

/**
 * @brief Class containing the main tab of about dialog
 *
 */
class InfoTab : public QWidget {
Q_OBJECT

public:
    explicit InfoTab(QWidget *parent = nullptr);
};

/**
 * @brief Class containing the translator tab of about dialog
 *
 */
class TranslatorsTab : public QWidget {
Q_OBJECT

public:
    explicit TranslatorsTab(QWidget *parent = nullptr);
};

/**
 * @brief Class for handling the about dialog
 *
 */
class AboutDialog : public QDialog {
Q_OBJECT

public:
    explicit AboutDialog(QWidget *parent = nullptr);
};

#endif  // __ABOUTDIALOG_H__

