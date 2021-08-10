//
// Created by Administrator on 2021/7/21.
//

#include "ui/ShowCopyDialog.h"

ShowCopyDialog::ShowCopyDialog(const QString &text, QWidget *parent) : QDialog(parent) {
    textEdit = new QTextEdit();
    textEdit->setReadOnly(true);
    textEdit->setLineWrapMode(QTextEdit::WidgetWidth);
    textEdit->setText(text);
    copyButton = new QPushButton("Copy");
    connect(copyButton, SIGNAL(clicked(bool)), this, SLOT(slotCopyText()));

    auto *layout = new QVBoxLayout();
    layout->addWidget(textEdit);
    layout->addWidget(copyButton);

    this->setModal(true);
    this->setLayout(layout);
}

void ShowCopyDialog::slotCopyText() {
    QClipboard *cb = QApplication::clipboard();
    cb->setText(textEdit->toPlainText());
}
