//
// Created by eric on 2021/6/3.
//

#ifndef GPGFRONTEND_KEYSETEXPIREDATEDIALOG_H
#define GPGFRONTEND_KEYSETEXPIREDATEDIALOG_H

#include "GpgFrontend.h"
#include "gpg/GpgContext.h"
#include "gpg/GpgKey.h"
#include "gpg/GpgSubKey.h"

class KeySetExpireDateDialog : public QDialog {
Q_OBJECT
public:
    explicit KeySetExpireDateDialog(GpgME::GpgContext *ctx, const GpgKey &key, const GpgSubKey *subkey, QWidget *parent = nullptr);

private:
    GpgME::GpgContext *mCtx;
    const GpgKey &mKey;
    const GpgSubKey *mSubkey;

    QDateTimeEdit *dateTimeEdit{};
    QPushButton *confirmButton{};
    QCheckBox *nonExpiredCheck{};

private slots:
    void slotConfirm();
    void slotNonExpiredChecked(int state);
};


#endif //GPGFRONTEND_KEYSETEXPIREDATEDIALOG_H
