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

#include "ui/widgets/KeyAlgoEditor.h"

#include "ui/function/KeyGenerateHelper.h"
#include "ui/widgets/ExpiryPicker.h"

namespace GpgFrontend::UI {

namespace {

auto ComboNameType(const QComboBox* combo) -> QPair<QString, QString> {
  const auto data = combo->currentData().toMap();
  return {data.value("name").toString(), data.value("type").toString()};
}

void SelectAlgo(QComboBox* combo, const KeyAlgo& algo) {
  for (int i = 0; i < combo->count(); ++i) {
    const auto data = combo->itemData(i).toMap();
    if (data.value("name").toString() == algo.Name() &&
        data.value("type").toString() == algo.Type()) {
      combo->setCurrentIndex(i);
      return;
    }
  }
  combo->setCurrentIndex(FirstSelectableComboIndex(combo));
}

auto Contains(const QList<KeyAlgo>& algos, const KeyAlgo& algo) -> bool {
  return std::any_of(algos.cbegin(), algos.cend(),
                     [&](const KeyAlgo& a) { return a == algo; });
}

auto MakeCombo(QWidget* parent) -> QComboBox* {
  auto* combo = new QComboBox(parent);
  combo->setMinimumContentsLength(14);
  combo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
  return combo;
}

}  // namespace

KeyAlgoEditor::KeyAlgoEditor(int channel, QList<KeyAlgo> algos, QWidget* parent)
    : QWidget(parent),
      channel_(channel),
      algos_([&algos]() {
        // "None" makes no key; a key or subkey that is not wanted is left
        // out by whoever owns this editor, not chosen in it.
        algos.removeIf([](const KeyAlgo& a) {
          return a == KeyGenerateInfo::kNoneAlgo || a.Id() == u"none"_s;
        });
        return std::move(algos);
      }()),
      form_(new QFormLayout(this)),
      algo_combo_(MakeCombo(this)),
      length_combo_(MakeCombo(this)),
      second_algo_combo_(MakeCombo(this)),
      second_length_combo_(MakeCombo(this)),
      usage_row_(new QWidget(this)),
      encr_check_(new QCheckBox(tr("Encrypt"), usage_row_)),
      sign_check_(new QCheckBox(tr("Sign"), usage_row_)),
      auth_check_(new QCheckBox(tr("Authenticate"), usage_row_)),
      expiry_(new ExpiryPicker(this)) {
  form_->setContentsMargins(0, 0, 0, 0);
  form_->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

  auto* usage = new QHBoxLayout(usage_row_);
  usage->setContentsMargins(0, 0, 0, 0);
  usage->setSpacing(12);
  usage->addWidget(encr_check_);
  usage->addWidget(sign_check_);
  usage->addWidget(auth_check_);
  usage->addStretch(1);

  form_->addRow(tr("Algorithm"), algo_combo_);
  form_->addRow(tr("Key length"), length_combo_);
  form_->addRow(tr("Paired with"), second_algo_combo_);
  form_->addRow(tr("Its key length"), second_length_combo_);
  form_->addRow(tr("Usage"), usage_row_);
  form_->addRow(tr("Expires"), expiry_);

  second_algo_combo_->setToolTip(
      tr("A hybrid algorithm combines a post-quantum algorithm with a "
         "classical one, so the key stays safe if either is broken."));

  PopulateAlgoComboBox(algo_combo_, algos_);

  connect(algo_combo_, &QComboBox::currentIndexChanged, this, [this]() {
    if (refreshing_ || info_ == nullptr) return;
    const auto [name, type] = ComboNameType(algo_combo_);
    auto [found, algo] = GetAlgoByNameType(name, type, algos_);
    info_->SetAlgo(found ? algo : KeyGenerateInfo::kNoneAlgo);
    info_->SetSubAlgo(KeyGenerateInfo::kNoneAlgo);
    changed();
  });

  connect(length_combo_, &QComboBox::currentIndexChanged, this, [this]() {
    if (refreshing_ || info_ == nullptr) return;
    const auto [name, type] = ComboNameType(algo_combo_);
    auto [found, algo] = GetAlgoByNameTypeAndKeyLength(
        name, type, length_combo_->currentData().toInt(), algos_);
    if (!found) return;

    // A pairing that the new length still allows is kept.
    const auto old_sub = info_->SubAlgo();
    info_->SetAlgo(algo);
    info_->SetSubAlgo(Contains(algo.SubAlgos(channel_), old_sub)
                          ? old_sub
                          : KeyGenerateInfo::kNoneAlgo);
    changed();
  });

  connect(second_algo_combo_, &QComboBox::currentIndexChanged, this, [this]() {
    if (refreshing_ || info_ == nullptr) return;
    const auto [name, type] = ComboNameType(second_algo_combo_);
    auto [found, algo] =
        GetAlgoByNameType(name, type, info_->GetAlgo().SubAlgos(channel_));
    info_->SetSubAlgo(found ? algo : KeyGenerateInfo::kNoneAlgo);
    changed();
  });

  connect(second_length_combo_, &QComboBox::currentIndexChanged, this,
          [this]() {
            if (refreshing_ || info_ == nullptr) return;
            const auto [name, type] = ComboNameType(second_algo_combo_);
            auto [found, algo] = GetAlgoByNameTypeAndKeyLength(
                name, type, second_length_combo_->currentData().toInt(),
                info_->GetAlgo().SubAlgos(channel_));
            if (found) info_->SetSubAlgo(algo);
            changed();
          });

  const auto bind_usage = [this](QCheckBox* check,
                                 void (KeyGenerateInfo::*set)(bool)) {
    connect(check, &QCheckBox::toggled, this, [this, set](bool on) {
      if (refreshing_ || info_ == nullptr) return;
      ((*info_).*set)(on);
      changed();
    });
  };
  bind_usage(encr_check_, &KeyGenerateInfo::SetAllowEncr);
  bind_usage(sign_check_, &KeyGenerateInfo::SetAllowSign);
  bind_usage(auth_check_, &KeyGenerateInfo::SetAllowAuth);

  connect(expiry_, &ExpiryPicker::SignalChanged, this, [this]() {
    if (refreshing_ || info_ == nullptr) return;
    // The date first: clearing "never" keeps a date that is ahead.
    info_->SetExpireTime(expiry_->Expire());
    info_->SetNonExpired(expiry_->NonExpired());
    changed();
  });

  setEnabled(false);
}

void KeyAlgoEditor::SetInfo(QSharedPointer<KeyGenerateInfo> info) {
  info_ = std::move(info);
  setEnabled(info_ != nullptr);
  Refresh();
}

auto KeyAlgoEditor::Info() const -> QSharedPointer<KeyGenerateInfo> {
  return info_;
}

void KeyAlgoEditor::Refresh() {
  if (info_ == nullptr) return;
  refreshing_ = true;

  // An algorithm not on offer here (a profile from another engine, the
  // initial "none") becomes the first one that is.
  if (!Contains(algos_, info_->GetAlgo())) {
    const auto first = FirstSelectableComboIndex(algo_combo_);
    if (first >= 0) {
      const auto data = algo_combo_->itemData(first).toMap();
      auto [found, algo] = GetAlgoByNameType(data.value("name").toString(),
                                             data.value("type").toString(),
                                             algos_);
      if (found) info_->SetAlgo(algo);
    }
  }

  SelectAlgo(algo_combo_, info_->GetAlgo());

  const auto [name, type] = ComboNameType(algo_combo_);
  SetKeyLengthComboxBoxByAlgo(length_combo_,
                              SearchAlgoByNameType(name, type, algos_));
  length_combo_->setCurrentIndex(
      std::max(0, length_combo_->findData(info_->GetKeyLength())));
  // One length is no choice; showing it greyed says what it is all the same.
  length_combo_->setEnabled(length_combo_->count() > 1);

  // A hybrid needs its second half; one that has none gets the first.
  const auto sub_algos = info_->GetAlgo().SubAlgos(channel_);
  if (!sub_algos.isEmpty() && !Contains(sub_algos, info_->SubAlgo())) {
    info_->SetSubAlgo(sub_algos.front());
  } else if (sub_algos.isEmpty()) {
    info_->SetSubAlgo(KeyGenerateInfo::kNoneAlgo);
  }

  set_row_visible(second_algo_combo_, !sub_algos.isEmpty());
  set_row_visible(second_length_combo_, !sub_algos.isEmpty());
  if (!sub_algos.isEmpty()) {
    PopulateAlgoComboBox(second_algo_combo_, sub_algos);
    SelectAlgo(second_algo_combo_, info_->SubAlgo());

    const auto [s_name, s_type] = ComboNameType(second_algo_combo_);
    SetKeyLengthComboxBoxByAlgo(second_length_combo_,
                                SearchAlgoByNameType(s_name, s_type, sub_algos));
    second_length_combo_->setCurrentIndex(std::max(
        0, second_length_combo_->findData(info_->SubAlgo().KeyLength())));
    second_length_combo_->setEnabled(second_length_combo_->count() > 1);
  }

  const auto show_usage = [](QCheckBox* check, bool on, bool can_change) {
    check->setChecked(on);
    check->setEnabled(can_change);
  };
  show_usage(encr_check_, info_->IsAllowEncr(), info_->IsAllowModifyEncr());
  show_usage(sign_check_, info_->IsAllowSign(), info_->IsAllowModifySign());
  show_usage(auth_check_, info_->IsAllowAuth(), info_->IsAllowModifyAuth());

  expiry_->SetExpiry(info_->IsNonExpired(), info_->GetExpireTime());

  refreshing_ = false;
}

void KeyAlgoEditor::SetExpiryVisible(bool visible) {
  set_row_visible(expiry_, visible);
}

void KeyAlgoEditor::SetUsageVisible(bool visible) {
  set_row_visible(usage_row_, visible);
}

auto KeyAlgoEditor::IsComplete() const -> bool {
  if (info_ == nullptr || info_->GetAlgo() == KeyGenerateInfo::kNoneAlgo) {
    return false;
  }
  const auto sub_algos = info_->GetAlgo().SubAlgos(channel_);
  return sub_algos.isEmpty() || Contains(sub_algos, info_->SubAlgo());
}

void KeyAlgoEditor::set_row_visible(QWidget* field, bool visible) {
  form_->setRowVisible(field, visible);
}

void KeyAlgoEditor::changed() {
  // What one choice allows decides the rest: the lengths, the pairing, which
  // usages can be changed.
  Refresh();
  emit SignalChanged();
}

}  // namespace GpgFrontend::UI
