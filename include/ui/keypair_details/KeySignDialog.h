//
// Created by eric on 2021/5/24.
//

#ifndef GPGFRONTEND_KEYSIGNDIALOG_H
#define GPGFRONTEND_KEYSIGNDIALOG_H

#include "GpgFrontend.h"

#include "gpg/GpgContext.h"
#include "ui/widgets/KeyList.h"

class KeySignDialog : public QDialog {
    Q_OBJECT

public:

    explicit KeySignDialog(GpgME::GpgContext *ctx, const GpgKey &key, const QVector<UID> &uid, QWidget *parent = nullptr);

private:

    GpgME::GpgContext *mCtx;

    KeyList *mKeyList;

    QPushButton *signKeyButton;

    QDateTimeEdit *expiresEdit;

    QCheckBox *nonExpireCheck;

    const QVector<UID> mUids;

    const GpgKey &mKey;


private slots:

    void slotSignKey(bool clicked);

};


#endif //GPGFRONTEND_KEYSIGNDIALOG_H
