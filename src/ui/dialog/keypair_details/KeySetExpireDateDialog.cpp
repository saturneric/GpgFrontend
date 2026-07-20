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

#include "KeySetExpireDateDialog.h"

#include "core/function/openpgp/KeyManagementOperation.h"
#include "core/utils/GpgUtils.h"
#include "ui/UISignalStation.h"

namespace GpgFrontend::UI {

namespace {

/// gpg reports a key that never expires as expiration time zero.
constexpr auto kNeverExpireSecs = 0;

/// A new expiration date this close to now is almost certainly a mistake, and
/// gpg would round it away anyway.
constexpr auto kMinimumValiditySecs = 60;

/// Unambiguous and compact. The locale's own formats are either two-digit-year
/// short ("7/12/28") or a full sentence with the timezone name spelled out,
/// neither of which belongs next to a fingerprint.
constexpr auto kDateTimeFormat = "yyyy-MM-dd HH:mm";

auto FormatDateTime(const QDateTime& date_time) -> QString {
  return QLocale().toString(date_time, kDateTimeFormat);
}
}  // namespace

KeySetExpireDateDialog::KeySetExpireDateDialog(int channel, GpgKeyPtr key,
                                               QWidget* parent)
    : GeneralDialog(typeid(KeySetExpireDateDialog).name(), parent),
      current_gpg_context_channel_(channel),
      m_key_(std::move(key)) {
  assert(m_key_->IsGood());
  init();
}

KeySetExpireDateDialog::KeySetExpireDateDialog(int channel, GpgKeyPtr key,
                                               QString subkey_fpr,
                                               QWidget* parent)
    : GeneralDialog(typeid(KeySetExpireDateDialog).name(), parent),
      current_gpg_context_channel_(channel),
      m_key_(std::move(key)),
      m_subkey_(std::move(subkey_fpr)) {
  assert(m_key_ != nullptr);
  init();
}

auto KeySetExpireDateDialog::current_expiration_time() const -> QDateTime {
  if (m_subkey_.isEmpty()) return m_key_->ExpirationTime();

  // A subkey carries its own expiration; reading it from the primary key would
  // pre-fill the dialog with an unrelated date.
  for (const auto& subkey : m_key_->SubKeys()) {
    if (subkey.Fingerprint() == m_subkey_) return subkey.ExpirationTime();
  }
  return m_key_->ExpirationTime();
}

auto KeySetExpireDateDialog::humanize_remaining(const QDateTime& target)
    -> QString {
  const auto now = QDateTime::currentDateTime();
  if (target <= now) return {};

  const auto from = now.date();
  const auto to = target.date();

  // Calendar months vary in length, so walk them instead of dividing days.
  int months = 0;
  while (from.addMonths(months + 1) <= to) ++months;

  const int years = months / 12;
  months %= 12;
  const auto days = from.addMonths((years * 12) + months).daysTo(to);

  // Spelled out per number instead of tr("%n year(s)"): without a translator
  // loaded Qt substitutes the count but keeps the literal "(s)", which reads as
  // "1 year(s)" in the untranslated build.
  QStringList parts;
  if (years > 0) {
    parts << (years == 1 ? tr("1 year") : tr("%1 years").arg(years));
  }
  if (months > 0) {
    parts << (months == 1 ? tr("1 month") : tr("%1 months").arg(months));
  }
  // Days would only add noise once the span is measured in years.
  if (years == 0 && (days > 0 || parts.isEmpty())) {
    parts << (days == 1 ? tr("1 day") : tr("%1 days").arg(days));
  }

  return parts.join(tr(", "));
}

void KeySetExpireDateDialog::init() {
  const auto is_subkey = !m_subkey_.isEmpty();
  const auto current_expire = current_expiration_time();
  const auto currently_non_expired =
      current_expire.toSecsSinceEpoch() == kNeverExpireSecs;

  non_expired_ = currently_non_expired;
  // Keep the date the key already carries so "Never Expires" can be turned off
  // without losing it. A key that never expires gets a sensible default.
  chosen_date_time_ =
      currently_non_expired || current_expire <= QDateTime::currentDateTime()
          ? QDateTime::currentDateTime().addYears(2)
          : current_expire;

  auto* title_label = new QLabel(is_subkey ? tr("Subkey Expiration Date")
                                           : tr("Key Expiration Date"));
  auto title_font = title_label->font();
  title_font.setBold(true);
  title_label->setFont(title_font);

  auto* hint_label = new QLabel(
      is_subkey
          ? tr("The subkey can no longer sign or encrypt after this moment. "
               "You can extend it again later.")
          : tr("The key can no longer sign or encrypt after this moment. "
               "You can extend it again later."));
  hint_label->setWordWrap(true);

  current_label_ = new QLabel();
  current_label_->setWordWrap(true);

  validity_period_combo_box_ = new QComboBox();
  for (const auto& period : k_validity_periods_) {
    validity_period_combo_box_->addItem(period.display, period.key);
  }

  expire_date_time_edit_ = new QDateTimeEdit();
  expire_date_time_edit_->setCalendarPopup(true);
  expire_date_time_edit_->setDisplayFormat(kDateTimeFormat);
  // The minimum is a date-time on a date-time editor, so no hidden time
  // component can silently push the date forward.
  expire_date_time_edit_->setMinimumDateTime(
      QDateTime::currentDateTime().addSecs(kMinimumValiditySecs));
  expire_date_time_edit_->setMaximumDateTime(
      QDateTime(QDate(2099, 12, 31), QTime(23, 59)));

  summary_label_ = new QLabel();
  summary_label_->setWordWrap(true);

  button_box_ =
      new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

  auto* top_line = new QFrame();
  top_line->setFrameShape(QFrame::HLine);
  top_line->setFrameShadow(QFrame::Sunken);
  auto* bottom_line = new QFrame();
  bottom_line->setFrameShape(QFrame::HLine);
  bottom_line->setFrameShadow(QFrame::Sunken);

  auto* form_layout = new QFormLayout();
  form_layout->setContentsMargins(0, 0, 0, 0);
  form_layout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
  form_layout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
  form_layout->addRow(tr("Validity Period"), validity_period_combo_box_);
  form_layout->addRow(tr("Expires On"), expire_date_time_edit_);

  // Header, then the controls, then the outcome: three bands separated by
  // rules, so the eye lands on the summary line last.
  auto* header_layout = new QVBoxLayout();
  header_layout->setSpacing(4);
  header_layout->addWidget(title_label);
  header_layout->addWidget(hint_label);
  header_layout->addWidget(current_label_);

  auto* layout = new QVBoxLayout();
  layout->setSpacing(12);
  layout->addLayout(header_layout);
  layout->addWidget(top_line);
  layout->addLayout(form_layout);
  layout->addWidget(bottom_line);
  layout->addWidget(summary_label_);
  layout->addWidget(button_box_);
  // No stretch: GeneralDialog adjustSize()s on first show, so the dialog ends
  // up hugging its content instead of leaving the dead band of empty space the
  // old fixed-geometry form had. Deliberately no QLayout::SetMinimumSize here —
  // it would overwrite the minimum width below and squash the text into wraps.

  connect(validity_period_combo_box_, &QComboBox::currentIndexChanged, this,
          &KeySetExpireDateDialog::slot_validity_period_changed);
  connect(expire_date_time_edit_, &QDateTimeEdit::dateTimeChanged, this,
          &KeySetExpireDateDialog::slot_expire_date_time_edited);
  connect(button_box_, &QDialogButtonBox::accepted, this,
          &KeySetExpireDateDialog::slot_confirm);
  connect(button_box_, &QDialogButtonBox::rejected, this,
          &KeySetExpireDateDialog::reject);
  connect(this, &KeySetExpireDateDialog::SignalKeyExpireDateUpdated,
          UISignalStation::GetInstance(),
          &UISignalStation::SignalKeyDatabaseRefresh);

  if (currently_non_expired) {
    current_label_->setText(tr("Currently set to never expire."));
  } else {
    const auto remaining = humanize_remaining(current_expire);
    current_label_->setText(
        remaining.isEmpty()
            ? tr("Currently expired since %1.")
                  .arg(FormatDateTime(current_expire))
            : tr("Currently expires %1 · %2 left")
                  .arg(FormatDateTime(current_expire), remaining));
  }

  const auto index = validity_period_combo_box_->findData(
      currently_non_expired ? QString("never") : QString("custom"));
  validity_period_combo_box_->setCurrentIndex(index);

  refresh_widgets_state();

  this->setLayout(layout);
  // Wide enough to keep the summary and "currently" lines on a single line
  // each; both wrap rather than clip if a locale needs more room.
  this->setMinimumWidth(460);
  this->setWindowTitle(is_subkey ? tr("Modify Subkey Expiration Date")
                                 : tr("Modify Key Expiration Date"));
  this->setAttribute(Qt::WA_DeleteOnClose);
  this->setModal(true);
}

void KeySetExpireDateDialog::refresh_widgets_state() {
  // The editor is a view; writing to it must not echo back as a user edit.
  expire_date_time_edit_->blockSignals(true);
  expire_date_time_edit_->setDateTime(chosen_date_time_);
  expire_date_time_edit_->blockSignals(false);

  expire_date_time_edit_->setEnabled(!non_expired_);

  const auto valid =
      non_expired_ || chosen_date_time_ > QDateTime::currentDateTime().addSecs(
                                              kMinimumValiditySecs);

  auto palette = summary_label_->palette();
  palette.setColor(summary_label_->foregroundRole(),
                   valid ? Qt::darkGreen : Qt::red);
  summary_label_->setPalette(palette);

  if (non_expired_) {
    summary_label_->setText(tr("Will never expire."));
  } else if (!valid) {
    summary_label_->setText(tr("The expiration date must be in the future."));
  } else {
    summary_label_->setText(tr("Valid for %1 — until %2")
                                .arg(humanize_remaining(chosen_date_time_),
                                     FormatDateTime(chosen_date_time_)));
  }

  button_box_->button(QDialogButtonBox::Ok)->setEnabled(valid);
}

void KeySetExpireDateDialog::slot_validity_period_changed(int index) {
  if (index < 0 || index >= k_validity_periods_.size()) return;
  const auto& period = k_validity_periods_.at(index);

  non_expired_ = period.key == "never";
  // "custom" and "never" leave the picked date untouched on purpose: switching
  // to "Never Expires" and back must give the user their date back.
  if (period.calc_time != nullptr) chosen_date_time_ = period.calc_time();

  refresh_widgets_state();
}

void KeySetExpireDateDialog::slot_expire_date_time_edited(
    const QDateTime& date_time) {
  chosen_date_time_ = date_time;

  // A hand-picked date is by definition no longer one of the presets.
  const auto custom_index = validity_period_combo_box_->findData("custom");
  if (validity_period_combo_box_->currentIndex() != custom_index) {
    QSignalBlocker const blocker(validity_period_combo_box_);
    validity_period_combo_box_->setCurrentIndex(custom_index);
  }

  refresh_widgets_state();
}

void KeySetExpireDateDialog::slot_confirm() {
  std::optional<QDateTime> expires = std::nullopt;
  if (!non_expired_) {
    expires = std::make_optional<QDateTime>(chosen_date_time_.toLocalTime());
  }

  auto err = KeyManagementOperation::GetInstance(current_gpg_context_channel_)
                 .SetExpire(m_key_, m_subkey_, expires);

  if (CheckGpgError(err) == GPG_ERR_NO_ERROR) {
    auto* msg_box = new QMessageBox(qobject_cast<QWidget*>(this->parent()));
    msg_box->setAttribute(Qt::WA_DeleteOnClose);
    msg_box->setStandardButtons(QMessageBox::Ok);
    msg_box->setWindowTitle(tr("Success"));
    msg_box->setText(tr("The expire date of the key pair has been updated."));
    msg_box->setModal(true);
    msg_box->open();

    emit SignalKeyExpireDateUpdated();

    this->close();
  } else {
    QMessageBox::critical(
        this, tr("Failure"),
        tr("Failed to update the expire date of the key pair."));
  }
}

}  // namespace GpgFrontend::UI
