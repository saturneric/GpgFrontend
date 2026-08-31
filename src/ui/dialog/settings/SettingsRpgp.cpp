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

#include "SettingsRpgp.h"

#include "core/function/GlobalSettingStation.h"
#include "core/utils/RustUtils.h"
#include "ui_RpgpSettings.h"

namespace GpgFrontend::UI {

RpgpTab::RpgpTab(QWidget* parent)
    : QWidget(parent),
      ui_(GpgFrontend::SecureCreateSharedObject<Ui_RpgpSettings>()) {
  ui_->setupUi(this);

  ui_->passwordCacheGroupBox->setTitle(tr("Password Cache"));

  // rPGP passphrase cache timeouts, presented in minutes (stored in seconds).
  ui_->rpgpCacheTtlLabel->setText(tr("Password Cache TTL (minutes):"));
  ui_->rpgpCacheTtlSpinBox->setRange(1, 1440);  // 1 minute .. 24 hours
  ui_->rpgpCacheTtlSpinBox->setSuffix(tr(" min"));
  ui_->rpgpCacheTtlSpinBox->setToolTip(
      tr("Idle time the rPGP engine keeps an entered passphrase cached. The "
         "window is renewed each time the passphrase is used."));

  ui_->rpgpCacheMaxTtlLabel->setText(tr("Password Cache Max TTL (minutes):"));
  ui_->rpgpCacheMaxTtlSpinBox->setRange(1, 10080);  // 1 minute .. 7 days
  ui_->rpgpCacheMaxTtlSpinBox->setSuffix(tr(" min"));
  ui_->rpgpCacheMaxTtlSpinBox->setToolTip(
      tr("Absolute lifetime of a cached passphrase, measured from when it was "
         "first entered, regardless of use. Never shorter than the TTL."));

  ui_->passwordCacheTipsLabel->setText(
      tr("These options only apply to the rPGP engine's in-memory passphrase "
         "cache."));

  ui_->passphrasePromptGroupBox->setTitle(tr("Passphrase Prompt"));

  // How long the prompt waits for an answer before it gives up. 0 disables the
  // countdown; the spin box shows "Never" for it.
  ui_->promptTimeoutLabel->setText(tr("Passphrase Prompt Timeout (seconds):"));
  ui_->promptTimeoutSpinBox->setRange(0, 3600);  // never .. 1 hour
  ui_->promptTimeoutSpinBox->setSuffix(tr(" s"));
  ui_->promptTimeoutSpinBox->setSpecialValueText(tr("Never"));
  ui_->promptTimeoutSpinBox->setToolTip(
      tr("Time the passphrase prompt waits for an answer before it closes "
         "itself and cancels the operation. Set to 0 to let it wait "
         "indefinitely."));

  ui_->passphrasePromptTipsLabel->setText(
      tr("Applies to the passphrase prompt GpgFrontend shows for the rPGP "
         "engine. GnuPG keys are unlocked through pinentry, which has its own "
         "timeout."));

  ui_->symmetricEncryptionGroupBox->setTitle(tr("Passphrase Encryption"));

  // The Argon2id parameters written into a passphrase-encrypted message.
  // RFC 9106 section 4 names exactly these two choices; the display text is
  // translated, the data token is the stable value stored in the settings.
  ui_->argon2ProfileLabel->setText(tr("Argon2 Parameters:"));
  ui_->argon2ProfileComboBox->addItem(tr("High memory, 2 GiB"),
                                      QString(kRpgpArgon2ProfileHighMemory));
  ui_->argon2ProfileComboBox->addItem(tr("Low memory, 64 MiB"),
                                      QString(kRpgpArgon2ProfileLowMemory));
  ui_->argon2ProfileComboBox->setToolTip(
      tr("How much work it takes to turn the passphrase into an encryption "
         "key. More memory makes a stolen message far more expensive to "
         "crack."));

  ui_->symmetricEncryptionTipsLabel->setText(
      tr("Decrypting a message needs as much memory as encrypting it, on "
         "every machine that opens it. Choose the low-memory option if this "
         "computer or the recipient's cannot spare 2 GiB. This applies only "
         "to messages encrypted from now on. "));

  SetSettings();
}

void RpgpTab::SetSettings() {
  auto settings = GetSettings();

  // Stored in seconds; presented in minutes (rounded up so a sub-minute value
  // never collapses to 0, which would disable the cache).
  auto cache_ttl_secs =
      settings.value("engine/password_cache_ttl", 600).toLongLong();
  auto cache_max_ttl_secs =
      settings.value("engine/password_cache_max_ttl", 7200).toLongLong();
  ui_->rpgpCacheTtlSpinBox->setValue(
      static_cast<int>(qMax<qint64>(1, (cache_ttl_secs + 59) / 60)));
  ui_->rpgpCacheMaxTtlSpinBox->setValue(
      static_cast<int>(qMax<qint64>(1, (cache_max_ttl_secs + 59) / 60)));

  ui_->promptTimeoutSpinBox->setValue(
      settings.value("engine/passphrase_prompt_timeout", 60).toInt());

  // An unknown token (an older or newer profile name) falls back to the first
  // entry, which is the high-memory default.
  auto argon2_profile = settings
                            .value("engine/argon2_s2k_profile",
                                   QString(kRpgpArgon2ProfileHighMemory))
                            .toString();
  auto argon2_index = ui_->argon2ProfileComboBox->findData(argon2_profile);
  ui_->argon2ProfileComboBox->setCurrentIndex(argon2_index != -1 ? argon2_index
                                                                 : 0);
}

void RpgpTab::ApplySettings() {
  auto settings = GpgFrontend::GetSettings();

  // Convert the minute-based UI values back to seconds. The max cap can never
  // be shorter than the sliding window; clamp here so the stored values are
  // sane (the engine clamps again defensively).
  qint64 cache_ttl_secs =
      static_cast<qint64>(ui_->rpgpCacheTtlSpinBox->value()) * 60;
  qint64 cache_max_ttl_secs = qMax<qint64>(
      cache_ttl_secs,
      static_cast<qint64>(ui_->rpgpCacheMaxTtlSpinBox->value()) * 60);
  settings.setValue("engine/password_cache_ttl",
                    static_cast<qulonglong>(cache_ttl_secs));
  settings.setValue("engine/password_cache_max_ttl",
                    static_cast<qulonglong>(cache_max_ttl_secs));

  // Apply immediately so the new timeouts take effect without a restart.
  SetRpgpPasswordCacheTtl(static_cast<uint64_t>(cache_ttl_secs),
                          static_cast<uint64_t>(cache_max_ttl_secs));

  // Read fresh by PassphraseService on every request, so the next prompt
  // already uses this value.
  settings.setValue("engine/passphrase_prompt_timeout",
                    ui_->promptTimeoutSpinBox->value());

  auto argon2_profile = ui_->argon2ProfileComboBox->currentData().toString();
  settings.setValue("engine/argon2_s2k_profile", argon2_profile);

  // Apply immediately so the next passphrase encryption already uses it.
  SetRpgpArgon2S2kParams(RpgpArgon2ParamsOfProfile(argon2_profile));
}

}  // namespace GpgFrontend::UI
