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

#include "KeysPicker.h"

#include "core/function/openpgp/AbstractKeyRepository.h"
#include "ui/function/KeyDatabaseRefresh.h"
#include "ui/widgets/KeyTreeView.h"

namespace GpgFrontend::UI {

namespace {

auto CollectFingerprints(const GpgAbstractKeyPtrList& keys) -> QSet<QString> {
  QSet<QString> ret;
  for (const auto& k : keys) {
    if (k == nullptr) continue;
    ret.insert(k->Fingerprint());
  }
  return ret;
}

auto IsKeyOrSubkey(const GpgAbstractKey* k) -> bool {
  return k->KeyType() == GpgAbstractKeyType::kGPG_KEY ||
         k->KeyType() == GpgAbstractKeyType::kGPG_SUBKEY;
}

}  // namespace

KeysPicker::KeysPicker(const QString& name, int channel,
                       const GpgAbstractKeyPtrList& restrict_keys,
                       const Texts& texts, const Filter& row_filter,
                       const Filter& key_filter, QWidget* parent)
    : GeneralDialog(name, parent),
      channel_(channel),
      tree_view_(new KeyTreeView(
          channel,
          [row_filter](GpgAbstractKey* k) -> bool {
            return k != nullptr && IsKeyOrSubkey(k) && row_filter(k);
          },
          [row_filter, key_filter,
           allowed = CollectFingerprints(restrict_keys)](
              const GpgAbstractKey* k) -> bool {
            if (k == nullptr || !k->IsGood()) return false;
            // Subkey rows are only reached when their primary key already
            // passed this filter, so restricting at the primary level is
            // enough.
            if (k->KeyType() == GpgAbstractKeyType::kGPG_SUBKEY) {
              return row_filter(k);
            }
            if (!key_filter(k)) return false;
            return allowed.isEmpty() || allowed.contains(k->Fingerprint());
          })) {
  // A reload of the key databases replaces the channel this works on; find
  // the database again, or say it is gone.
  KeepBoundToKeyDatabase(
      this, [this]() { return channel_; },
      [this](int channel) { channel_ = channel; });

  setWindowTitle(texts.window_title);
  setModal(true);
  setMinimumSize(560, 420);
  resize(700, 500);

  auto* title_label = new QLabel(texts.heading);
  auto title_font = title_label->font();
  title_font.setBold(true);
  title_label->setFont(title_font);

  auto* desc_label = new QLabel(texts.description);
  desc_label->setWordWrap(true);

  auto tips_palette = desc_label->palette();
  tips_palette.setColor(
      QPalette::WindowText,
      palette().color(QPalette::Disabled, QPalette::WindowText));
  desc_label->setPalette(tips_palette);

  auto* button_box = new QDialogButtonBox(
      QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);

  confirm_btn_ = button_box->button(QDialogButtonBox::Ok);
  confirm_btn_->setText(texts.confirm);
  confirm_btn_->setEnabled(false);

  button_box->button(QDialogButtonBox::Cancel)->setText(texts.cancel);

  connect(button_box, &QDialogButtonBox::accepted, this, [this, texts]() {
    if (tree_view_ == nullptr || tree_view_->GetAllCheckedKeys().isEmpty()) {
      QMessageBox::information(this, texts.none_selected_title,
                               texts.none_selected);
      return;
    }
    accept();
  });

  connect(button_box, &QDialogButtonBox::rejected, this, &QDialog::reject);

  connect(tree_view_, &KeyTreeView::SignalKeysChecked, this,
          &KeysPicker::update_confirm_button_state);

  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(10, 10, 10, 10);
  layout->setSpacing(8);

  layout->addWidget(title_label);
  layout->addWidget(desc_label);
  layout->addWidget(tree_view_, 1);
  layout->addWidget(button_box);

  setAttribute(Qt::WA_DeleteOnClose, false);

  update_confirm_button_state();
}

auto KeysPicker::GetKeys() const -> GpgAbstractKeyPtrList {
  if (tree_view_ == nullptr) return {};

  const auto keys = tree_view_->GetAllCheckedKeys();
  GpgAbstractKeyPtrList ret;
  auto& getter = AbstractKeyRepository::GetInstance(channel_);

  for (const auto& k : keys) {
    if (k == nullptr || !IsKeyOrSubkey(k.get())) continue;
    auto key = getter.GetKey(k->Fingerprint() + "!");
    if (key != nullptr) ret.push_back(key);
  }

  return ret;
}

void KeysPicker::update_confirm_button_state() {
  if (confirm_btn_ == nullptr || tree_view_ == nullptr) return;
  confirm_btn_->setEnabled(!tree_view_->GetAllCheckedKeys().isEmpty());
}

void KeysPicker::showEvent(QShowEvent* event) {
  GeneralDialog::showEvent(event);
  if (!isRectRestored()) movePosition2CenterOfParent();
}

EncryptionKeysPicker::EncryptionKeysPicker(
    int channel, const GpgAbstractKeyPtrList& restrict_keys, QWidget* parent)
    : KeysPicker(
          "EncryptionKeysPicker", channel, restrict_keys,
          {tr("Select Encryption Key(s)"),
           tr("Choose Encryption Key(s) or Subkey(s)"),
           tr("Select the recipient key(s) or specific encryption subkey(s) to "
              "use for this operation. Expand a key to choose a particular "
              "subkey."),
           tr("Confirm"), tr("Cancel"), tr("No Key Selected"),
           tr("Please select at least one encryption key or subkey.")},
          [](const GpgAbstractKey* k) {
            return k->IsHasEncrCap() && !k->IsRevoked();
          },
          [](const GpgAbstractKey* k) { return k->IsHasEncrCap(); }, parent) {}

SigningKeysPicker::SigningKeysPicker(int channel, QWidget* parent)
    : SigningKeysPicker(channel, {}, parent) {}

SigningKeysPicker::SigningKeysPicker(int channel,
                                     const GpgAbstractKeyPtrList& restrict_keys,
                                     QWidget* parent)
    : KeysPicker(
          "SigningKeysPicker", channel, restrict_keys,
          {tr("Select Signing Key(s)"),
           tr("Choose Signing Key(s) or Subkey(s)"),
           tr("Select the private key(s) or specific signing subkey(s) to use "
              "for this operation. Expand a key to choose a particular "
              "subkey."),
           tr("Confirm"), tr("Cancel"), tr("No Key Selected"),
           tr("Please select at least one signing key or subkey.")},
          [](const GpgAbstractKey* k) { return k->IsHasSignCap(); },
          [](const GpgAbstractKey* k) {
            return k->IsPrivateKey() && k->IsHasSignCap();
          },
          parent) {}

}  // namespace GpgFrontend::UI
