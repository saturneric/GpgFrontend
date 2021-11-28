/**
 * This file is part of GPGFrontend.
 *
 * GPGFrontend is free software: you can redistribute it and/or modify
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

#include "ui/widgets/VerifyKeyDetailBox.h"

#include "gpg/function/GpgKeyGetter.h"

namespace GpgFrontend::UI {

VerifyKeyDetailBox::VerifyKeyDetailBox(QWidget* parent, KeyList* keyList,
                                       gpgme_signature_t signature)
    : QGroupBox(parent), mKeyList(keyList), fpr(signature->fpr) {
  auto* vbox = new QVBoxLayout();

  switch (gpg_err_code(signature->status)) {
    case GPG_ERR_NO_PUBKEY: {
      this->setTitle("A Error Signature");
      auto* importButton = new QPushButton(tr("Import from keyserver"));
      connect(importButton, SIGNAL(clicked()), this,
              SLOT(slotImportFormKeyserver()));

      this->setTitle(tr("Key not present with id 0x") + signature->fpr);

      auto grid = new QGridLayout();

      grid->addWidget(new QLabel(tr("Status:")), 0, 0);
      // grid->addWidget(new QLabel(tr("Fingerprint:")), 1, 0);
      grid->addWidget(new QLabel(tr("Key not present in keylist")), 0, 1);
      // grid->addWidget(new QLabel(signature->fpr), 1, 1);
      grid->addWidget(importButton, 2, 0, 2, 1);

      vbox->addLayout(grid);
      break;
    }
    case GPG_ERR_NO_ERROR: {
      this->setTitle("A Signature:");
      auto gird = createKeyInfoGrid(signature);
      if (gird != nullptr) {
        vbox->addLayout(gird);
      } else {
        vbox->addWidget(new QLabel(tr("Key Information is NOT Available")));
        if (signature->fpr != nullptr) {
          vbox->addWidget(
              new QLabel(tr("Fingerprint: ") + QString(signature->fpr)));
        }
      }
      break;
    }
    case GPG_ERR_CERT_REVOKED: {
      this->setTitle("An Error Signature");
      vbox->addWidget(new QLabel(tr("Status: Cert Revoked")));
      auto gird = createKeyInfoGrid(signature);
      if (gird != nullptr) {
        vbox->addLayout(gird);
      } else {
        vbox->addWidget(new QLabel(tr("Key Information is NOT Available")));
        if (signature->fpr != nullptr) {
          vbox->addWidget(
              new QLabel(tr("Fingerprint: ") + QString(signature->fpr)));
        }
      }
      break;
    }
    case GPG_ERR_SIG_EXPIRED: {
      this->setTitle("An Error Signature");
      vbox->addWidget(new QLabel(tr("Status: Signature Expired")));
      auto gird = createKeyInfoGrid(signature);
      if (gird != nullptr) {
        vbox->addLayout(gird);
      } else {
        vbox->addWidget(new QLabel(tr("Key Information is NOT Available")));
        if (signature->fpr != nullptr) {
          vbox->addWidget(
              new QLabel(tr("Fingerprint: ") + QString(signature->fpr)));
        }
      }
      break;
    }
    case GPG_ERR_KEY_EXPIRED: {
      this->setTitle("An Error Signature");
      vbox->addWidget(new QLabel(tr("Status: Signature Expired")));
      vbox->addWidget(new QLabel(tr("Status: Key Expired")));
      auto gird = createKeyInfoGrid(signature);
      if (gird != nullptr) {
        vbox->addLayout(gird);
      } else {
        vbox->addWidget(new QLabel(tr("Key Information is NOT Available")));
        if (signature->fpr != nullptr) {
          vbox->addWidget(
              new QLabel(tr("Fingerprint: ") + QString(signature->fpr)));
        }
      }
      break;
    }
    case GPG_ERR_GENERAL: {
      this->setTitle("An Error Signature");
      vbox->addWidget(new QLabel(tr("Status: General Error")));
      auto gird = createKeyInfoGrid(signature);
      if (gird != nullptr) {
        vbox->addLayout(gird);
      } else {
        vbox->addWidget(new QLabel(tr("Key Information is NOT Available")));
        if (signature->fpr != nullptr) {
          vbox->addWidget(
              new QLabel(tr("Fingerprint: ") + QString(signature->fpr)));
        }
      }
      break;
    }
    default: {
      this->setTitle("An Error Signature");
      this->setTitle(tr("Status: Unknown Error"));
      auto gird = createKeyInfoGrid(signature);
      if (gird != nullptr) {
        vbox->addLayout(gird);
      } else {
        vbox->addWidget(new QLabel(tr("Key Information is NOT Available")));
        if (signature->fpr != nullptr) {
          vbox->addWidget(
              new QLabel(tr("Fingerprint: ") + QString(signature->fpr)));
        }
      }
      break;
    }
  }
  this->setLayout(vbox);
}

void VerifyKeyDetailBox::slotImportFormKeyserver() {
  auto* importDialog = new KeyServerImportDialog(mKeyList, false, this);
  auto key_ids = std::make_unique<KeyIdArgsList>();
  key_ids->push_back(fpr.toStdString());
  importDialog->slotImport(key_ids);
}

QString VerifyKeyDetailBox::beautifyFingerprint(QString fingerprint) {
  uint len = fingerprint.length();
  if ((len > 0) && (len % 4 == 0))
    for (uint n = 0; 4 * (n + 1) < len; ++n)
      fingerprint.insert(static_cast<int>(5u * n + 4u), ' ');
  return fingerprint;
}

QGridLayout* VerifyKeyDetailBox::createKeyInfoGrid(
    gpgme_signature_t& signature) {
  auto grid = new QGridLayout();
  GpgKey key = GpgKeyGetter::GetInstance().GetKey(signature->fpr);

  if (!key.good()) return nullptr;
  grid->addWidget(new QLabel(tr("Signer Name:")), 0, 0);
  grid->addWidget(new QLabel(tr("Signer Email:")), 1, 0);
  grid->addWidget(new QLabel(tr("Key's Fingerprint:")), 2, 0);
  grid->addWidget(new QLabel(tr("Valid:")), 3, 0);
  grid->addWidget(new QLabel(tr("Flags:")), 4, 0);

  grid->addWidget(new QLabel(QString::fromStdString(key.name())), 0, 1);
  grid->addWidget(new QLabel(QString::fromStdString(key.email())), 1, 1);
  grid->addWidget(new QLabel(beautifyFingerprint(signature->fpr)), 2, 1);

  if (signature->summary & GPGME_SIGSUM_VALID) {
    grid->addWidget(new QLabel(tr("Fully Valid")), 3, 1);
  } else {
    grid->addWidget(new QLabel(tr("NOT Fully Valid")), 3, 1);
  }

  QString flags;
  QTextStream textStream(&flags);

  if (signature->summary & GPGME_SIGSUM_GREEN) {
    textStream << tr("Good ");
  }
  if (signature->summary & GPGME_SIGSUM_RED) {
    textStream << tr("Bad ");
  }
  if (signature->summary & GPGME_SIGSUM_SIG_EXPIRED) {
    textStream << tr("Expired ");
  }
  if (signature->summary & GPGME_SIGSUM_KEY_MISSING) {
    textStream << tr("Missing Key ");
  }
  if (signature->summary & GPGME_SIGSUM_KEY_REVOKED) {
    textStream << tr("Revoked Key ");
  }
  if (signature->summary & GPGME_SIGSUM_KEY_EXPIRED) {
    textStream << tr("Expired Key ");
  }
  if (signature->summary & GPGME_SIGSUM_CRL_MISSING) {
    textStream << tr("Missing CRL ");
  }

  grid->addWidget(new QLabel(tr(flags.toUtf8().constData())), 4, 1);
  return grid;
}

}  // namespace GpgFrontend::UI