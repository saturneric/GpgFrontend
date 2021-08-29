/**
 * This file is part of GPGFrontend.
 *
 * GPGFrontend is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Foobar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Foobar.  If not, see <https://www.gnu.org/licenses/>.
 *
 * The initial version of the source code is inherited from gpg4usb-team.
 * Their source code version also complies with GNU General Public License.
 *
 * The source code version of this software was modified and released
 * by Saturneric<eric@bktus.com><eric@bktus.com> starting on May 12, 2021.
 *
 */

#ifndef __KEYGENDIALOG_H__
#define __KEYGENDIALOG_H__

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
    explicit KeyGenDialog(GpgFrontend::GpgContext *ctx, QWidget *parent = nullptr);

private:

    QGroupBox *create_key_usage_group_box();

    QGroupBox *create_basic_info_group_box();

    QRegularExpression re_email{
            R"((?:[a-z0-9!#$%&'*+/=?^_`{|}~-]+(?:\.[a-z0-9!#$%&'*+/=?^_`{|}~-]+)*|"(?:[\x01-\x08\x0b\x0c\x0e-\x1f\x21\x23-\x5b\x5d-\x7f]|\\[\x01-\x09\x0b\x0c\x0e-\x7f])*")@(?:(?:[a-z0-9](?:[a-z0-9-]*[a-z0-9])?\.)+[a-z0-9](?:[a-z0-9-]*[a-z0-9])?|\[(?:(?:(2(5[0-5]|[0-4][0-9])|1[0-9][0-9]|[1-9]?[0-9]))\.){3}(?:(2(5[0-5]|[0-4][0-9])|1[0-9][0-9]|[1-9]?[0-9])|[a-z0-9-]*[a-z0-9]:(?:[\x01-\x08\x0b\x0c\x0e-\x1f\x21-\x5a\x53-\x7f]|\\[\x01-\x09\x0b\x0c\x0e-\x7f])+)\]))"};

    GpgFrontend::GpgContext *mCtx; /** The current gpg context */
    QStringList errorMessages; /** List of errors occuring when checking entries of lineedits */
    GenKeyInfo genKeyInfo{};

    QDialogButtonBox *buttonBox; /** Box for standardbuttons */
    QLabel *errorLabel{}; /** Label containing error message */
    QLineEdit *nameEdit{}; /** Lineedit for the keys name */
    QLineEdit *emailEdit{}; /** Lineedit for the keys email */
    QLineEdit *commentEdit{}; /** Lineedit for the keys comment */
    QSpinBox *keySizeSpinBox{}; /** Spinbox for the keys size (in bit) */
    QComboBox *keyTypeComboBox{}; /** Combobox for Keytpe */
    QDateTimeEdit *dateEdit{}; /** Dateedit for expiration date */
    QCheckBox *expireCheckBox{}; /** Checkbox, if key should expire */
    QCheckBox *noPassPhraseCheckBox{};

    QGroupBox *keyUsageGroupBox{}; /** Group of Widgets detecting the usage of the Key **/

//    ENCR, SIGN, CERT, AUTH
    std::vector<QCheckBox *> keyUsageCheckBoxes;

    void generateKeyDialog();

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
