//
// Created by eric on 2021/5/21.
//

#ifndef GPGFRONTEND_KEYPAIRDETAILTAB_H
#define GPGFRONTEND_KEYPAIRDETAILTAB_H

#include "GpgFrontend.h"
#include "gpg/GpgContext.h"

class KeyPairDetailTab : public QWidget {
    Q_OBJECT

    /**
 * @details Return QString with a space inserted at every fourth character
 *
 * @param fingerprint The fingerprint to be beautified
 */
    static QString beautifyFingerprint(QString fingerprint);

private slots:

    /**
     * @details Export the key to a file, which is choosen in a file dialog
     */
    void slotExportPrivateKey();

    /**
     * @details Copy the fingerprint to clipboard
     */
    void slotCopyFingerprint();

private:

    QString *keyid; /** The id of the key the details should be shown for */

    GpgME::GpgContext *mCtx; /** The current gpg-context */

    QGroupBox *ownerBox; /** Groupbox containing owner information */
    QGroupBox *keyBox; /** Groupbox containing key information */
    QGroupBox *fingerprintBox; /** Groupbox containing fingerprint information */
    QGroupBox *additionalUidBox; /** Groupbox containing information about additional uids */
    QDialogButtonBox *buttonBox; /** Box containing the close button */

    QLabel *nameVarLabel; /** Label containng the keys name */
    QLabel *emailVarLabel; /** Label containng the keys email */
    QLabel *commentVarLabel; /** Label containng the keys commment */
    QLabel *keySizeVarLabel; /** Label containng the keys keysize */
    QLabel *expireVarLabel; /** Label containng the keys expiration date */
    QLabel *createdVarLabel; /** Label containng the keys creation date */
    QLabel *algorithmVarLabel; /** Label containng the keys algorithm */
    QLabel *keyidVarLabel;  /** Label containng the keys keyid */
    QLabel *fingerPrintVarLabel; /** Label containng the keys fingerprint */
    QLabel *usageVarLabel;

public:
    explicit KeyPairDetailTab(GpgME::GpgContext *ctx, const GpgKey& key, QWidget *parent = nullptr);
};


#endif //GPGFRONTEND_KEYPAIRDETAILTAB_H
