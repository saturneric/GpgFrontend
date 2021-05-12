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

#include "keygendialog.h"
#include "keymgmt.h"
#include "gpgconstants.h"
#include "settingsdialog.h"

class QCheckBox;

class QLabel;

class QLineEdit;

class QRadioButton;

class Wizard : public QWizard {
Q_OBJECT
    Q_ENUMS(WizardPages)

public:
    enum WizardPages {
        Page_Intro, Page_Choose, Page_ImportFromGpg4usb, Page_ImportFromGnupg, Page_GenKey,
        Page_Conclusion
    };

    Wizard(GpgME::GpgContext *ctx, KeyMgmt *keyMgmt, QWidget *parent = nullptr);

    static bool importPubAndSecKeysFromDir(const QString& dir, KeyMgmt *keyMgmt);

private:
    GpgME::GpgContext *mCtx;
    KeyMgmt *mKeyMgmt;

private slots:

    void slotWizardAccepted();

signals:

    void signalOpenHelp(QString page);
};

class IntroPage : public QWizardPage {
Q_OBJECT

public:
    explicit IntroPage(QWidget *parent = nullptr);

    QHash<QString, QString> languages;

    [[nodiscard]] int nextId() const override;

private slots:

    void slotLangChange(const QString& lang);
};

class ChoosePage : public QWizardPage {
Q_OBJECT

public:
    explicit ChoosePage(QWidget *parent = nullptr);

private slots:

    void slotJumpPage(const QString &page);

private:
    [[nodiscard]] int nextId() const override;

    int nextPage;
};

class ImportFromGpg4usbPage : public QWizardPage {
Q_OBJECT

public:
    ImportFromGpg4usbPage(GpgME::GpgContext *ctx, KeyMgmt *keyMgmt, QWidget *parent = nullptr);

private slots:

    /**
      * @details  Import keys from gnupg-homedir, private or/and public depend on the checked boxes
      */
    void slotImportFromOlderGpg4usb();

    bool slotImportConfFromGpg4usb(const QString& dir);

private:
    [[nodiscard]] int nextId() const override;

    KeyMgmt *mKeyMgmt;
    GpgME::GpgContext *mCtx;
    QCheckBox *gpg4usbKeyCheckBox;
    QCheckBox *gpg4usbConfigCheckBox;
};

class ImportFromGnupgPage : public QWizardPage {
Q_OBJECT

public:
    ImportFromGnupgPage(GpgME::GpgContext *ctx, KeyMgmt *keyMgmt, QWidget *parent = nullptr);

private slots:

    /**
      * @details  Import keys from gnupg-homedir, private or/and public depend on the checked boxes
      */
    void slotrImportKeysFromGnupg();

private:
    KeyMgmt *mKeyMgmt;

    [[nodiscard]] int nextId() const override;

    /**
      * @details  String containing the gnupg-homedir
      * @returns String containg the gnupg-homedir, but NULL, if the in windows registry entry
      * doesn't exist or in linux ~/.gnupg doesn't exist
      */
    static QString getGnuPGHome();

    GpgME::GpgContext *mCtx;
    QPushButton *importFromGnupgButton;
};

class KeyGenPage : public QWizardPage {
Q_OBJECT

public:
    explicit KeyGenPage(GpgME::GpgContext *ctx, QWidget *parent = nullptr);

    [[nodiscard]] int nextId() const override;

private slots:

    void slotGenerateKeyDialog();

private:
    GpgME::GpgContext *mCtx;
};

class ConclusionPage : public QWizardPage {
Q_OBJECT

public:
    explicit ConclusionPage(QWidget *parent = nullptr);

    [[nodiscard]] int nextId() const override;

private:
    QCheckBox *dontShowWizardCheckBox;
    QCheckBox *openHelpCheckBox;
};

#endif
