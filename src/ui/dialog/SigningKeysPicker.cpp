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

#include "SigningKeysPicker.h"

#include "core/function/openpgp/AbstractKeyRepository.h"
#include "ui/widgets/KeyTreeView.h"

namespace GpgFrontend::UI {

SigningKeysPicker::SigningKeysPicker(int channel, QWidget* parent)
    : GeneralDialog("SigningKeysPicker", parent),
      channel_(channel),
      tree_view_(new KeyTreeView(
          channel,
          [](GpgAbstractKey* k) -> bool {
            return k != nullptr &&
                   (k->KeyType() == GpgAbstractKeyType::kGPG_KEY ||
                    k->KeyType() == GpgAbstractKeyType::kGPG_SUBKEY) &&
                   k->IsHasSignCap();
          },
          [](const GpgAbstractKey* k) -> bool {
            if (k == nullptr || !k->IsGood()) return false;
            if (k->KeyType() == GpgAbstractKeyType::kGPG_SUBKEY) {
              return k->IsHasSignCap();
            }
            return k->IsPrivateKey() && k->IsHasSignCap();
          })) {
#ifdef Q_OS_MACOS
  setWindowFlag(Qt::WindowContextHelpButtonHint, false);
#endif

  setWindowTitle(tr("Select Signing Key(s)"));
  setModal(true);
  setMinimumSize(560, 420);
  resize(700, 500);

  auto* title_label = new QLabel(tr("Choose Signing Key(s) or Subkey(s)"));
  auto title_font = title_label->font();
  title_font.setBold(true);
  title_label->setFont(title_font);

  auto* desc_label = new QLabel(
      tr("Select the private key(s) or specific signing subkey(s) to use for "
         "this operation. Expand a key to choose a particular subkey."));
  desc_label->setWordWrap(true);

  auto tips_palette = desc_label->palette();
  tips_palette.setColor(
      QPalette::WindowText,
      palette().color(QPalette::Disabled, QPalette::WindowText));
  desc_label->setPalette(tips_palette);

  auto* button_box = new QDialogButtonBox(
      QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);

  confirm_btn_ = button_box->button(QDialogButtonBox::Ok);
  confirm_btn_->setText(tr("Confirm"));
  confirm_btn_->setEnabled(false);

  button_box->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));

  connect(button_box, &QDialogButtonBox::accepted, this, [this]() {
    if (tree_view_ == nullptr || tree_view_->GetAllCheckedKeys().isEmpty()) {
      QMessageBox::information(
          this, tr("No Key Selected"),
          tr("Please select at least one signing key or subkey."));
      return;
    }
    accept();
  });

  connect(button_box, &QDialogButtonBox::rejected, this, &QDialog::reject);

  connect(tree_view_, &KeyTreeView::SignalKeysChecked, this,
          &SigningKeysPicker::update_confirm_button_state);

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

auto SigningKeysPicker::GetSigningKeys() const -> GpgAbstractKeyPtrList {
  if (tree_view_ == nullptr) return {};

  const auto keys = tree_view_->GetAllCheckedKeys();
  GpgAbstractKeyPtrList ret;
  auto& getter = AbstractKeyRepository::GetInstance(channel_);

  for (const auto& k : keys) {
    if (k == nullptr) continue;
    if (k->KeyType() != GpgAbstractKeyType::kGPG_KEY &&
        k->KeyType() != GpgAbstractKeyType::kGPG_SUBKEY) {
      continue;
    }
    auto key = getter.GetKey(k->Fingerprint() + "!");
    if (key != nullptr) ret.push_back(key);
  }

  return ret;
}

void SigningKeysPicker::update_confirm_button_state() {
  if (confirm_btn_ == nullptr || tree_view_ == nullptr) return;
  confirm_btn_->setEnabled(!tree_view_->GetAllCheckedKeys().isEmpty());
}

void SigningKeysPicker::showEvent(QShowEvent* event) {
  GeneralDialog::showEvent(event);
  if (!isRectRestored()) movePosition2CenterOfParent();
}

}  // namespace GpgFrontend::UI
