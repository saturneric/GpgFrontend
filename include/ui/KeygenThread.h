/*
 *      keygenthread.h
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

#ifndef __KEYGENTHREAD_H__
#define __KEYGENTHREAD_H__

#include "gpg/GpgContext.h"


QT_BEGIN_NAMESPACE
class QMessageBox;

QT_END_NAMESPACE

class KeyGenThread : public QThread {
Q_OBJECT

public:
    KeyGenThread(GenKeyInfo *keyGenParams, GpgME::GpgContext *ctx);

signals:

    void signalKeyGenerated(bool success);

private:
    GenKeyInfo *keyGenParams;
    GpgME::GpgContext *mCtx;
    [[maybe_unused]] bool abort;
    QMutex mutex;

protected:
    void run();

};

#endif // __KEYGENTHREAD_H__
