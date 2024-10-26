/**
 * Copyright (C) 2021-2024 Saturneric <eric@bktus.com>
 *
 * This file is part of GpgFrontend.
 *
 * GpgFrontend is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * GpgFrontend is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GpgFrontend. If not, see <https://www.gnu.org/licenses/>.
 *
 * The initial version of the source code is inherited from
 * the gpg4usb project, which is under GPL-3.0-or-later.
 *
 * All the source code of GpgFrontend was modified and released by
 * Saturneric <eric@bktus.com> starting on May 12, 2021.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include "ui/widgets/VerifyKeyDetailBox.h"

#include "core/GpgModel.h"
#include "core/function/GlobalSettingStation.h"
#include "core/function/gpg/GpgKeyGetter.h"
#include "core/utils/CommonUtils.h"
#include "ui/UserInterfaceUtils.h"

namespace GpgFrontend::UI {

VerifyKeyDetailBox::VerifyKeyDetailBox(int channel,
                                       const GpgSignature& signature,
                                       QWidget* parent)
    : QGroupBox(parent),
      current_gpg_context_channel_(channel),
      fpr_(signature.GetFingerprint()) {
  auto* vbox = new QVBoxLayout();

  switch (gpg_err_code(signature.GetStatus())) {
    case GPG_ERR_NO_PUBKEY: {
      this->setTitle("A Error Signature");

      bool forbid_all_gnupg_connection =
          GlobalSettingStation::GetInstance()
              .GetSettings()
              .value("network/forbid_all_gnupg_connection", false)
              .toBool();

      auto* import_button = new QPushButton(tr("Import from keyserver"));
      import_button->setDisabled(forbid_all_gnupg_connection);
      connect(import_button, &QPushButton::clicked, this,
              &VerifyKeyDetailBox::slot_import_form_key_server);

      this->setTitle(tr("Key not present with id 0x") + fpr_);

      auto* grid = new QGridLayout();

      grid->addWidget(new QLabel(tr("Status") + tr(":")), 0, 0);
      // grid->addWidget(new QLabel(tr("Fingerprint:")), 1, 0);
      grid->addWidget(new QLabel(tr("Key not present in key list")), 0, 1);
      // grid->addWidget(new QLabel(signature->fpr), 1, 1);
      grid->addWidget(import_button, 2, 0, 2, 1);

      vbox->addLayout(grid);
      break;
    }
    case GPG_ERR_NO_ERROR: {
      this->setTitle(tr("A Signature") + ":");
      auto* gird = create_key_info_grid(signature);
      if (gird != nullptr) {
        vbox->addLayout(gird);
      } else {
        vbox->addWidget(new QLabel(tr("Key Information is NOT Available")));
        if (!signature.GetFingerprint().isEmpty()) {
          vbox->addWidget(new QLabel(tr("Fingerprint") + ": " +
                                     signature.GetFingerprint()));
        }
      }
      break;
    }
    case GPG_ERR_CERT_REVOKED: {
      this->setTitle("An Error Signature");
      vbox->addWidget(new QLabel(tr("Status") + ":" + tr("Cert Revoked")));
      auto* gird = create_key_info_grid(signature);
      if (gird != nullptr) {
        vbox->addLayout(gird);
      } else {
        vbox->addWidget(new QLabel(tr("Key Information is NOT Available")));
        if (!signature.GetFingerprint().isEmpty()) {
          vbox->addWidget(new QLabel(tr("Fingerprint") + ": " +
                                     signature.GetFingerprint()));
        }
      }
      break;
    }
    case GPG_ERR_SIG_EXPIRED: {
      this->setTitle("An Error Signature");
      vbox->addWidget(new QLabel(tr("Status") + ":" + tr("Signature Expired")));
      auto* gird = create_key_info_grid(signature);
      if (gird != nullptr) {
        vbox->addLayout(gird);
      } else {
        vbox->addWidget(new QLabel(tr("Key Information is NOT Available")));
        if (!signature.GetFingerprint().isEmpty()) {
          vbox->addWidget(new QLabel(tr("Fingerprint") + ": " +
                                     signature.GetFingerprint()));
        }
      }
      break;
    }
    case GPG_ERR_KEY_EXPIRED: {
      this->setTitle("An Error Signature");
      vbox->addWidget(new QLabel(tr("Status") + ":" + tr("Key Expired")));
      vbox->addWidget(new QLabel(tr("Status") + ":" + tr("Key Expired")));
      auto* gird = create_key_info_grid(signature);
      if (gird != nullptr) {
        vbox->addLayout(gird);
      } else {
        vbox->addWidget(new QLabel(tr("Key Information is NOT Available")));
        if (!signature.GetFingerprint().isEmpty()) {
          vbox->addWidget(new QLabel(tr("Fingerprint") + ": " +
                                     signature.GetFingerprint()));
        }
      }
      break;
    }
    case GPG_ERR_GENERAL: {
      this->setTitle("An Error Signature");
      vbox->addWidget(new QLabel(tr("Status") + ":" + tr("General Error")));
      auto* gird = create_key_info_grid(signature);
      if (gird != nullptr) {
        vbox->addLayout(gird);
      } else {
        vbox->addWidget(new QLabel(tr("Key Information is NOT Available")));
        if (!signature.GetFingerprint().isEmpty()) {
          vbox->addWidget(new QLabel(tr("Fingerprint") + ": " +
                                     signature.GetFingerprint()));
        }
      }
      break;
    }
    default: {
      this->setTitle("An Error Signature");
      this->setTitle(tr("Status") + ":" + tr("Unknown Error "));
      auto* gird = create_key_info_grid(signature);
      if (gird != nullptr) {
        vbox->addLayout(gird);
      } else {
        vbox->addWidget(new QLabel(tr("Key Information is NOT Available")));
        if (!signature.GetFingerprint().isEmpty()) {
          vbox->addWidget(new QLabel(tr("Fingerprint") + ": " +
                                     signature.GetFingerprint()));
        }
      }
      break;
    }
  }
  this->setLayout(vbox);
}

void VerifyKeyDetailBox::slot_import_form_key_server() {
  CommonUtils::GetInstance()->ImportKeyFromKeyServer(
      current_gpg_context_channel_, {fpr_});
}

auto VerifyKeyDetailBox::create_key_info_grid(const GpgSignature& signature)
    -> QGridLayout* {
  auto* grid = new QGridLayout();
  auto key =
      GpgKeyGetter::GetInstance(current_gpg_context_channel_).GetKey(fpr_);
  assert(key.IsGood());

  if (!key.IsGood()) return nullptr;
  grid->addWidget(new QLabel(tr("Signer Name") + ":"), 0, 0);
  grid->addWidget(new QLabel(tr("Signer Email") + ":"), 1, 0);
  grid->addWidget(new QLabel(tr("Key's Fingerprint") + ":"), 2, 0);
  grid->addWidget(new QLabel(tr("Valid") + ":"), 3, 0);
  grid->addWidget(new QLabel(tr("Flags") + ":"), 4, 0);

  grid->addWidget(new QLabel(key.GetName()), 0, 1);
  grid->addWidget(new QLabel(key.GetEmail()), 1, 1);
  grid->addWidget(new QLabel(BeautifyFingerprint(fpr_)), 2, 1);

  if ((signature.GetSummary() & GPGME_SIGSUM_VALID) != 0U) {
    grid->addWidget(new QLabel(tr("Fully Valid")), 3, 1);
  } else {
    grid->addWidget(new QLabel(tr("NOT Fully Valid")), 3, 1);
  }

  QString buffer;
  QTextStream text_stream(&buffer);

  if ((signature.GetSummary() & GPGME_SIGSUM_GREEN) != 0U) {
    text_stream << tr("Good") << " ";
  }
  if ((signature.GetSummary() & GPGME_SIGSUM_RED) != 0U) {
    text_stream << tr("Bad") << " ";
  }
  if ((signature.GetSummary() & GPGME_SIGSUM_SIG_EXPIRED) != 0U) {
    text_stream << tr("Expired") << " ";
  }
  if ((signature.GetSummary() & GPGME_SIGSUM_KEY_MISSING) != 0U) {
    text_stream << tr("Missing Key") << " ";
  }
  if ((signature.GetSummary() & GPGME_SIGSUM_KEY_REVOKED) != 0U) {
    text_stream << tr("Revoked Key") << " ";
  }
  if ((signature.GetSummary() & GPGME_SIGSUM_KEY_EXPIRED) != 0U) {
    text_stream << tr("Expired Key") << " ";
  }
  if ((signature.GetSummary() & GPGME_SIGSUM_CRL_MISSING) != 0U) {
    text_stream << tr("Missing CRL") << " ";
  }

  grid->addWidget(new QLabel(text_stream.readAll()), 4, 1);
  return grid;
}

}  // namespace GpgFrontend::UI