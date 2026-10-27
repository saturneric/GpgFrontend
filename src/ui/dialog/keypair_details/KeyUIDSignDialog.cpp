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

#include "KeyUIDSignDialog.h"

#include "core/function/openpgp/KeyManagementOperation.h"
#include "core/utils/CommonUtils.h"
#include "core/utils/GpgUtils.h"
#include "ui/UISignalStation.h"
#include "ui/function/GpgOperaHelper.h"
#include "ui/function/KeyDatabaseRefresh.h"
#include "ui/function/UIStyle.h"
#include "ui/widgets/ExpiryPicker.h"
#include "ui/widgets/KeyList.h"

namespace GpgFrontend::UI {

namespace {

const auto kSignerColumns = GpgKeyTableColumn::kNAME |
                            GpgKeyTableColumn::kEMAIL_ADDRESS |
                            GpgKeyTableColumn::kKEY_ID;

}  // namespace

KeyUIDSignDialog::KeyUIDSignDialog(int channel, const GpgKeyPtr& key,
                                   const QString& uid, QWidget* parent)
    : GeneralDialog(typeid(KeyUIDSignDialog).name(), parent),
      current_gpg_context_channel_(channel),
      m_uid_(uid),
      m_key_(key) {
  // A reload of the key databases replaces the channel this works on; find
  // the database again, or say it is gone.
  KeepBoundToKeyDatabase(
      this, [this]() { return current_gpg_context_channel_; },
      [this](int channel) { current_gpg_context_channel_ = channel; });

  assert(m_key_ != nullptr);
  const auto key_id = m_key_->ID();
  const auto all_uids = m_uid_.isEmpty();

  const auto title = all_uids ? tr("Certify Key") : tr("Certify User ID");
  setWindowTitle(title);
  setModal(true);
  setAttribute(Qt::WA_DeleteOnClose, true);
  setMinimumWidth(560);

  auto* layout = new QVBoxLayout(this);
  layout->setSpacing(12);

  layout->addLayout(CreateDialogHeader(
      u":/icons/signature.png"_s, title,
      tr("Say that you have checked this key belongs to the person it names. "
         "Compare the fingerprint with them first, in person or over a "
         "channel you trust."),
      this));

  // What is being vouched for, with the fingerprint the user should have
  // compared: the one fact the whole act rests on.
  auto* target = new QWidget(this);
  auto* target_form = new QFormLayout(target);
  target_form->setContentsMargins(0, 0, 0, 0);
  target_form->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
  target_form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

  auto* uid_label = new QLabel(all_uids ? m_key_->UID() : m_uid_, target);
  uid_label->setTextFormat(Qt::PlainText);
  uid_label->setWordWrap(true);
  uid_label->setTextInteractionFlags(Qt::TextSelectableByMouse);
  target_form->addRow(all_uids ? tr("Key") : tr("User ID"), uid_label);

  if (all_uids && m_key_->UIDs().size() > 1) {
    target_form->addRow(QString{},
                        CreateDetailLabel(tr("All %1 user IDs of this key are "
                                             "certified.")
                                              .arg(m_key_->UIDs().size()),
                                          target));
  }

  auto* fpr_label =
      new QLabel(BeautifyFingerprint(m_key_->Fingerprint()), target);
  fpr_label->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
  fpr_label->setTextFormat(Qt::PlainText);
  fpr_label->setWordWrap(true);
  fpr_label->setTextInteractionFlags(Qt::TextSelectableByMouse);
  target_form->addRow(tr("Fingerprint"), fpr_label);

  layout->addWidget(CreateCard(tr("Certifying"), target, this));

  // Only the search bar: one fixed tab and three columns leave the rail and
  // the column filter nothing to do.
  m_key_list_ =
      new KeyList(channel, KeyMenuAbility::kSEARCH_BAR, kSignerColumns, this);
  m_key_list_->SetPersistenceScope("uid_sign", kSignerColumns);
  m_key_list_->AddListGroupTab(
      tr("Certifying Keys"), "signers", GpgKeyTableDisplayMode::kPRIVATE_KEY,
      [key_id](const GpgAbstractKey* key) -> bool {
        if (key->KeyType() != GpgAbstractKeyType::kGPG_KEY) return false;
        return !(key->IsDisabled() || !key->IsHasCertCap() ||
                 !dynamic_cast<const GpgKey*>(key)->IsHasMasterKey() ||
                 key->IsExpired() || key->IsRevoked() || key_id == key->ID());
      });
  m_key_list_->SetCategoryRailVisible(false);
  m_key_list_->SetChromeInset({0, 0, 0, 0}, {0, 0, 0, 0});
  m_key_list_->SetFilteredOutText(
      tr("None of your private keys can certify. A key needs its primary "
         "secret key, and must not be expired, revoked or disabled."));
  m_key_list_->SlotRefresh();
  // The usual case is one key of your own: nothing to choose.
  m_key_list_->CheckSoleKey();

  auto* signer_label = new QLabel(tr("Certify with"), this);
  auto signer_font = signer_label->font();
  signer_font.setBold(true);
  signer_label->setFont(signer_font);
  layout->addWidget(signer_label);
  layout->addWidget(m_key_list_, 1);

  // gpg's own default is a certification that never expires.
  expiry_picker_ = new ExpiryPicker(this);
  expiry_picker_->SetValidity(u"forever"_s);
  auto* expiry_form = new QFormLayout();
  expiry_form->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
  expiry_form->addRow(tr("Expires"), expiry_picker_);
  layout->addLayout(expiry_form);

  auto* buttons = new QDialogButtonBox(QDialogButtonBox::Cancel, this);
  sign_button_ =
      buttons->addButton(tr("Certify"), QDialogButtonBox::AcceptRole);
  sign_button_->setDefault(true);
  layout->addWidget(buttons);

  connect(buttons, &QDialogButtonBox::accepted, this,
          [this]() { slot_sign_key(true); });
  connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
  connect(m_key_list_, &KeyList::SignalKeyChecked, this,
          &KeyUIDSignDialog::refresh_sign_button);
  refresh_sign_button();

  connect(this, &KeyUIDSignDialog::SignalKeyUIDSignUpdate,
          UISignalStation::GetInstance(),
          &UISignalStation::SignalKeyDatabaseRefresh);
}

void KeyUIDSignDialog::refresh_sign_button() {
  sign_button_->setEnabled(!m_key_list_->GetCheckedKeys().isEmpty());
}

void KeyUIDSignDialog::slot_sign_key(bool) {
  // The signers are the ticked rows, not the highlighted ones.
  auto keys = m_key_list_->GetCheckedKeys();
  if (keys.isEmpty()) {
    QMessageBox::information(this, tr("No Key Selected"),
                             tr("Please check the key to certify with."));
    return;
  }

  std::optional<QDateTime> expires = std::nullopt;
  if (!expiry_picker_->NonExpired()) expires = expiry_picker_->Expire();

  auto f = [this, keys, expires](const OperaWaitingHd& hd) {
    KeyManagementOperation::GetInstance(current_gpg_context_channel_)
        .SignKey(m_key_, keys, m_uid_, expires,
                 [this, self = QPointer<KeyUIDSignDialog>(this), hd](
                     GpgError err, const DataObjectPtr&) {
                   hd();
                   // A reload of the key databases may have closed the dialog.
                   if (self == nullptr) return;

                   if (CheckGpgError(err) == GPG_ERR_CANCELED) {
                     // The user dismissed the passphrase prompt: nothing
                     // failed, and the dialog stays open for another try.
                     return;
                   }

                   // Plain text: auto-detected rich text would swallow the
                   // "<email>" of the user ID as a tag.
                   const auto target =
                       m_uid_.isEmpty() ? m_key_->UID() : m_uid_;

                   if (CheckGpgError(err) != GPG_ERR_NO_ERROR) {
                     QMessageBox box(
                         QMessageBox::Critical, tr("Certification Failed"),
                         tr("Failed to certify %1.").arg(target) + "\n\n" +
                             DescribeGpgErrCode(err).second,
                         QMessageBox::Ok, this);
                     box.setTextFormat(Qt::PlainText);
                     box.exec();
                     return;
                   }

                   auto* msg_box =
                       new QMessageBox(qobject_cast<QWidget*>(this->parent()));
                   msg_box->setAttribute(Qt::WA_DeleteOnClose);
                   msg_box->setStandardButtons(QMessageBox::Ok);
                   msg_box->setWindowTitle(tr("Certification Complete"));
                   msg_box->setTextFormat(Qt::PlainText);
                   msg_box->setText(tr("%1 has been certified.").arg(target));
                   msg_box->setModal(true);
                   msg_box->open();

                   emit SignalKeyUIDSignUpdate();
                   this->close();
                 });
  };
  GpgOperaHelper::WaitForOpera(this, tr("Certifying"), f);
}

}  // namespace GpgFrontend::UI
