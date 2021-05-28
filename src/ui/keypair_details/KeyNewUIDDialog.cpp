//
// Created by eric on 2021/5/28.
//

#include "ui/keypair_details/KeyNewUIDDialog.h"

KeyNewUIDDialog::KeyNewUIDDialog(GpgME::GpgContext *ctx, const GpgKey &key, QWidget *parent) :
    mCtx(ctx), mKey(key), QDialog(parent) {

    name = new QLineEdit();
    name->setMinimumWidth(240);
    email = new QLineEdit();
    email->setMinimumWidth(240);
    comment = new QLineEdit();
    comment->setMinimumWidth(240);
    createButton = new QPushButton("Create");
    errorLabel = new QLabel();

    auto gridLayout = new QGridLayout();
    gridLayout->addWidget(new QLabel(tr("Name")), 0, 0);
    gridLayout->addWidget(new QLabel(tr("Email")), 1, 0);
    gridLayout->addWidget(new QLabel(tr("Comment")), 2, 0);


    gridLayout->addWidget(name, 0 ,1);
    gridLayout->addWidget(email, 1 ,1);
    gridLayout->addWidget(comment, 2 ,1);

    gridLayout->addWidget(createButton, 3, 0, 1, 2);
    gridLayout->addWidget(errorLabel, 4, 0, 1, 2);

    connect(createButton, SIGNAL(clicked(bool)), this, SLOT(slotCreateNewUID()));

    this->setLayout(gridLayout);
    this->setWindowTitle(tr("Create New UID"));
    this->setAttribute(Qt::WA_DeleteOnClose, true);
    this->setModal(true);
}

void KeyNewUIDDialog::slotCreateNewUID() {

    QString errorString = "";

    /**
     * check for errors in keygen dialog input
     */
    if ((name->text()).size() < 5) {
        errorString.append(tr("  Name must contain at least five characters.  \n"));
    } if(email->text().isEmpty() || !check_email_address(email->text())) {
        errorString.append(tr("  Please give a email address.   \n"));
    }

    if (errorString.isEmpty()) {
        UID uid;
        uid.name = name->text();
        uid.email = email->text();
        uid.comment = comment->text();

        if(mCtx->addUID(mKey, uid)) {
            emit finished(1);

        } else {
            emit finished(-1);
        }

    } else {
        /**
         * create error message
         */
        errorLabel->setAutoFillBackground(true);
        QPalette error = errorLabel->palette();
        error.setColor(QPalette::Background, "#ff8080");
        errorLabel->setPalette(error);
        errorLabel->setText(errorString);

        this->show();
    }
}

bool KeyNewUIDDialog::check_email_address(const QString &str) {
    return re_email.match(str).hasMatch();
}
