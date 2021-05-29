//
// Created by eric on 2021/5/30.
//

#include "ui/keygen/SubkeyGenerateDialog.h"

SubkeyGenerateDialog::SubkeyGenerateDialog(GpgME::GpgContext *ctx, const GpgKey &key, QWidget *parent)
        : genKeyInfo(true), mCtx(ctx), mKey(key), QDialog(parent) {

    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    keyUsageGroupBox = create_key_usage_group_box();

    auto *groupGrid = new QGridLayout(this);
    groupGrid->addWidget(create_basic_info_group_box(), 0, 0);
    groupGrid->addWidget(keyUsageGroupBox, 1, 0);

    auto *nameList = new QWidget(this);
    nameList->setLayout(groupGrid);

    auto *vbox2 = new QVBoxLayout();
    vbox2->addWidget(nameList);
    vbox2->addWidget(errorLabel);
    vbox2->addWidget(buttonBox);

    this->setWindowTitle(tr("Generate Subkey"));

    this->setLayout(vbox2);
    this->setModal(true);

    set_signal_slot();
    refresh_widgets_state();
}

QGroupBox *SubkeyGenerateDialog::create_key_usage_group_box() {
    auto *groupBox = new QGroupBox(this);
    auto *grid = new QGridLayout(this);

    groupBox->setTitle("Key Usage");

    auto* encrypt = new QCheckBox(tr("Encryption"), groupBox);
    encrypt->setTristate(false);

    auto* sign = new QCheckBox(tr("Signing"),groupBox);
    sign->setTristate(false);

    auto* cert = new QCheckBox(tr("Certification"),groupBox);
    cert->setTristate(false);

    auto* auth = new QCheckBox(tr("Authentication"), groupBox);
    auth->setTristate(false);

    keyUsageCheckBoxes.push_back(encrypt);
    keyUsageCheckBoxes.push_back(sign);
    keyUsageCheckBoxes.push_back(cert);
    keyUsageCheckBoxes.push_back(auth);

    grid->addWidget(encrypt, 0, 0);
    grid->addWidget(sign, 0, 1);
    grid->addWidget(cert, 1, 0);
    grid->addWidget(auth, 1, 1);

    groupBox->setLayout(grid);

    return groupBox;
}

QGroupBox *SubkeyGenerateDialog::create_basic_info_group_box() {
    errorLabel = new QLabel(tr(""));
    keySizeSpinBox = new QSpinBox(this);
    keyTypeComboBox = new QComboBox(this);

    for(auto &algo : GenKeyInfo::SupportedSubkeyAlgo) {
        keyTypeComboBox->addItem(algo);
    }
    if(!GenKeyInfo::SupportedKeyAlgo.isEmpty()) {
        keyTypeComboBox->setCurrentIndex(0);
    }

    QDateTime maxDateTime = QDateTime::currentDateTime().addYears(2);

    dateEdit = new QDateTimeEdit(maxDateTime, this);
    dateEdit->setMinimumDateTime(QDateTime::currentDateTime());
    dateEdit->setMaximumDateTime(maxDateTime);
    dateEdit->setDisplayFormat("dd/MM/yyyy hh:mm:ss");
    dateEdit->setCalendarPopup(true);
    dateEdit->setEnabled(true);

    expireCheckBox = new QCheckBox(this);
    expireCheckBox->setCheckState(Qt::Unchecked);

    auto *vbox1 = new QGridLayout;

    vbox1->addWidget(new QLabel(tr("Expiration Date:")), 2, 0);
    vbox1->addWidget(new QLabel(tr("Never Expire")), 2, 3);
    vbox1->addWidget(new QLabel(tr("KeySize (in Bit):")), 1, 0);
    vbox1->addWidget(new QLabel(tr("Key Type:")), 0, 0);

    vbox1->addWidget(dateEdit, 2, 1);
    vbox1->addWidget(expireCheckBox, 2, 2);
    vbox1->addWidget(keySizeSpinBox, 1, 1);
    vbox1->addWidget(keyTypeComboBox, 0, 1);

    auto basicInfoGroupBox = new QGroupBox();
    basicInfoGroupBox->setLayout(vbox1);
    basicInfoGroupBox->setTitle(tr("Basic Information"));

    return basicInfoGroupBox;
}

