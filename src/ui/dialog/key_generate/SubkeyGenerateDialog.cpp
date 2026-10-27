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

#include "SubkeyGenerateDialog.h"

#include <cassert>

#include "core/function/openpgp/GpgKeyRepository.h"
#include "core/function/openpgp/KeyGenerationOperation.h"
#include "core/function/openpgp/support/KeyManagementOpSupport.h"
#include "core/utils/CommonUtils.h"
#include "core/utils/GpgUtils.h"
#include "ui/UISignalStation.h"
#include "ui/dialog/key_generate/KeyGenerateSummary.h"
#include "ui/function/GpgErrorMessageBox.h"
#include "ui/function/GpgOperaHelper.h"
#include "ui/function/KeyDatabaseRefresh.h"
#include "ui/function/KeyGenSettings.h"
#include "ui/function/UIStyle.h"
#include "ui/widgets/ChoiceTile.h"
#include "ui/widgets/ExpiryPicker.h"
#include "ui/widgets/KeyAlgoEditor.h"
#include "ui/widgets/ToggleSwitch.h"

namespace GpgFrontend::UI {

namespace {

auto PurposeId(SubkeyPurpose purpose) -> int {
  return static_cast<int>(purpose);
}

/// A quiet line with the info icon, for what the reader should know but need
/// not act on.
auto MakeNote(const QString& text, QWidget* parent) -> QLayout* {
  auto* row = new QHBoxLayout();
  row->setSpacing(8);
  auto* icon = new QLabel(parent);
  icon->setPixmap(
      QPixmap(u":/icons/info.png"_s)
          .scaled(16, 16, Qt::KeepAspectRatio, Qt::SmoothTransformation));
  icon->setFixedSize(16, 16);
  row->addWidget(icon, 0, Qt::AlignTop);
  row->addWidget(CreateDetailLabel(text, parent), 1);
  return row;
}

}  // namespace

SubkeyGenerateDialog::SubkeyGenerateDialog(int channel, GpgKeyPtr key,
                                           QWidget* parent)
    : GeneralDialog(typeid(SubkeyGenerateDialog).name(), parent),
      current_gpg_context_channel_(channel),
      key_(std::move(key)),
      gen_subkey_info_(SecureCreateSharedObject<KeyGenerateInfo>(true)),
      supported_subkey_algos_(KeyGenerateInfo::GetSupportedSubkeyAlgo(
          current_gpg_context_channel_, *key_)),
      primary_algo_(NormalizeKeyAlgoId(key_->Algo())),
      purpose_group_(new QButtonGroup(this)),
      editor_(new KeyAlgoEditor(channel, supported_subkey_algos_, this)),
      no_passphrase_check_(new QCheckBox(tr("Without a passphrase"), this)) {
  assert(key_ != nullptr);

  // A reload of the key databases replaces the channel this works on; find
  // the database again, or say it is gone.
  KeepBoundToKeyDatabase(
      this, [this]() { return current_gpg_context_channel_; },
      [this](int channel) { current_gpg_context_channel_ = channel; });

  setWindowTitle(tr("Add Subkey"));
  setAttribute(Qt::WA_DeleteOnClose);
  setModal(true);
  // Room for the header and each tile's sentence on one or two lines.
  setMinimumWidth(480);

  auto* layout = new QVBoxLayout(this);
  layout->setSpacing(10);

  const auto who = key_->Email().isEmpty()
                       ? key_->Name()
                       : u"%1 <%2>"_s.arg(key_->Name(), key_->Email());
  layout->addLayout(CreateDialogHeader(
      u":/icons/key_generate.png"_s, tr("Add Subkey"),
      tr("A new subkey for %1 (%2). Others who have your public key need its "
         "updated copy to use it.")
          .arg(who, key_->ID()),
      this));

  // Two views of one choice: the purposes, or with Expert Mode the editor.
  // Swapped, not stacked one under the other, so neither crowds the other.
  auto* views = new QStackedWidget(this);
  auto* easy = new QWidget(views);
  auto* easy_layout = new QVBoxLayout(easy);
  easy_layout->setContentsMargins(0, 0, 0, 0);
  easy_layout->setAlignment(Qt::AlignTop);
  easy_layout->addWidget(new QLabel(tr("What is the subkey for?"), easy));

  struct Purpose {
    SubkeyPurpose purpose;
    QString title;
    QString detail;
  };
  const QList<Purpose> purposes{
      {SubkeyPurpose::kEncrypt, tr("Encryption"),
       tr("For messages and files others send to you.")},
      {SubkeyPurpose::kSign, tr("Signing"),
       tr("For signing everyday, so the primary key can be kept somewhere "
          "safer.")},
      {SubkeyPurpose::kAuthenticate, tr("Authentication"),
       tr("For logging in, for example over SSH.")},
  };

  // Each a name and one sentence; the algorithm it picks is in the tooltip
  // and, with Expert Mode, in the editor below.
  for (const auto& p : purposes) {
    const auto algo =
        SuggestSubkeyAlgo(p.purpose, primary_algo_, supported_subkey_algos_);
    const auto available = algo != KeyGenerateInfo::kNoneAlgo;
    auto* tile = new ChoiceTile(
        p.title, available ? p.detail : tr("Not available for this key."), {},
        easy);
    tile->setEnabled(available);
    // The algorithm the purpose picks, quietly, as on the Key Type step.
    if (available) {
      tile->SetSecondary(DescribeKeyAlgo(algo, KeyGenerateInfo::kNoneAlgo));
    }
    purpose_group_->addButton(tile->Radio(), PurposeId(p.purpose));
    easy_layout->addWidget(tile);
  }

  auto* expert = new QWidget(views);
  auto* expert_layout = new QVBoxLayout(expert);
  expert_layout->setContentsMargins(0, 0, 0, 0);
  expert_layout->setAlignment(Qt::AlignTop);
  expert_layout->addWidget(new QLabel(
      tr("Choose the algorithm, key length and usage yourself."), expert));
  editor_->setParent(expert);
  editor_->SetExpiryVisible(false);  // the row below says when
  expert_layout->addWidget(editor_);
  no_passphrase_check_->setParent(expert);
  expert_layout->addWidget(no_passphrase_check_);

  views->addWidget(easy);
  views->addWidget(expert);
  layout->addWidget(views);

  const auto expiry_supported =
      IsOpSupported<SetExpireOpTag>(current_gpg_context_channel_);
  if (expiry_supported) {
    expiry_ = new ExpiryPicker(this);
    auto* expiry_row = new QFormLayout();
    expiry_row->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    expiry_row->addRow(tr("Expires"), expiry_);
    layout->addLayout(expiry_row);
  }

  const auto engine =
      OpenPGPContext::GetInstance(current_gpg_context_channel_).Engine();

  // The "subkey passphrase must equal the primary key's" constraint is a GnuPG
  // behaviour; rPGP does not impose it, so only show this tip for GnuPG.
  if (engine == OpenPGPEngine::kGNUPG) {
    layout->addLayout(
        MakeNote(tr("GnuPG asks for the key pair's passphrase. The subkey is "
                    "protected by the same one."),
                 this));
  }

  // Post-quantum (PQC) subkey algorithms require a v6 primary key; for a v4
  // primary key they are filtered out of the algorithm list above. Tell the
  // user why instead of leaving them silently absent. The version is 0 when it
  // could not be determined, so only show this when the key is known to be v4.
  if (GpgKeyRepository::GetInstance(current_gpg_context_channel_)
          .GetKeyVersion(key_->Fingerprint()) == 4) {
    layout->addLayout(MakeNote(
        tr("Post-quantum subkeys are not offered: this key uses the v4 key "
           "format. Generate a v6 key to use them."),
        this));
  }

  layout->addStretch(1);

  auto* buttons = new QDialogButtonBox(QDialogButtonBox::Cancel, this);
  generate_button_ =
      buttons->addButton(tr("Generate"), QDialogButtonBox::AcceptRole);
  generate_button_->setDefault(true);
  // Named for the manual's screenshots.
  generate_button_->setObjectName(u"generateButton"_s);

  // Expert Mode beside the buttons, as in the key generation wizard, and the
  // same remembered switch.
  expert_switch_ = new ToggleSwitch(tr("Expert Mode"), this);
  expert_switch_->setToolTip(
      tr("Choose the algorithm, key length and usage yourself."));
  auto* button_row = new QHBoxLayout();
  button_row->addWidget(expert_switch_);
  button_row->addStretch(1);
  button_row->addWidget(buttons);
  layout->addLayout(button_row);

  // A stack is as tall as its tallest view; only the one shown counts here,
  // so what follows sits right under it in either mode.
  const auto show_view = [views](bool on) {
    for (int i = 0; i < views->count(); ++i) {
      const auto current = i == (on ? 1 : 0);
      views->widget(i)->setSizePolicy(
          QSizePolicy::Preferred,
          current ? QSizePolicy::Preferred : QSizePolicy::Ignored);
    }
    views->setCurrentIndex(on ? 1 : 0);
    views->updateGeometry();
  };
  const auto expert_on = LoadKeyGenSettings().expert_mode;
  expert_switch_->setChecked(expert_on);
  show_view(expert_on);
  connect(expert_switch_, &QAbstractButton::toggled, this,
          [this, show_view](bool on) {
            show_view(on);
            StoreKeyGenExpertMode(on);
            fit_height();
          });

  editor_->SetInfo(gen_subkey_info_);

  connect(buttons, &QDialogButtonBox::accepted, this,
          &SubkeyGenerateDialog::slot_key_gen_accept);
  connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
  connect(purpose_group_, &QButtonGroup::idToggled, this,
          [this](int id, bool checked) {
            if (checked) apply_purpose(static_cast<SubkeyPurpose>(id));
          });
  connect(editor_, &KeyAlgoEditor::SignalChanged, this,
          &SubkeyGenerateDialog::mark_custom);
  if (expiry_ != nullptr) {
    connect(expiry_, &ExpiryPicker::SignalChanged, this, [this]() {
      gen_subkey_info_->SetExpireTime(expiry_->Expire());
      gen_subkey_info_->SetNonExpired(expiry_->NonExpired());
    });
  }
  connect(no_passphrase_check_, &QCheckBox::toggled, this,
          [this](bool on) { gen_subkey_info_->SetNonPassPhrase(on); });

  // The usual reason to add a subkey, when it can be made here.
  auto* first = purpose_group_->button(PurposeId(SubkeyPurpose::kEncrypt));
  if (first != nullptr && first->parentWidget()->isEnabled()) {
    first->setChecked(true);
  } else {
    // Nothing to suggest: the choice is the user's, so show it.
    editor_->Refresh();
    expert_switch_->setChecked(true);
    refresh_generate();
  }

  if (expiry_ != nullptr) {
    expiry_->SetExpiry(gen_subkey_info_->IsNonExpired(),
                       gen_subkey_info_->GetExpireTime());
  }
  no_passphrase_check_->setEnabled(gen_subkey_info_->IsAllowNoPassPhrase());

  show();
  raise();
  activateWindow();
  fit_height();
}

void SubkeyGenerateDialog::fit_height() {
  // As tall as what is shown and no taller: the views differ in height, and a
  // height remembered from a longer layout is only blank space. The width the
  // user chose stays. Queued, so the new view has been laid out first.
  QTimer::singleShot(0, this, [this]() {
    const auto needed = layout()->hasHeightForWidth()
                            ? layout()->totalHeightForWidth(width())
                            : sizeHint().height();
    if (needed > 0 && needed != height()) resize(width(), needed);
  });
}

void SubkeyGenerateDialog::apply_purpose(SubkeyPurpose purpose) {
  const auto algo =
      SuggestSubkeyAlgo(purpose, primary_algo_, supported_subkey_algos_);
  gen_subkey_info_->SetAlgo(algo);
  gen_subkey_info_->SetSubAlgo(KeyGenerateInfo::kNoneAlgo);

  // One job each: an algorithm that could also sign is not made to.
  gen_subkey_info_->SetAllowEncr(purpose == SubkeyPurpose::kEncrypt);
  gen_subkey_info_->SetAllowSign(purpose == SubkeyPurpose::kSign);
  gen_subkey_info_->SetAllowAuth(purpose == SubkeyPurpose::kAuthenticate);

  editor_->Refresh();
  refresh_generate();
}

void SubkeyGenerateDialog::mark_custom() {
  // Changed by hand: no purpose stands for it any more. An exclusive group
  // never lets its last checked button go.
  {
    const QSignalBlocker blocker(purpose_group_);
    purpose_group_->setExclusive(false);
    for (auto* button : purpose_group_->buttons()) button->setChecked(false);
    purpose_group_->setExclusive(true);
  }
  refresh_generate();
}

void SubkeyGenerateDialog::refresh_generate() {
  const auto usable = gen_subkey_info_->IsAllowEncr() ||
                      gen_subkey_info_->IsAllowSign() ||
                      gen_subkey_info_->IsAllowAuth();
  generate_button_->setEnabled(editor_->IsComplete() && usable);
}

void SubkeyGenerateDialog::slot_key_gen_accept() {
  if (!gen_subkey_info_->IsNonExpired() &&
      gen_subkey_info_->GetExpireTime() < CurrentDateTime().addSecs(120)) {
    QMessageBox::warning(
        this, tr("Add Subkey"),
        tr("The subkey would expire in less than two minutes. Choose a later "
           "date."));
    return;
  }

  // WaitForOpera() runs a nested event loop and returns only after the
  // completion callback below has run, so the flag reliably reflects the
  // outcome by the time it returns.
  auto succeeded = std::make_shared<bool>(false);

  GpgOperaHelper::WaitForOpera(
      this, tr("Generating"),
      [this, succeeded, key = key_,
       info = gen_subkey_info_](const OperaWaitingHd& hd) -> void {
        KeyGenerationOperation::GetInstance(current_gpg_context_channel_)
            .GenerateSubkey(
                key, info,
                [this, hd, succeeded](GpgError err,
                                      const DataObjectPtr& data_obj) -> void {
                  hd();

                  if (CheckGpgError(err) == GPG_ERR_USER_1) {
                    QMessageBox::critical(this, tr("Error"),
                                          tr("Unknown error occurred"));
                    return;
                  }

                  if (CheckGpgError(err) != GPG_ERR_NO_ERROR) {
                    // Reported on the still-open dialog, so the user can
                    // adjust and retry.
                    RaiseFailureMessageBox(this, err,
                                           KeyGenerationErrorDetail(data_obj));
                    return;
                  }

                  *succeeded = true;
                  emit UISignalStation::GetInstance()
                      -> SignalKeyDatabaseRefresh();
                });
      });

  if (!*succeeded) return;

  // Close the dialog first, then confirm to the user anchored to the parent
  // window, so the success notice doesn't appear layered over a dialog that is
  // about to disappear.
  auto* notify_parent = parentWidget();
  accept();

  QMessageBox::information(notify_parent, tr("Subkey Added"),
                           tr("The subkey was generated and added to the key "
                              "pair."));
}

}  // namespace GpgFrontend::UI
