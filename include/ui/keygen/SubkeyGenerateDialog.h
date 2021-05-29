//
// Created by eric on 2021/5/30.
//

#ifndef GPGFRONTEND_SUBKEYGENERATEDIALOG_H
#define GPGFRONTEND_SUBKEYGENERATEDIALOG_H

#include "GpgFrontend.h"
#include "gpg/GpgKey.h"
#include "gpg/GpgSubKey.h"
#include "gpg/GpgGenKeyInfo.h"

#include "SubkeyGenerateThread.h"

class SubkeyGenerateDialog : public QDialog {
Q_OBJECT

public:

    explicit SubkeyGenerateDialog(GpgME::GpgContext *ctx, const GpgKey &key, QWidget *parent = nullptr);

private:

    GpgME::GpgContext *mCtx; /** The current gpg context */
    const GpgKey &mKey;

    GenKeyInfo genKeyInfo{};
    SubkeyGenerateThread *kg{}; /** Thread for key generation */

    QGroupBox *keyUsageGroupBox{};
    QDialogButtonBox *buttonBox; /** Box for standardbuttons */
    QLabel *errorLabel{}; /** Label containing error message */
    QSpinBox *keySizeSpinBox{}; /** Spinbox for the keys size (in bit) */
    QComboBox *keyTypeComboBox{}; /** Combobox for Keytpe */
    QDateTimeEdit *dateEdit{}; /** Dateedit for expiration date */
    QCheckBox *expireCheckBox{}; /** Checkbox, if key should expire */

    // ENCR, SIGN, CERT, AUTH
    std::vector<QCheckBox *> keyUsageCheckBoxes;

    QGroupBox *create_key_usage_group_box();

    QGroupBox *create_basic_info_group_box();

    void set_signal_slot();

    /**
     * @details Refresh widgets state by GenKeyInfo
     */
    void refresh_widgets_state();

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

    void slotKeyGenResult(bool success);

};


#endif //GPGFRONTEND_SUBKEYGENERATEDIALOG_H
