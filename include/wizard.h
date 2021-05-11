/*
 *      wizard.h
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


#ifndef WIZARD_H
#define WIZARD_H

#include <QWizard>
#include "keygendialog.h"
#include "keymgmt.h"
#include "gpgconstants.h"
#include "settingsdialog.h"

class QCheckBox;
class QLabel;
class QLineEdit;
class QRadioButton;

class Wizard : public QWizard
{
    Q_OBJECT
    Q_ENUMS(WizardPages)

public:
    enum WizardPages { Page_Intro, Page_Choose, Page_ImportFromGpg4usb, Page_ImportFromGnupg, Page_GenKey,
                Page_Conclusion };

    Wizard(GpgME::GpgContext *ctx, KeyMgmt *keyMgmt, QWidget *parent = 0);
    static bool importPubAndSecKeysFromDir(const QString dir, KeyMgmt *keyMgmt);

private:
    GpgME::GpgContext *mCtx;
    KeyMgmt *mKeyMgmt;

private slots:
    void slotWizardAccepted();

signals:
    void signalOpenHelp(QString page);
};

class IntroPage : public QWizardPage
{
    Q_OBJECT

public:
    IntroPage(QWidget *parent = 0);
    QHash<QString,QString> languages;
    int nextId() const;

private slots:
    void slotLangChange(QString lang);
};

class ChoosePage : public QWizardPage
{
    Q_OBJECT

public:
    ChoosePage(QWidget *parent = 0);

private slots:
    void slotJumpPage(const QString& page);

private:
    int nextId() const;
    int nextPage;
};

class ImportFromGpg4usbPage : public QWizardPage
{
    Q_OBJECT

public:
    ImportFromGpg4usbPage(GpgME::GpgContext *ctx, KeyMgmt *keyMgmt, QWidget *parent = 0);

private slots:
    /**
      * @details  Import keys from gnupg-homedir, private or/and public depend on the checked boxes
      */
    void slotImportFromOlderGpg4usb();
    bool slotImportConfFromGpg4usb(QString dir);

private:
    int nextId() const;

    KeyMgmt *mKeyMgmt;
    GpgME::GpgContext *mCtx;
    QCheckBox *gpg4usbKeyCheckBox;
    QCheckBox *gpg4usbConfigCheckBox;
};

class ImportFromGnupgPage : public QWizardPage
{
    Q_OBJECT

public:
    ImportFromGnupgPage(GpgME::GpgContext *ctx, KeyMgmt *keyMgmt, QWidget *parent = 0);

private slots:
    /**
      * @details  Import keys from gnupg-homedir, private or/and public depend on the checked boxes
      */
    void slotrImportKeysFromGnupg();

private:
    KeyMgmt *mKeyMgmt;
    int nextId() const;

    /**
      * @details  String containing the gnupg-homedir
      * @returns String containg the gnupg-homedir, but NULL, if the in windows registry entry
      * doesn't exist or in linux ~/.gnupg doesn't exist
      */
    QString getGnuPGHome();

    GpgME::GpgContext *mCtx;
    QPushButton *importFromGnupgButton;
};

class KeyGenPage : public QWizardPage
{
    Q_OBJECT

public:
    KeyGenPage(GpgME::GpgContext *ctx, QWidget *parent = 0);
    int nextId() const;

private slots:
    void slotGenerateKeyDialog();

private:
    GpgME::GpgContext *mCtx;
};

class ConclusionPage : public QWizardPage
{
    Q_OBJECT

public:
    ConclusionPage(QWidget *parent = 0);
    int nextId() const;

private:
    QCheckBox *dontShowWizardCheckBox;
    QCheckBox *openHelpCheckBox;
};

#endif
