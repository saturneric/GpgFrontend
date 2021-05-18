/*
 *      keygendialog.h
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

#ifndef __KEYGENDIALOG_H__
#define __KEYGENDIALOG_H__

#include "KeygenThread.h"
#include "gpg/GpgContext.h"


class KeyGenDialog : public QDialog {
Q_OBJECT


public:

    /**
     * @details Constructor of this class
     *
     * @param ctx The current GpgME context
     * @param key The key to show details of
     * @param parent The parent of this widget
     */
    explicit KeyGenDialog(GpgME::GpgContext *ctx, QWidget *parent = nullptr);

private:

    QGroupBox *create_key_usage_group_box();

    QRegularExpression re_email{
            R"((?:[a-z0-9!#$%&'*+/=?^_`{|}~-]+(?:\.[a-z0-9!#$%&'*+/=?^_`{|}~-]+)*|"(?:[\x01-\x08\x0b\x0c\x0e-\x1f\x21\x23-\x5b\x5d-\x7f]|\\[\x01-\x09\x0b\x0c\x0e-\x7f])*")@(?:(?:[a-z0-9](?:[a-z0-9-]*[a-z0-9])?\.)+[a-z0-9](?:[a-z0-9-]*[a-z0-9])?|\[(?:(?:(2(5[0-5]|[0-4][0-9])|1[0-9][0-9]|[1-9]?[0-9]))\.){3}(?:(2(5[0-5]|[0-4][0-9])|1[0-9][0-9]|[1-9]?[0-9])|[a-z0-9-]*[a-z0-9]:(?:[\x01-\x08\x0b\x0c\x0e-\x1f\x21-\x5a\x53-\x7f]|\\[\x01-\x09\x0b\x0c\x0e-\x7f])+)\]))"};

    GpgME::GpgContext *mCtx; /** The current gpg context */
    __attribute__((unused)) KeyGenThread *keyGenThread{}; /** Thread for key generation */
    __attribute__((unused)) QStringList errorMessages; /** List of errors occuring when checking entries of lineedits */
    GenKeyInfo genKeyInfo{};

    QDialogButtonBox *buttonBox; /** Box for standardbuttons */
    QLabel *errorLabel{}; /** Label containing error message */
    QLineEdit *nameEdit{}; /** Lineedit for the keys name */
    QLineEdit *emailEdit{}; /** Lineedit for the keys email */
    QLineEdit *commentEdit{}; /** Lineedit for the keys comment */
    QLineEdit *passwordEdit{}; /** Lineedit for the keys password */
    QLineEdit *repeatpwEdit{}; /** Lineedit for the repetition of the keys password */
    QSpinBox *keySizeSpinBox{}; /** Spinbox for the keys size (in bit) */
    QComboBox *keyTypeComboBox{}; /** Combobox for Keytpe */
    QDateTimeEdit *dateEdit{}; /** Dateedit for expiration date */
    QCheckBox *expireCheckBox{}; /** Checkbox, if key should expire */
    QCheckBox *noPassPhraseCheckBox{};
    QSlider *pwStrengthSlider{}; /** Slider showing the password strength */

    QGroupBox *keyUsageGroupBox{}; /** Group of Widgets detecting the usage of the Key **/

//    ENCR, SIGN, CERT, AUTH
    std::vector<QCheckBox *> keyUsageCheckBoxes;

    KeyGenThread *kg = nullptr;

    void generateKeyDialog();

    /**
     * @details Check the password strength of the text in the passwordEdit member
     *
     * @return digit between 0 and 6, the higher the more secure is the password
     */
    int checkPassWordStrength();


    /**
     * @details Refresh widgets state by GenKeyInfo
     */
    void refresh_widgets_state();

    void set_signal_slot();

    bool check_email_address(const QString &str);

private slots:

    /**
     * @details when expirebox was checked/unchecked, enable/disable the expiration date box
     */
    void slotExpireBoxChanged();

    /**
     * @details When passwordedit changed, set new value for password strength slider
     */
    void slotPasswordEditChanged();

    /**
     * @details check all lineedits for false entries. Show error, when there is one, otherwise generate the key
     */
    void slotKeyGenAccept();

    void slotEncryptionBoxChanged(int state);

    void slotSigningBoxChanged(int state);

    void slotCertificationBoxChanged(int state);

    void slotAuthenticationBoxChanged(int state);

    void slotActivatedKeyType(int index);

};

#endif // __KEYGENDIALOG_H__
