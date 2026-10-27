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

#include "ui/widgets/ExpiryPicker.h"

#include "core/utils/CommonUtils.h"
#include "ui/function/KeyGenProfile.h"

namespace GpgFrontend::UI {

namespace {

const auto kCustom = u"custom"_s;

}  // namespace

ExpiryPicker::ExpiryPicker(QWidget* parent)
    : QWidget(parent), combo_(new QComboBox(this)), date_(new QDateTimeEdit(this)) {
  for (const auto& preset : KeyGenValidityPresets()) {
    combo_->addItem(KeyGenValidityDisplayName(preset), preset);
  }
  combo_->addItem(tr("Custom date"), kCustom);

  date_->setCalendarPopup(true);
  date_->setDisplayFormat(QLocale().dateFormat(QLocale::ShortFormat));
  // A key that expires before anyone can use it is a mistake, not a choice.
  date_->setMinimumDateTime(CurrentDateTime().addDays(1));

  auto* layout = new QHBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);
  layout->addWidget(combo_, 1);
  layout->addWidget(date_, 1);

  SetValidity(u"2y"_s);

  connect(combo_, &QComboBox::currentIndexChanged, this, [this]() {
    if (combo_->currentData().toString() == kCustom) {
      // Start the date where the list was, so switching to it changes nothing
      // until the date is changed.
      const QSignalBlocker blocker(date_);
      if (date_->dateTime() <= CurrentDateTime()) {
        date_->setDateTime(CurrentDateTime().addYears(2));
      }
    }
    sync_date_visibility();
    emit SignalChanged();
  });
  connect(date_, &QDateTimeEdit::dateTimeChanged, this,
          [this]() { emit SignalChanged(); });
}

void ExpiryPicker::SetExpiry(bool non_expired, const QDateTime& expire) {
  SetValidity(KeyGenValidityFor(non_expired, expire, CurrentDateTime()));
  if (!non_expired && combo_->currentData().toString() == kCustom) {
    const QSignalBlocker blocker(date_);
    date_->setDateTime(expire);
  }
}

void ExpiryPicker::SetValidity(const QString& validity) {
  const QSignalBlocker combo_blocker(combo_);
  const QSignalBlocker date_blocker(date_);

  const auto key = IsNeverExpiringValidity(validity) ? u"forever"_s : validity;
  const auto index = combo_->findData(key);
  if (index >= 0) {
    combo_->setCurrentIndex(index);
  } else {
    combo_->setCurrentIndex(combo_->findData(kCustom));
    date_->setDateTime(ParseKeyGenValidity(validity, CurrentDateTime()));
  }
  sync_date_visibility();
}

auto ExpiryPicker::NonExpired() const -> bool {
  return IsNeverExpiringValidity(combo_->currentData().toString());
}

auto ExpiryPicker::Expire() const -> QDateTime {
  const auto key = combo_->currentData().toString();
  if (key == kCustom) return date_->dateTime();
  return ParseKeyGenValidity(key, CurrentDateTime());
}

auto ExpiryPicker::Validity() const -> QString {
  const auto key = combo_->currentData().toString();
  if (key != kCustom) return key;
  return KeyGenValidityFor(false, date_->dateTime(), CurrentDateTime());
}

void ExpiryPicker::sync_date_visibility() {
  date_->setVisible(combo_->currentData().toString() == kCustom);
}

}  // namespace GpgFrontend::UI
