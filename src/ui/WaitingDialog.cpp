#include "ui/WaitingDialog.h"

WaitingDialog::WaitingDialog(const QString &title, QWidget *parent) : QDialog(parent) {
    auto *pb = new QProgressBar();
    pb->setRange(0, 0);
    pb->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    pb->setTextVisible(false);

    auto *layout = new QVBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(pb);
    this->setLayout(layout);

    this->setModal(true);
    this->setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::CustomizeWindowHint);
    this->setWindowTitle(title);
    this->setFixedSize(240, 42);
    this->show();
}
