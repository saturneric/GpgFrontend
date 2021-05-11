/*
 *      settingsdialog.h
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

#ifndef __SETTINGSDIALOG_H__
#define __SETTINGSDIALOG_H__

#include "keylist.h"

class GeneralTab : public QWidget {
Q_OBJECT

public:
    explicit GeneralTab(GpgME::GpgContext *ctx, QWidget *parent = nullptr);

    void setSettings();

    void applySettings();

private:
    QCheckBox *rememberPasswordCheckBox;
    [[maybe_unused]] QCheckBox *importConfirmationcheckBox;
    QCheckBox *saveCheckedKeysCheckBox;
    QCheckBox *importConfirmationCheckBox;
    QComboBox *langSelectBox;
    QComboBox *ownKeySelectBox;
    QHash<QString, QString> lang;
    QHash<QString, QString> keyIds;
    QString ownKeyId;
    KeyList *mKeyList;
    GpgME::GpgContext *mCtx; /** The current gpg context */

private slots:

    void slotOwnKeyIdChanged();

    void slotLanguageChanged();

signals:

    void signalRestartNeeded(bool needed);

};

class MimeTab : public QWidget {
Q_OBJECT

public:
    explicit MimeTab(QWidget *parent = nullptr);

    void setSettings();

    void applySettings();

private:
    QCheckBox *mimeParseCheckBox;
    QCheckBox *mimeQPCheckBox;
    QCheckBox *mimeOpenAttachmentCheckBox;

signals:

    void signalRestartNeeded(bool needed);

};

class AppearanceTab : public QWidget {
Q_OBJECT

public:
    //void setSettings();
    explicit AppearanceTab(QWidget *parent = nullptr);

    void setSettings();

    void applySettings();

private:
    QButtonGroup *iconStyleGroup;
    QRadioButton *iconSizeSmall;
    QRadioButton *iconSizeMedium;
    QRadioButton *iconSizeLarge;
    QButtonGroup *iconSizeGroup;
    QRadioButton *iconTextButton;
    QRadioButton *iconIconsButton;
    QRadioButton *iconAllButton;
    QCheckBox *windowSizeCheckBox;

signals:

    void signalRestartNeeded(bool needed);

};

class KeyserverTab : public QWidget {
Q_OBJECT

public:
    explicit KeyserverTab(QWidget *parent = nullptr);

    void setSettings();

    void applySettings();

private:
    QComboBox *comboBox;
    QLineEdit *newKeyServerEdit;

private slots:

    void addKeyServer();

signals:

    void signalRestartNeeded(bool needed);

};

class AdvancedTab : public QWidget {
Q_OBJECT

public:
    explicit AdvancedTab(QWidget *parent = nullptr);

    void setSettings();

    void applySettings();

private:
    QCheckBox *steganoCheckBox;

signals:

    void signalRestartNeeded(bool needed);

};

class GpgPathsTab : public QWidget {
Q_OBJECT
public:
    explicit GpgPathsTab(QWidget *parent = nullptr);

    void applySettings();

private:
    QString getRelativePath(QString dir1, QString dir2);

    QString defKeydbPath; /** The default keydb path used by gpg4usb */
    QString accKeydbPath; /** The currently used keydb path */
    QLabel *keydbLabel;

    void setSettings();

private slots:

    QString chooseKeydbDir();

    void setKeydbPathToDefault();

};

class SettingsDialog : public QDialog {
Q_OBJECT

public:
    explicit SettingsDialog(GpgME::GpgContext *ctx, QWidget *parent = nullptr);

    GeneralTab *generalTab;
    MimeTab *mimeTab;
    AppearanceTab *appearanceTab;
    KeyserverTab *keyserverTab;
    AdvancedTab *advancedTab;
    GpgPathsTab *gpgPathsTab;

    static QHash<QString, QString> listLanguages();

public slots:

    void slotAccept();

signals:

    void signalRestartNeeded(bool needed);

private:
    QTabWidget *tabWidget;
    QDialogButtonBox *buttonBox;
    GpgME::GpgContext *mCtx; /** The current gpg context */
    bool restartNeeded;

    bool getRestartNeeded();

private slots:

    void slotSetRestartNeeded(bool needed);

};

#endif  // __SETTINGSDIALOG_H__
