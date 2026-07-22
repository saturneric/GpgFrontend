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

#include "SettingsAdvanced.h"

#include <QStandardItemModel>

#include "core/GFCoreLog.h"
#include "core/function/AppSecureKeyManager.h"
#include "core/function/GFBufferFactory.h"
#include "core/function/GlobalSettingStation.h"
#include "core/function/SystemSecretStore.h"
#include "ui/UserInterfaceUtils.h"
#include "ui/dialog/AppKeyPinDialog.h"

namespace GpgFrontend::UI {

namespace {

/// The memory-hardening level at which weekly key rotation is on.
constexpr int kRotationSecureLevel = 3;

/// Ring buffer bounds. The floor keeps a crash report useful; the ceiling keeps
/// the buffer from dominating memory, since every entry is held in RAM.
constexpr int kMinRingCapacity = 128;
constexpr int kMaxRingCapacity = 65536;

/**
 * @brief Make a tooltip word-wrap instead of rendering as one long line.
 *
 * Qt only wraps a tooltip it recognises as rich text, so the text is placed in
 * a paragraph. The markup is added here rather than inside tr() so translators
 * never have to carry it, and the text is escaped in case a translation
 * contains characters that would otherwise be read as markup.
 *
 * @param text plain tooltip text
 * @return rich-text tooltip that wraps
 */
auto WrappingToolTip(const QString& text) -> QString {
  return QStringLiteral("<p>%1</p>").arg(text.toHtmlEscaped());
}

/**
 * @brief Whether dropping to @p chosen_level warrants a data-loss warning.
 *
 * Only a move out of the rotation tier is destructive: objects written while
 * weekly key rotation was on are stamped with a rotated key that the lower
 * levels never load, so they orphan and are eventually garbage-collected.
 * Moving between two non-rotation levels, staying put, or raising the level is
 * always safe.
 *
 * @param applied_level secure level the running process actually loaded
 * @param chosen_level level the user just picked
 * @return true when the pick leaves the rotation tier and should be confirmed
 */
auto ShouldWarnRotationDowngrade(int applied_level, int chosen_level) -> bool {
  return applied_level >= kRotationSecureLevel &&
         chosen_level < kRotationSecureLevel;
}

}  // namespace

AdvancedTab::AdvancedTab(QWidget* parent) : QWidget(parent) {
  env_locked_keys_ = qApp->property("GFEnvLockedKeys").toStringList();

  auto* security_box = new QGroupBox(tr("Security"), this);
  auto* security_form = new QFormLayout(security_box);

  // Levels mirror what the core actually does: 1 wipes freed memory, 2 adds
  // locked pages, 3 additionally rotates the data-object keys every week. This
  // is memory and key hygiene only — how the key file is protected at rest is
  // the separate control below.
  secure_level_combo_ = new QComboBox(security_box);
  // Tier names are shared with the status readout via SecureLevelDisplayName;
  // pair each with a brief note on what it adds, since the tier word alone does
  // not say what the level actually does. The notes are cumulative — each level
  // keeps everything the ones below it do.
  const auto level_hint = [](int level) -> QString {
    switch (level) {
      case 1:
        return tr("wipe freed memory");
      case 2:
        return tr("also lock memory pages");
      case 3:
        return tr("also rotate keys weekly");
      default:
        return tr("no extra hardening");
    }
  };
  for (int level = 0; level <= 3; ++level) {
    secure_level_combo_->addItem(
        QStringLiteral("%1 (%2)").arg(SecureLevelDisplayName(level),
                                      level_hint(level)),
        level);
  }
  secure_level_combo_->setToolTip(WrappingToolTip(
      tr("How aggressively the application protects your secrets. "
         "Higher levels cost some performance.")));
  security_form->addRow(tr("Secure Level:"), secure_level_combo_);

  // One at-rest protection, chosen from three mutually exclusive backends, with
  // an explicit default. This replaces the old keychain checkbox and the
  // never-selectable level-3 PIN item.
  protection_combo_ = new QComboBox(security_box);
  protection_combo_->addItem(tr("No extra protection (default)"),
                             AppKeyProtectionToString(AppKeyProtection::kNONE));
  protection_combo_->addItem(
      tr("System keychain"),
      AppKeyProtectionToString(AppKeyProtection::kKEYCHAIN));
  protection_combo_->addItem(tr("PIN at startup"),
                             AppKeyProtectionToString(AppKeyProtection::kPIN));
  protection_combo_->setToolTip(WrappingToolTip(
      tr("How the application key file is protected on disk. The system "
         "keychain keeps a secret on this computer and never asks you for a "
         "password; a PIN is asked for each time the application starts.")));

  change_pin_button_ = new QPushButton(tr("Change PIN…"), security_box);
  auto* protection_row = new QHBoxLayout();
  protection_row->setContentsMargins(0, 0, 0, 0);
  protection_row->addWidget(protection_combo_, 1);
  protection_row->addWidget(change_pin_button_);
  security_form->addRow(tr("Application Key Protection:"), protection_row);

  protection_advice_label_ = new QLabel(
      tr("Weekly key rotation offers little protection while the application "
         "key itself is stored unprotected on disk. Consider using the system "
         "keychain or a PIN."),
      security_box);
  protection_advice_label_->setWordWrap(true);
  protection_advice_label_->setVisible(false);
  security_form->addRow(protection_advice_label_);

  self_check_box_ = new QCheckBox(
      tr("Verify signed libraries and binaries at startup"), security_box);
  self_check_box_->setToolTip(WrappingToolTip(
      tr("Check that the shipped libraries and executables still match the "
         "signatures made at build time. The application refuses to start if "
         "the check fails.")));
  security_form->addRow(self_check_box_);

  auto* diagnostics_box = new QGroupBox(tr("Diagnostics"), this);
  auto* diagnostics_form = new QFormLayout(diagnostics_box);

  log_level_combo_ = new QComboBox(diagnostics_box);
  log_level_combo_->addItem(tr("Debug"), static_cast<int>(GFLogLevel::kDEBUG));
  log_level_combo_->addItem(tr("Info"), static_cast<int>(GFLogLevel::kINFO));
  log_level_combo_->addItem(tr("Warning"),
                            static_cast<int>(GFLogLevel::kWARNING));
  log_level_combo_->addItem(tr("Error"),
                            static_cast<int>(GFLogLevel::kCRITICAL));
  log_level_combo_->addItem(tr("Fatal"), static_cast<int>(GFLogLevel::kFATAL));
  log_level_combo_->setToolTip(WrappingToolTip(
      tr("The least severe message that still gets written to the log. Debug "
         "is the most detailed and writes the most to disk.")));
  diagnostics_form->addRow(tr("Log Level:"), log_level_combo_);

  ring_capacity_spin_ = new QSpinBox(diagnostics_box);
  ring_capacity_spin_->setRange(kMinRingCapacity, kMaxRingCapacity);
  ring_capacity_spin_->setSuffix(tr(" entries"));
  ring_capacity_spin_->setToolTip(WrappingToolTip(
      tr("How many recent log messages are kept in memory for crash reports "
         "and the log viewer. Larger values use more memory.")));
  diagnostics_form->addRow(tr("Log Ring Buffer:"), ring_capacity_spin_);

  auto* restart_note = new QLabel(
      tr("These settings are read once while the application starts, so a "
         "change only takes effect after a restart."),
      this);
  restart_note->setWordWrap(true);

  env_notice_label_ = new QLabel(
      tr("Some settings on this page are fixed by the ENV.ini file next to the "
         "application and cannot be changed here. Edit that file to change "
         "them."),
      this);
  env_notice_label_->setWordWrap(true);
  env_notice_label_->setVisible(false);

  auto* layout = new QVBoxLayout(this);
  layout->addWidget(security_box);
  layout->addWidget(diagnostics_box);
  layout->addWidget(restart_note);
  layout->addWidget(env_notice_label_);
  layout->addStretch(1);
  setLayout(layout);

  SetSettings();

  // Only announce a restart when a value actually moved, so opening and closing
  // the dialog untouched never nags the user.
  const auto declare_restart = [this]() { emit SignalDeepRestartNeeded(); };
  connect(secure_level_combo_, &QComboBox::currentIndexChanged, this,
          declare_restart);
  connect(secure_level_combo_, &QComboBox::currentIndexChanged, this,
          [this]() { refresh_protection_advice(); });
  connect(self_check_box_, &QCheckBox::toggled, this, declare_restart);
  connect(protection_combo_, &QComboBox::currentIndexChanged, this,
          declare_restart);
  // The advice tracks the pending selection, but the Change PIN button does
  // not: it re-keys the key file as it is on disk right now, so it stays tied
  // to the applied protection (set in SetSettings and after a successful
  // transition). Enabling it just because "PIN" is selected but not yet applied
  // would offer to change a PIN that does not exist, and the current-PIN check
  // could never pass.
  connect(protection_combo_, &QComboBox::currentIndexChanged, this,
          [this]() { refresh_protection_advice(); });

  // Probe the store the moment the user picks the keychain, rather than letting
  // them restart only to find out it never worked. This is the first point at
  // which a probe, and any unlock prompt it triggers, is something they asked
  // for — opening the dialog must never do it on its own.
  connect(protection_combo_, &QComboBox::activated, this, [this](int index) {
    const auto chosen = AppKeyProtectionFromString(
        protection_combo_->itemData(index).toString());
    if (chosen != AppKeyProtection::kKEYCHAIN) return;

    auto* store = GetSystemSecretStore();
    if (store != nullptr && store->IsAvailable()) return;

    QMessageBox::warning(
        this, tr("System Keychain Unavailable"),
        tr("The system credential store could not be used, so the "
           "application key cannot be protected with it.") +
            "\n" +
            tr("On Linux this needs a running secret service, such as "
               "GNOME Keyring or KWallet, and it must be unlocked."));

    const auto none = protection_combo_->findData(
        AppKeyProtectionToString(AppKeyProtection::kNONE));
    if (none != -1) protection_combo_->setCurrentIndex(none);
  });

  // Confirm before leaving the rotation tier: below it the weekly rotated keys
  // are never loaded, so everything saved while rotation was on orphans and is
  // eventually garbage-collected. This mirrors the keychain probe above — it
  // reacts to an explicit user pick (activated), never to SetSettings()
  // populating the combo, and reverts on decline.
  connect(secure_level_combo_, &QComboBox::activated, this, [this](int index) {
    const auto chosen = secure_level_combo_->itemData(index).toInt();
    const auto applied = qApp->property("GFSecureLevel").toInt();
    if (!ShouldWarnRotationDowngrade(applied, chosen)) return;

    const auto answer = QMessageBox::warning(
        this, tr("Turn Off Weekly Key Rotation?"),
        tr("At the %1 level the application saves your data with a key that "
           "changes every week. Choosing a lower level stops loading those "
           "keys, so anything saved while this level was on can no longer be "
           "read and is deleted after a short grace period.")
                .arg(SecureLevelDisplayName(kRotationSecureLevel)) +
            +"\n\n" + tr("Lower the level anyway?"),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (answer == QMessageBox::Yes) return;

    const auto revert = secure_level_combo_->findData(applied);
    if (revert != -1) {
      QSignalBlocker block(secure_level_combo_);
      secure_level_combo_->setCurrentIndex(revert);
    }
    refresh_protection_advice();
  });

  connect(change_pin_button_, &QPushButton::clicked, this,
          [this]() { change_pin(); });
  connect(log_level_combo_, &QComboBox::currentIndexChanged, this,
          declare_restart);
  connect(ring_capacity_spin_, &QSpinBox::valueChanged, this, declare_restart);
}

auto AdvancedTab::lock_if_pinned(QWidget* widget, const QString& user_key)
    -> bool {
  if (!env_locked_keys_.contains(user_key)) return false;

  widget->setEnabled(false);
  widget->setToolTip(
      WrappingToolTip(tr("Fixed by ENV.ini and cannot be changed here.")));
  env_notice_label_->setVisible(true);
  return true;
}

void AdvancedTab::SetSettings() {
  // Read the resolved values off qApp rather than the settings store: those are
  // what the running process actually used, ENV.ini overrides included.
  const auto secure_level = qApp->property("GFSecureLevel").toInt();
  const auto log_level = qApp->property("GFLogLevel").toInt();
  const auto ring_capacity = qApp->property("GFLogRingBufferCapacity").toInt();

  const auto select = [](QComboBox* combo, int value) {
    const auto index = combo->findData(value);
    if (index != -1) combo->setCurrentIndex(index);
  };

  const auto protection = qApp->property("GFAppKeyProtection").toString();
  const auto protection_index = protection_combo_->findData(protection);
  if (protection_index != -1) {
    protection_combo_->setCurrentIndex(protection_index);
  }

  select(secure_level_combo_, secure_level);
  select(log_level_combo_, log_level);
  self_check_box_->setChecked(qApp->property("GFSelfCheck").toBool());
  ring_capacity_spin_->setValue(qBound(kMinRingCapacity,
                                       ring_capacity > 0 ? ring_capacity : 1024,
                                       kMaxRingCapacity));

  change_pin_button_->setEnabled(AppKeyProtectionFromString(protection) ==
                                 AppKeyProtection::kPIN);

  configure_protection_items();
  refresh_protection_advice();

  lock_if_pinned(secure_level_combo_, "advanced/secure_level");
  lock_if_pinned(self_check_box_, "advanced/self_check");
  lock_if_pinned(log_level_combo_, "advanced/log_level");
  lock_if_pinned(ring_capacity_spin_, "advanced/log_ring_buffer_capacity");

  if (lock_if_pinned(protection_combo_, "advanced/app_key_protection")) {
    change_pin_button_->setEnabled(false);
  }
}

void AdvancedTab::configure_protection_items() {
  // Enable or disable individual items rather than the whole combo, so the
  // reason a mode is unavailable is attached to that mode. Portable mode allows
  // only "none" and "pin"; a keychain secret cannot follow the directory to
  // another computer.
  auto* model = qobject_cast<QStandardItemModel*>(protection_combo_->model());
  if (model == nullptr) return;

  const auto set_item = [&](AppKeyProtection mode, bool enabled,
                            const QString& reason) {
    const auto index =
        protection_combo_->findData(AppKeyProtectionToString(mode));
    if (index == -1) return;
    auto* item = model->item(index);
    item->setEnabled(enabled);
    item->setToolTip(enabled ? QString() : WrappingToolTip(reason));
  };

  if (GetGSS().IsProtableMode()) {
    set_item(AppKeyProtection::kKEYCHAIN, false,
             tr("Not available in portable mode: a portable installation must "
                "not depend on secrets stored on one particular computer."));
  } else if (GetSystemSecretStore() == nullptr) {
    // A null check only, never IsAvailable(): probing writes to the store and
    // can raise an unlock prompt, which merely opening this dialog must not do.
    // A store that exists but is locked is caught when the user selects it.
    set_item(AppKeyProtection::kKEYCHAIN, false,
             tr("No system credential store is available on this computer. On "
                "Linux this needs a running secret service, such as GNOME "
                "Keyring or KWallet."));
  } else {
    set_item(AppKeyProtection::kKEYCHAIN, true, {});
  }
}

void AdvancedTab::refresh_protection_advice() {
  const auto level = secure_level_combo_->currentData().toInt();
  const auto protection =
      AppKeyProtectionFromString(protection_combo_->currentData().toString());
  protection_advice_label_->setVisible(level >= kRotationSecureLevel &&
                                       protection == AppKeyProtection::kNONE);
}

void AdvancedTab::ApplySettings() {
  auto settings = GetSettings();

  // Never write back a value ENV.ini owns: storing it would make the user
  // setting disagree with what actually takes effect, and the disagreement
  // would only surface later, if the ENV.ini key were ever removed.
  const auto store = [this, &settings](const QString& user_key,
                                       const QVariant& value) {
    if (env_locked_keys_.contains(user_key)) return;
    settings.setValue(user_key, value);
  };

  store("advanced/secure_level", secure_level_combo_->currentData().toInt());
  store("advanced/self_check", self_check_box_->isChecked());
  store("advanced/log_level", log_level_combo_->currentData().toInt());
  store("advanced/log_ring_buffer_capacity", ring_capacity_spin_->value());

  // Record the protection only once the key file actually carries it, so the
  // stored setting can never disagree with the file on disk.
  if (apply_app_key_protection()) {
    store("advanced/app_key_protection",
          protection_combo_->currentData().toString());
  }
}

auto AdvancedTab::apply_app_key_protection() -> bool {
  const auto to =
      AppKeyProtectionFromString(protection_combo_->currentData().toString());
  const auto from = AppKeyProtectionFromApp();
  if (to == from) return true;

  auto& mgr = AppSecureKeyManager::GetInstance();

  const auto revert = [this, from]() {
    const auto index =
        protection_combo_->findData(AppKeyProtectionToString(from));
    if (index != -1) {
      QSignalBlocker block(protection_combo_);
      protection_combo_->setCurrentIndex(index);
    }
    change_pin_button_->setEnabled(from == AppKeyProtection::kPIN);
    refresh_protection_advice();
  };

  GFBuffer pin;
  if (to == AppKeyProtection::kPIN) {
    AppKeyPinDialog dialog(AppKeyPinDialog::Mode::kSET, this);
    if (dialog.exec() != QDialog::Accepted) {
      revert();
      return false;
    }
    pin = dialog.Pin();
  }

  const auto result = AppSecureKeyManager::ChangeProtection(
      mgr.GetLegacyKeyPath(), GetSystemSecretStore(), mgr.GetLegacyKey(), from,
      to, pin);

  if (!result.Ok()) {
    if (result.status == AppKeyProtectionStatus::kSTORE_UNAVAILABLE) {
      QMessageBox::warning(
          this, tr("System Keychain Unavailable"),
          tr("The application key could not be protected with the system "
             "keychain, so it has been left exactly as it was."));
    } else {
      QMessageBox::critical(
          this, tr("Application Key Error"),
          tr("The application key could not be re-protected: %1.")
                  .arg(result.detail) +
              "\n" +
              tr("It has been left exactly as it was, so nothing has been "
                 "lost."));
    }
    revert();
    return false;
  }

  // Keep the resolved property honest: SetSettings() and the dialog's revert
  // path both read it, and it is what the file on disk now actually says.
  qApp->setProperty("GFAppKeyProtection", AppKeyProtectionToString(to));
  change_pin_button_->setEnabled(to == AppKeyProtection::kPIN);
  return true;
}

void AdvancedTab::change_pin() {
  auto& mgr = AppSecureKeyManager::GetInstance();

  AppKeyPinDialog dialog(AppKeyPinDialog::Mode::kCHANGE, this);
  while (dialog.exec() == QDialog::Accepted) {
    // Verify the current PIN before re-keying, so someone at an unlocked
    // machine cannot silently change it.
    auto on_disk = GFBufferFactory::FromFile(mgr.GetLegacyKeyPath());
    if (!on_disk ||
        !AppSecureKeyManager::UnsealKey(dialog.CurrentPin(), {}, *on_disk)) {
      dialog.SetErrorText(tr("The current PIN is not correct."));
      dialog.Clear();
      continue;
    }

    const auto result = AppSecureKeyManager::ChangeProtection(
        mgr.GetLegacyKeyPath(), GetSystemSecretStore(), mgr.GetLegacyKey(),
        AppKeyProtection::kPIN, AppKeyProtection::kPIN, dialog.Pin());

    if (result.Ok()) {
      QMessageBox::information(this, tr("PIN Changed"),
                               tr("The application PIN has been changed."));
      return;
    }

    QMessageBox::critical(
        this, tr("Application Key Error"),
        tr("The PIN could not be changed: %1.").arg(result.detail) + "\n" +
            tr("It has been left exactly as it was, so nothing has been "
               "lost."));
    return;
  }
}

}  // namespace GpgFrontend::UI
