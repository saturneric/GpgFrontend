/*
 *      fileencryptiondialog.cpp
 *
 *      Copyright 2008 gpg4usb-team <gpg4usb@cpunk.de>
 *
 *      This file is part of gpg4usb.
 *
 *      Gpg4usb is free software: you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation, either version 3 of the License, or
 *      (at your option) any later version.
 *
 *      Gpg4usb is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with gpg4usb.  If not, see <http://www.gnu.org/licenses/>
 */

#include "fileencryptiondialog.h"

FileEncryptionDialog::FileEncryptionDialog(GpgME::GpgContext *ctx, QStringList keyList, DialogAction action, QWidget *parent)
    : QDialog(parent)

{
    mAction = action;
    mCtx = ctx;
    if (mAction == Decrypt) {
        setWindowTitle(tr("Decrypt File"));
        resize(500, 200);
    } else if (mAction == Encrypt) {
        setWindowTitle(tr("Encrypt File"));
        resize(500, 400);
    } else if (mAction == Sign) {
        setWindowTitle(tr("Sign File"));
        resize(500, 400);
    } else if (mAction == Verify) {
        setWindowTitle(tr("Verify File"));
        resize(500, 200);
    }

    setModal(true);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(slotExecuteAction()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    QGroupBox *groupBox1 = new QGroupBox(tr("File"));

    /* Setup input & Outputfileselection*/
    inputFileEdit = new QLineEdit();
    QPushButton *fb1 = new QPushButton("...");
    connect(fb1, SIGNAL(clicked()), this, SLOT(slotSelectInputFile()));
    QLabel *fl1 = new QLabel(tr("Input"));
    fl1->setBuddy(inputFileEdit);

    outputFileEdit = new QLineEdit();
    QPushButton *fb2 = new QPushButton("...");
    connect(fb2, SIGNAL(clicked()), this, SLOT(slotSelectOutputFile()));
    QLabel *fl2 = new QLabel(tr("Output"));
    fl2->setBuddy(outputFileEdit);

    QGridLayout *gLayout = new QGridLayout();
    gLayout->addWidget(fl1, 0, 0);
    gLayout->addWidget(inputFileEdit, 0, 1);
    gLayout->addWidget(fb1, 0, 2);
    signFileEdit = new QLineEdit();
    // verify does not need outfile, but signature file
    if(mAction != Verify) {
        gLayout->addWidget(fl2, 1, 0);
        gLayout->addWidget(outputFileEdit, 1, 1);
        gLayout->addWidget(fb2, 1, 2);
    } else {
        QPushButton *sfb1 = new QPushButton("...");
        connect(sfb1, SIGNAL(clicked()), this, SLOT(slotSelectSignFile()));
        QLabel *sfl1 = new QLabel(tr("Signature"));
        sfl1->setBuddy(signFileEdit);

        gLayout->addWidget(sfl1, 1, 0);
        gLayout->addWidget(signFileEdit, 1, 1);
        gLayout->addWidget(sfb1, 1, 2);
    }
    groupBox1->setLayout(gLayout);

    /*Setup KeyList*/
    mKeyList = new KeyList(mCtx);
    mKeyList->hide();
    mKeyList->setColumnWidth(2, 150);
    mKeyList->setColumnWidth(3, 150);
    mKeyList->setChecked(&keyList);

    statusLabel = new QLabel();
    statusLabel->setStyleSheet("QLabel {color: red;}");

    QVBoxLayout *vbox2 = new QVBoxLayout();
    vbox2->addWidget(groupBox1);
    vbox2->addWidget(mKeyList);
    vbox2->addWidget(statusLabel);
    vbox2->addWidget(buttonBox);
    vbox2->addStretch(0);
    setLayout(vbox2);

    if(action == Encrypt || action == Sign) {
        slotShowKeyList();
    }

    exec();
}

void FileEncryptionDialog::slotSelectInputFile()
{
    QString path = "";
    if (inputFileEdit->text().size() > 0) {
        path = QFileInfo(inputFileEdit->text()).absolutePath();
    }

//    QString infileName = QFileDialog::getOpenFileName(this, tr("Open File"), path, tr("Files") + tr("All Files (*)"));
    QString infileName = QFileDialog::getOpenFileName(this, tr("Open File"), path);
    inputFileEdit->setText(infileName);

    // try to find a matching output-filename, if not yet done
    if (infileName > 0
            && outputFileEdit->text().size() == 0
            && signFileEdit->text().size() == 0) {
        if (mAction == Encrypt) {
            outputFileEdit->setText(infileName + ".asc");
        } else if (mAction == Sign) {
            outputFileEdit->setText(infileName + ".sig");
        } else if (mAction == Verify) {
            signFileEdit->setText(infileName + ".sig");
        } else {
            if (infileName.endsWith(".asc", Qt::CaseInsensitive)) {
                QString ofn = infileName;
                ofn.chop(4);
                outputFileEdit->setText(ofn);
            } else {
                outputFileEdit->setText(infileName + ".out");
            }
        }
    }
}

void FileEncryptionDialog::slotSelectOutputFile()
{
    QString path = "";
    if (outputFileEdit->text().size() > 0) {
        path = QFileInfo(outputFileEdit->text()).absolutePath();
    }

    QString outfileName = QFileDialog::getSaveFileName(this, tr("Save File"),path, NULL ,NULL ,QFileDialog::DontConfirmOverwrite);
    outputFileEdit->setText(outfileName);

}

void FileEncryptionDialog::slotSelectSignFile()
{
    QString path = "";
    if (signFileEdit->text().size() > 0) {
        path = QFileInfo(signFileEdit->text()).absolutePath();
    }

    QString signfileName = QFileDialog::getSaveFileName(this, tr("Open File"),path, NULL ,NULL ,QFileDialog::DontConfirmOverwrite);
    signFileEdit->setText(signfileName);

    if (inputFileEdit->text().size() == 0 && signfileName.endsWith(".sig", Qt::CaseInsensitive)) {
        QString sfn = signfileName;
        sfn.chop(4);
        inputFileEdit->setText(sfn);
    }

}

void FileEncryptionDialog::slotExecuteAction()
{

    QFile infile;
    infile.setFileName(inputFileEdit->text());
    if (!infile.open(QIODevice::ReadOnly)) {
        statusLabel->setText( tr("Couldn't open file"));
        inputFileEdit->setStyleSheet("QLineEdit { background: yellow }");
        return;
    }

    QByteArray inBuffer = infile.readAll();
    QByteArray *outBuffer = new QByteArray();
    infile.close();
    if ( mAction == Encrypt ) {
        if (! mCtx->encrypt(mKeyList->getChecked(), inBuffer, outBuffer)) return;
    }

    if ( mAction == Decrypt )  {
        if (! mCtx->decrypt(inBuffer, outBuffer)) return;
    }

    if( mAction == Sign ) {
        if(! mCtx->sign(mKeyList->getChecked(), inBuffer, outBuffer, true)) return;
    }

    if( mAction == Verify ) {
        QFile signfile;
        signfile.setFileName(signFileEdit->text());
        if (!signfile.open(QIODevice::ReadOnly)) {
            statusLabel->setText( tr("Couldn't open file"));
            signFileEdit->setStyleSheet("QLineEdit { background: yellow }");
            return;
        }
        QByteArray signBuffer = signfile.readAll();
        new VerifyDetailsDialog(this, mCtx, mKeyList, &inBuffer, &signBuffer);
        return;
    }

    QFile outfile(outputFileEdit->text());
    if (outfile.exists()){
        QMessageBox::StandardButton ret;
        ret = QMessageBox::warning(this, tr("File"),
                                           tr("File exists! Do you want to overwrite it?"),
                                           QMessageBox::Ok|QMessageBox::Cancel);
        if (ret == QMessageBox::Cancel){
            return;
        }
    }

    if (!outfile.open(QFile::WriteOnly)) {
        QMessageBox::warning(this, tr("File"),
                             tr("Cannot write file %1:\n%2.")
                             .arg(outputFileEdit->text())
                             .arg(outfile.errorString()));
        return;
    }

    QDataStream out(&outfile);
    out.writeRawData(outBuffer->data(), outBuffer->length());
    outfile.close();
    QMessageBox::information(0, "Done", "Output saved to " + outputFileEdit->text());

    accept();
}

void FileEncryptionDialog::slotShowKeyList()
{
    mKeyList->show();
}

void FileEncryptionDialog::slotHideKeyList()
{
    mKeyList->hide();
}
