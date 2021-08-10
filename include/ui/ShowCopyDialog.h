//
// Created by Administrator on 2021/7/21.
//

#ifndef GPGFRONTEND_ZH_CN_TS_SHOWCOPYDIALOG_H
#define GPGFRONTEND_ZH_CN_TS_SHOWCOPYDIALOG_H

#include "GpgFrontend.h"

class ShowCopyDialog : public QDialog {
Q_OBJECT
public:
    explicit ShowCopyDialog(const QString &text, QWidget *parent = nullptr);

private slots:

    void slotCopyText();

private:
    QTextEdit *textEdit;
    QPushButton *copyButton;
};


#endif //GPGFRONTEND_ZH_CN_TS_SHOWCOPYDIALOG_H