void SubkeyGenerateDialog::set_signal_slot() {
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(slotKeyGenAccept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    connect(expireCheckBox, SIGNAL(stateChanged(int)), this, SLOT(slotExpireBoxChanged()));

    connect(keyUsageCheckBoxes[0], SIGNAL(stateChanged(int)), this, SLOT(slotEncryptionBoxChanged(int)));
    connect(keyUsageCheckBoxes[1], SIGNAL(stateChanged(int)), this, SLOT(slotSigningBoxChanged(int)));
    connect(keyUsageCheckBoxes[2], SIGNAL(stateChanged(int)), this, SLOT(slotCertificationBoxChanged(int)));
    connect(keyUsageCheckBoxes[3], SIGNAL(stateChanged(int)), this, SLOT(slotAuthenticationBoxChanged(int)));

    connect(keyTypeComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(slotActivatedKeyType(int)));
}

void SubkeyGenerateDialog::slotExpireBoxChanged() {
    if (expireCheckBox->checkState()) {
        dateEdit->setEnabled(false);
    } else {
        dateEdit->setEnabled(true);
    }
}

void SubkeyGenerateDialog::refresh_widgets_state() {
    qDebug() << "refresh_widgets_state called";

    if(genKeyInfo.isAllowEncryption())
        keyUsageCheckBoxes[0]->setCheckState(Qt::CheckState::Checked);
    else
        keyUsageCheckBoxes[0]->setCheckState(Qt::CheckState::Unchecked);

    if(genKeyInfo.isAllowChangeEncryption())
        keyUsageCheckBoxes[0]->setDisabled(false);
    else
        keyUsageCheckBoxes[0]->setDisabled(true);


    if(genKeyInfo.isAllowSigning())
        keyUsageCheckBoxes[1]->setCheckState(Qt::CheckState::Checked);
    else
        keyUsageCheckBoxes[1]->setCheckState(Qt::CheckState::Unchecked);

    if(genKeyInfo.isAllowChangeSigning())
        keyUsageCheckBoxes[1]->setDisabled(false);
    else
        keyUsageCheckBoxes[1]->setDisabled(true);


    if(genKeyInfo.isAllowCertification())
        keyUsageCheckBoxes[2]->setCheckState(Qt::CheckState::Checked);
    else
        keyUsageCheckBoxes[2]->setCheckState(Qt::CheckState::Unchecked);

    if(genKeyInfo.isAllowChangeCertification())
        keyUsageCheckBoxes[2]->setDisabled(false);
    else
        keyUsageCheckBoxes[2]->setDisabled(true);


    if(genKeyInfo.isAllowAuthentication())
        keyUsageCheckBoxes[3]->setCheckState(Qt::CheckState::Checked);
    else
        keyUsageCheckBoxes[3]->setCheckState(Qt::CheckState::Unchecked);

    if(genKeyInfo.isAllowChangeAuthentication())
        keyUsageCheckBoxes[3]->setDisabled(false);
    else
        keyUsageCheckBoxes[3]->setDisabled(true);


    keySizeSpinBox->setRange(genKeyInfo.getSuggestMinKeySize(), genKeyInfo.getSuggestMaxKeySize());
    keySizeSpinBox->setValue(genKeyInfo.getKeySize());
    keySizeSpinBox->setSingleStep(genKeyInfo.getSizeChangeStep());

}

void SubkeyGenerateDialog::slotKeyGenAccept() {
    QString errorString = "";

    /**
     * primary keys should have a reasonable expiration date (no more than 2 years in the future)
     */
    if(dateEdit->dateTime() > QDateTime::currentDateTime().addYears(2)) {

        errorString.append(tr("  Expiration time no more than 2 years.  "));
    }

    if (errorString.isEmpty()) {

        genKeyInfo.setKeySize(keySizeSpinBox->value());

        if (expireCheckBox->checkState()) {
            genKeyInfo.setNonExpired(true);
        } else {
            genKeyInfo.setExpired(dateEdit->dateTime());
        }

        kg = new SubkeyGenerateThread(mKey ,&genKeyInfo, mCtx);
        connect(kg, SIGNAL(signalKeyGenerated(bool)), this, SLOT(slotKeyGenResult(bool)));
        kg->start();

        this->accept();

        auto *dialog = new QDialog(this, Qt::CustomizeWindowHint | Qt::WindowTitleHint);
        dialog->setModal(true);
        dialog->setWindowTitle(tr("Generating Subkey..."));

        auto *waitMessage = new QLabel(
                tr("Collecting random data for subkey generation.\n This may take a while.\n To speed up the process use your computer\n (e.g. browse the net, listen to music,...)"));
        auto *pb = new QProgressBar();
        pb->setRange(0, 0);

        auto *layout = new QVBoxLayout(dialog);
        layout->addWidget(waitMessage);
        layout->addWidget(pb);
        dialog->setLayout(layout);

        dialog->show();

        while (!kg->isFinished() && kg->isRunning()) {
            QCoreApplication::processEvents();
        }

        dialog->close();

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

void SubkeyGenerateDialog::slotEncryptionBoxChanged(int state) {
    if(state == 0) {
        genKeyInfo.setAllowEncryption(false);
    } else {
        genKeyInfo.setAllowEncryption(true);
    }
}

void SubkeyGenerateDialog::slotSigningBoxChanged(int state) {
    if(state == 0) {
        genKeyInfo.setAllowSigning(false);
    } else {
        genKeyInfo.setAllowSigning(true);
    }
}

void SubkeyGenerateDialog::slotCertificationBoxChanged(int state) {
    if(state == 0) {
        genKeyInfo.setAllowCertification(false);
    } else {
        genKeyInfo.setAllowCertification(true);
    }
}

void SubkeyGenerateDialog::slotAuthenticationBoxChanged(int state) {
    if(state == 0) {
        genKeyInfo.setAllowAuthentication(false);
    } else {
        genKeyInfo.setAllowAuthentication(true);
    }
}

void SubkeyGenerateDialog::slotActivatedKeyType(int index) {
    qDebug() << "key type index changed " << index;
    genKeyInfo.setAlgo(this->keyTypeComboBox->itemText(index));
    refresh_widgets_state();
}

void SubkeyGenerateDialog::slotKeyGenResult(bool success) {
    if(success)
        QMessageBox::information(nullptr, tr("Success"), tr("The new subkey has been generated."));
    else
        QMessageBox::critical(nullptr, tr("Failure"), tr("An error occurred during subkey generation."));
}
