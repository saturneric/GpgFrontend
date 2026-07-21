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

#include "core/GFCoreLog.h"
#include "core/function/GlobalSettingStation.h"
#include "core/function/SystemSecretStore.h"

namespace GpgFrontend::UI {

namespace {

constexpr int kMaxGuiSecureLevel = 2;

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

}  // namespace

AdvancedTab::AdvancedTab(QWidget* parent) : QWidget(parent) {
  env_locked_keys_ = qApp->property("GFEnvLockedKeys").toStringList();

  auto* security_box = new QGroupBox(tr("Security"), this);
  auto* security_form = new QFormLayout(security_box);

  // Levels mirror what the core actually does: 1 wipes freed memory, 2 adds
  // locked pages and the strict data-object GC policy. Level 3 (PIN-protected
  // secure key) is deliberately not offered here — see the note below.
  secure_level_combo_ = new QComboBox(security_box);
  secure_level_combo_->addItem(tr("Standard"), 0);
  secure_level_combo_->addItem(tr("Wipe freed memory"), 1);
  secure_level_combo_->addItem(tr("Wipe and lock memory pages"), 2);
  secure_level_combo_->setToolTip(WrappingToolTip(
      tr("How aggressively the application protects secrets held in memory. "
         "Higher levels cost some performance.")));
  security_form->addRow(tr("Secure Level:"), secure_level_combo_);

  self_check_box_ = new QCheckBox(
      tr("Verify signed libraries and binaries at startup"), security_box);
  self_check_box_->setToolTip(WrappingToolTip(
      tr("Check that the shipped libraries and executables still match the "
         "signatures made at build time. The application refuses to start if "
         "the check fails.")));
  security_form->addRow(self_check_box_);

  os_secret_store_box_ = new QCheckBox(
      tr("Protect the application key with the system keychain"), security_box);
  os_secret_store_box_->setToolTip(WrappingToolTip(
      tr("Keep a random secret in your system's credential store and use it to "
         "encrypt the application key file, so the key is not left readable on "
         "disk. You are never asked for a password. The key can then only be "
         "opened on this computer, by this user account.")));
  security_form->addRow(os_secret_store_box_);

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
  connect(self_check_box_, &QCheckBox::toggled, this, declare_restart);
  connect(os_secret_store_box_, &QCheckBox::toggled, this, declare_restart);

  // Verify the store the moment the user opts in, rather than letting them
  // restart only to find out it never worked. This is the first point at which
  // a probe, and any unlock prompt it triggers, is something they asked for.
  connect(os_secret_store_box_, &QCheckBox::toggled, this,
          [this](bool checked) -> void {
            if (!checked) return;

            auto* store = GetSystemSecretStore();
            if (store != nullptr && store->IsAvailable()) return;

            QMessageBox::warning(
                this, tr("System Keychain Unavailable"),
                tr("The system credential store could not be used, so the "
                   "application key cannot be protected with it.") +
                    "\n" +
                    tr("On Linux this needs a running secret service, such as "
                       "GNOME Keyring or KWallet, and it must be unlocked."));

            os_secret_store_box_->setChecked(false);
          });
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

  // Level 3 demands a PIN and encrypts the app secure key with it. Selecting it
  // here would strand an already-unencrypted key: the next start prompts for a
  // PIN, fails to decrypt, and refuses to launch. It stays ENV.ini-only, and is
  // added to the combo purely so a machine already running at that level shows
  // its real value instead of silently reading back as something lower.
  if (secure_level > kMaxGuiSecureLevel &&
      secure_level_combo_->findData(secure_level) == -1) {
    secure_level_combo_->addItem(tr("High (PIN required at startup)"),
                                 secure_level);
  }

  const auto select = [](QComboBox* combo, int value) {
    const auto index = combo->findData(value);
    if (index != -1) combo->setCurrentIndex(index);
  };

  select(secure_level_combo_, secure_level);
  select(log_level_combo_, log_level);
  self_check_box_->setChecked(qApp->property("GFSelfCheck").toBool());
  ring_capacity_spin_->setValue(qBound(kMinRingCapacity,
                                       ring_capacity > 0 ? ring_capacity : 1024,
                                       kMaxRingCapacity));

  os_secret_store_box_->setChecked(qApp->property("GFOSSecretStore").toBool());

  // Give the single most specific reason the control is unavailable, rather
  // than a generic one that leaves the user guessing which condition applies.
  auto* store = GetSystemSecretStore();
  if (secure_level > kMaxGuiSecureLevel) {
    os_secret_store_box_->setEnabled(false);
    os_secret_store_box_->setToolTip(WrappingToolTip(
        tr("Not needed at this secure level: the application key is already "
           "encrypted with the PIN you enter at startup.")));
  } else if (GetGSS().IsProtableMode()) {
    os_secret_store_box_->setEnabled(false);
    os_secret_store_box_->setToolTip(WrappingToolTip(
        tr("Not available in portable mode: a portable installation must not "
           "depend on secrets stored on one particular computer.")));
  } else if (store == nullptr) {
    // Deliberately only a null check, never IsAvailable(): probing writes to
    // the credential store, which can raise an unlock prompt. Merely opening
    // this dialog must not do that, so a store that exists but is locked or
    // broken is caught when the user actually opts in, below.
    os_secret_store_box_->setEnabled(false);
    os_secret_store_box_->setToolTip(WrappingToolTip(
        tr("No system credential store is available on this computer. On "
           "Linux this needs a running secret service, such as GNOME Keyring "
           "or KWallet.")));
  } else {
    lock_if_pinned(os_secret_store_box_, "advanced/os_secret_store");
  }

  lock_if_pinned(secure_level_combo_, "advanced/secure_level");
  lock_if_pinned(self_check_box_, "advanced/self_check");
  lock_if_pinned(log_level_combo_, "advanced/log_level");
  lock_if_pinned(ring_capacity_spin_, "advanced/log_ring_buffer_capacity");
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
  store("advanced/os_secret_store", os_secret_store_box_->isChecked());
  store("advanced/log_level", log_level_combo_->currentData().toInt());
  store("advanced/log_ring_buffer_capacity", ring_capacity_spin_->value());
}

}  // namespace GpgFrontend::UI
