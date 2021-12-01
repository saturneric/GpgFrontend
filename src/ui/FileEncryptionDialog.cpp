/**
 * This file is part of GpgFrontend.
 *
 * GpgFrontend is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Foobar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Foobar.  If not, see <https://www.gnu.org/licenses/>.
 *
 * The initial version of the source code is inherited from gpg4usb-team.
 * Their source code version also complies with GNU General Public License.
 *
 * The source code version of this software was modified and released
 * by Saturneric<eric@bktus.com> starting on May 12, 2021.
 *
 */

#include "ui/FileEncryptionDialog.h"

#include "gpg/function/BasicOperator.h"
#include "gpg/function/GpgKeyGetter.h"

namespace GpgFrontend::UI {
FileEncryptionDialog::FileEncryptionDialog(KeyIdArgsListPtr keyList,
                                           DialogAction action, QWidget* parent)
    : QDialog(parent), mAction(action) {
  if (mAction == Decrypt) {
    setWindowTitle(_("Decrypt File"));
  } else if (mAction == Encrypt) {
    setWindowTitle(_("Encrypt File"));
  } else if (mAction == Sign) {
    setWindowTitle(_("Sign File"));
  } else if (mAction == Verify) {
    setWindowTitle(_("Verify File"));
  }

  setModal(true);

  auto* buttonBox =
      new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  connect(buttonBox, SIGNAL(accepted()), this, SLOT(slotExecuteAction()));
  connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

  auto* groupBox1 = new QGroupBox(_("Input Parameters"));

  /* Setup input & Outputfileselection*/
  inputFileEdit = new QLineEdit();
  auto* fb1 = new QPushButton("Select");
  connect(fb1, SIGNAL(clicked()), this, SLOT(slotSelectInputFile()));
  auto* fl1 = new QLabel(_("Target File"));
  fl1->setBuddy(inputFileEdit);

  outputFileEdit = new QLineEdit();
  auto* fb2 = new QPushButton("Select");
  connect(fb2, SIGNAL(clicked()), this, SLOT(slotSelectOutputFile()));
  auto* fl2 = new QLabel(_("Output File"));
  fl2->setBuddy(outputFileEdit);

  auto* gLayout = new QGridLayout();
  gLayout->addWidget(fl1, 0, 0);
  gLayout->addWidget(inputFileEdit, 0, 1);
  gLayout->addWidget(fb1, 0, 2);
  signFileEdit = new QLineEdit();
  // verify does not need outfile, but signature file
  if (mAction != Verify) {
    gLayout->addWidget(fl2, 1, 0);
    gLayout->addWidget(outputFileEdit, 1, 1);
    gLayout->addWidget(fb2, 1, 2);
  } else {
    auto* sfb1 = new QPushButton("Select");
    connect(sfb1, SIGNAL(clicked()), this, SLOT(slotSelectSignFile()));
    auto* sfl1 = new QLabel(_("Signature File(.sig) Path"));
    sfl1->setBuddy(signFileEdit);

    gLayout->addWidget(sfl1, 1, 0);
    gLayout->addWidget(signFileEdit, 1, 1);
    gLayout->addWidget(sfb1, 1, 2);
  }
  groupBox1->setLayout(gLayout);

  /*Setup KeyList*/
  mKeyList = new KeyList(
      KeyListRow::ONLY_SECRET_KEY,
      KeyListColumn::NAME | KeyListColumn::EmailAddress | KeyListColumn::Usage);
  if (mAction == Verify)
    mKeyList->setFilter([](const GpgKey& key) -> bool {
      if (key.disabled() || key.expired() || key.revoked())
        return false;
      else
        return true;
    });

  if (mAction == Encrypt)
    mKeyList->setFilter([](const GpgKey& key) -> bool {
      if (!key.CanEncrActual())
        return false;
      else
        return true;
    });

  if (mAction == Sign)
    mKeyList->setFilter([](const GpgKey& key) -> bool {
      if (!key.CanSignActual())
        return false;
      else
        return true;
    });

  if (mAction == Decrypt) mKeyList->setDisabled(true);

  mKeyList->slotRefresh();
  mKeyList->setChecked(keyList);

  statusLabel = new QLabel();
  statusLabel->setStyleSheet("QLabel {color: red;}");

  auto* vbox2 = new QVBoxLayout();
  vbox2->addWidget(groupBox1);
  vbox2->addWidget(mKeyList);
  vbox2->addWidget(statusLabel);
  vbox2->addWidget(buttonBox);
  vbox2->addStretch(0);
  setLayout(vbox2);

  this->setMinimumWidth(480);
  this->show();
}

void FileEncryptionDialog::slotSelectInputFile() {
  QString path;
  if (inputFileEdit->text().size() > 0) {
    path = QFileInfo(inputFileEdit->text()).absolutePath();
  }

  //    QString infileName = QFileDialog::getOpenFileName(this, _("Open File"),
  //    path, _("Files") + _("All Files (*)"));
  QString infileName = QFileDialog::getOpenFileName(this, _("Open File"), path);
  inputFileEdit->setText(infileName);

  // try to find a matching output-filename, if not yet done
  if (!infileName.isEmpty() && outputFileEdit->text().size() == 0 &&
      signFileEdit->text().size() == 0) {
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

void FileEncryptionDialog::slotSelectOutputFile() {
  QString path;
  if (outputFileEdit->text().size() > 0) {
    path = QFileInfo(outputFileEdit->text()).absolutePath();
  }

  QString outfileName =
      QFileDialog::getSaveFileName(this, _("Save File"), path, nullptr, nullptr,
                                   QFileDialog::DontConfirmOverwrite);
  outputFileEdit->setText(outfileName);
}

void FileEncryptionDialog::slotSelectSignFile() {
  QString path;
  if (signFileEdit->text().size() > 0) {
    path = QFileInfo(signFileEdit->text()).absolutePath();
  }

  QString signfileName =
      QFileDialog::getSaveFileName(this, _("Open File"), path, nullptr, nullptr,
                                   QFileDialog::DontConfirmOverwrite);
  signFileEdit->setText(signfileName);

  if (inputFileEdit->text().size() == 0 &&
      signfileName.endsWith(".sig", Qt::CaseInsensitive)) {
    QString sfn = signfileName;
    sfn.chop(4);
    inputFileEdit->setText(sfn);
  }
}

void FileEncryptionDialog::slotExecuteAction() {
  QFile infile;
  infile.setFileName(inputFileEdit->text());
  if (!infile.open(QIODevice::ReadOnly)) {
    statusLabel->setText(_("Couldn't open file"));
    inputFileEdit->setStyleSheet("QLineEdit { background: yellow }");
    return;
  }
  auto in_data = read_all_data_in_file(inputFileEdit->text().toStdString());
  auto out_data = std::make_unique<ByteArray>();

  auto key_ids = mKeyList->getChecked();
  auto keys = std::vector<GpgKey>();
  for (const auto& key_id : *key_ids)
    keys.push_back(GpgKeyGetter::GetInstance().GetKey(key_id));

  if (mAction == Encrypt) {
    qDebug() << "Action Encrypt";
    GpgEncrResult result = nullptr;
    gpgme_error_t err = BasicOperator::GetInstance().Encrypt(
        std::move(keys), in_data, out_data, result);
    if (gpgme_err_code(err) != GPG_ERR_NO_ERROR) {
      qDebug() << "Error" << gpgme_strerror(err);

      QMessageBox::warning(this, _("Error"),
                           _("Error Occurred During Encryption"));
      return;
    }
  }

  else if (mAction == Decrypt) {
    qDebug() << "Action Decrypt";
    GpgDecrResult result = nullptr;
    gpgme_error_t err =
        BasicOperator::GetInstance().Decrypt(in_data, out_data, result);
    if (gpgme_err_code(err) != GPG_ERR_NO_ERROR) {
      qDebug() << "Error" << gpgme_strerror(err);
      QMessageBox::warning(this, _("Error"),
                           _("Error Occurred During Decryption"));
      return;
    }
  }

  else if (mAction == Sign) {
    qDebug() << "Action Sign";
    GpgSignResult result = nullptr;
    gpgme_error_t err = BasicOperator::GetInstance().Sign(
        std::move(keys), in_data, out_data, GPGME_SIG_MODE_DETACH, result);
    if (gpgme_err_code(err) != GPG_ERR_NO_ERROR) {
      qDebug() << "Error" << gpgme_strerror(err);
      QMessageBox::warning(this, _("Error"),
                           _("Error Occurred During Signature"));
      return;
    }
  }

  if (mAction == Verify) {
    auto sign_data = std::make_unique<ByteArray>(
        read_all_data_in_file(signFileEdit->text().toStdString()));
    GpgVerifyResult result = nullptr;
    auto error =
        BasicOperator::GetInstance().Verify(in_data, sign_data, result);
    new VerifyDetailsDialog(this, mKeyList, error, std::move(result));
    return;
  }

  write_buffer_to_file(outputFileEdit->text().toStdString(), *out_data);

  accept();
}

void FileEncryptionDialog::slotShowKeyList() { mKeyList->show(); }

void FileEncryptionDialog::slotHideKeyList() { mKeyList->hide(); }

}  // namespace GpgFrontend::UI
