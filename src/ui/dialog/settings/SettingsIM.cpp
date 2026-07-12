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

#include "SettingsIM.h"

#include "core/GFConstants.h"
#include "core/function/GlobalSettingStation.h"
#include "core/function/openpgp/GpgKeyRepository.h"
#include "core/model/GpgKey.h"

namespace GpgFrontend::UI {

InstantMessagingTab::InstantMessagingTab(QWidget* parent) : QWidget(parent) {
  // Mode: forward secrecy (Double Ratchet) on by default. Turning it off falls
  // back to plain, stateless PGP-in-a-token messages (the classic behaviour).
  auto* mode_box = new QGroupBox(tr("Mode"), this);
  auto* mode_form = new QFormLayout(mode_box);
  forward_secrecy_check_ =
      new QCheckBox(tr("Enable forward secrecy (Double Ratchet)"), mode_box);
  mode_form->addRow(forward_secrecy_check_);
  auto* mode_note = new QLabel(
      tr("When enabled, instant messages use a forward-secret session so past "
         "messages stay safe even if a key is later compromised. When "
         "disabled, "
         "messages are plain OpenPGP wrapped in a compact token, with no "
         "forward secrecy and no signing key required."),
      mode_box);
  mode_note->setWordWrap(true);
  mode_form->addRow(mode_note);

  // Identity: the OpenPGP key that signs (authenticates) our forward-secret
  // handshakes. This is who the recipient sees and trusts as the sender.
  auto* id_box = new QGroupBox(tr("Identity"), this);
  auto* id_form = new QFormLayout(id_box);
  identity_key_combo_ = new QComboBox(id_box);
  id_form->addRow(tr("Signing Key:"), identity_key_combo_);
  auto* id_note = new QLabel(
      tr("The private OpenPGP key used to sign your instant-messaging "
         "sessions, binding them to your identity. You must choose one before "
         "sending instant messages."),
      id_box);
  id_note->setWordWrap(true);
  id_form->addRow(id_note);

  auto* book_box = new QGroupBox(tr("Message Book"), this);
  auto* book_form = new QFormLayout(book_box);
  book_phrase_edit_ = new QLineEdit(book_box);
  book_phrase_edit_->setPlaceholderText(
      tr("Leave empty to use the shared default book"));
  book_form->addRow(tr("Message Book Phrase:"), book_phrase_edit_);
  auto* note = new QLabel(
      tr("Adds a shared secret to forward-secret instant-messaging sessions. "
         "Both sides must set the same phrase; empty means the shared default. "
         "Forward secrecy is established automatically — just pick a key and "
         "encrypt; no setup needed."),
      book_box);
  note->setWordWrap(true);
  book_form->addRow(note);

  auto* layout = new QVBoxLayout(this);
  layout->addWidget(mode_box);
  layout->addWidget(id_box);
  layout->addWidget(book_box);
  layout->addStretch(1);
  setLayout(layout);

  SetSettings();
}

void InstantMessagingTab::populate_identity_keys() {
  identity_key_combo_->clear();
  // Placeholder: a signing key must be chosen explicitly before instant
  // messaging can be used; there is no automatic fallback.
  identity_key_combo_->addItem(tr("— Select a signing key —"), QString{});

  const auto keys =
      GpgKeyRepository::GetInstance(kGpgFrontendDefaultChannel).Fetch();
  for (const auto& key : keys) {
    if (key == nullptr || !key->IsGood() || !key->IsPrivateKey() ||
        !key->IsHasSignCap()) {
      continue;
    }
    const auto label =
        QStringLiteral("%1 <%2> (%3)")
            .arg(key->Name(), key->Email(), key->Fingerprint().right(16));
    identity_key_combo_->addItem(label, key->Fingerprint());
  }
}

void InstantMessagingTab::SetSettings() {
  populate_identity_keys();

  auto settings = GetSettings();

  forward_secrecy_check_->setChecked(
      settings.value("im/forward_secrecy", false).toBool());

  const auto fpr = settings.value("im/identity_key_fpr").toString();
  const int idx = identity_key_combo_->findData(fpr);
  identity_key_combo_->setCurrentIndex(idx >= 0 ? idx : 0);

  book_phrase_edit_->setText(
      settings.value("im/password_book_phrase").toString());
}

void InstantMessagingTab::ApplySettings() {
  auto settings = GetSettings();
  settings.setValue("im/forward_secrecy", forward_secrecy_check_->isChecked());
  settings.setValue("im/identity_key_fpr",
                    identity_key_combo_->currentData().toString());
  settings.setValue("im/password_book_phrase",
                    book_phrase_edit_->text().trimmed());
}

}  // namespace GpgFrontend::UI
