#include "ui/WaitingDialog.h"

WaitingDialog::WaitingDialog(QWidget *parent) : QDialog(parent) {
    auto *pb = new QProgressBar();
    pb->setRange(0, 0);

    auto *layout = new QVBoxLayout();
    layout->addWidget(pb);
    this->setLayout(layout);

    this->setModal(true);
    this->setWindowTitle(tr("Processing"));
    this->setFixedSize(240, 42);
    this->show();
}
