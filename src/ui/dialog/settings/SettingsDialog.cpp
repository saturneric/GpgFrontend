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

#include "SettingsDialog.h"

#include "core/GFConstants.h"
#include "core/function/GlobalSettingStation.h"
#include "core/utils/CommonUtils.h"
#include "ui/dialog/settings/SettingsAdvanced.h"
#include "ui/dialog/settings/SettingsAppearance.h"
#include "ui/dialog/settings/SettingsGeneral.h"
#include "ui/dialog/settings/SettingsGnuPG.h"
#include "ui/dialog/settings/SettingsIM.h"
#include "ui/dialog/settings/SettingsKeyDatabases.h"
#include "ui/dialog/settings/SettingsNetwork.h"
#include "ui/dialog/settings/SettingsRpgp.h"
#include "ui/main_window/MainWindow.h"

namespace GpgFrontend::UI {

SettingsDialog::SettingsDialog(QWidget* parent)
    : GeneralDialog(typeid(SettingsDialog).name(), parent) {
  tab_widget_ = new QTabWidget();
  // Keep the tab bar from forcing the dialog wide, while always showing each
  // tab's full name: never elide the labels, and when they don't all fit show
  // scroll buttons instead — so the width stays freely adjustable.
  tab_widget_->setUsesScrollButtons(true);
  tab_widget_->setElideMode(Qt::ElideNone);

  general_tab_ = new GeneralTab();
  appearance_tab_ = new AppearanceTab();
  network_tab_ = new NetworkTab();
  key_dbs_tab_ = new KeyDatabasesTab();
  gnupg_tab_ = new GnuPGTab();
  rpgp_tab_ = new RpgpTab();
  im_tab_ = new InstantMessagingTab();
  advanced_tab_ = new AdvancedTab();

  auto* main_layout = new QVBoxLayout();
  main_layout->addWidget(tab_widget_);
  main_layout->stretch(0);

  // Wrap each page in a scroll area so a tab's content never dictates a minimum
  // dialog width; the user can shrink the window and the page scrolls instead.
  const auto add_tab = [this](QWidget* page, const QString& title) {
    auto* area = new QScrollArea(tab_widget_);
    area->setWidgetResizable(true);
    area->setFrameShape(QFrame::NoFrame);
    area->setWidget(page);
    tab_widget_->addTab(area, title);
  };

  // Named so the restart confirmation can tell the user which pages hold a
  // change that needs one.
  const auto general_title = tr("General");
  const auto appearance_title = tr("Appearance");
  const auto key_dbs_title = tr("Key Databases");
  const auto gnupg_title = tr("GnuPG");
  const auto advanced_title = tr("Advanced");

  add_tab(general_tab_, general_title);
  add_tab(appearance_tab_, appearance_title);

  // network settings is not available in sandbox environment, so only add the
  // tab when not running in sandbox
  if (!IsRunningInSandBox()) {
    add_tab(network_tab_, tr("Network"));
  }

  add_tab(key_dbs_tab_, key_dbs_title);

  if (GetGSS().IsEngineSupported(OpenPGPEngine::kGNUPG)) {
    add_tab(gnupg_tab_, gnupg_title);
  }

  if (GetGSS().IsEngineSupported(OpenPGPEngine::kRPGP)) {
    add_tab(rpgp_tab_, tr("rPGP"));
  }

  add_tab(im_tab_, tr("Instant Messaging"));
  add_tab(advanced_tab_, advanced_title);

#ifdef Q_OS_MACOS
  connect(this, &QDialog::finished, this, &SettingsDialog::SlotAccept);
  setWindowTitle(tr("Preference"));
#else
  button_box_ =
      new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  connect(button_box_, &QDialogButtonBox::accepted, this,
          &SettingsDialog::SlotAccept);
  connect(button_box_, &QDialogButtonBox::rejected, this,
          &SettingsDialog::reject);
  main_layout->addWidget(button_box_);
  main_layout->stretch(0);
  setWindowTitle(tr("Settings"));
#endif

  setLayout(main_layout);

  // Each tab announces a needed restart as the user edits, not when settings
  // are applied — so by the time OK is pressed we already know whether to ask
  // for confirmation, and which pages to name when we do.
  const auto declare = [this](int mode, const QString& page) {
    return [this, mode, page]() { declare_restart(mode, page); };
  };

  // restart ui
  connect(general_tab_, &GeneralTab::SignalRestartNeeded, this,
          declare(kRestartCode, general_title));

  // restart core and ui
  connect(general_tab_, &GeneralTab::SignalDeepRestartNeeded, this,
          declare(kDeepRestartCode, general_title));

  // restart core and ui
  connect(appearance_tab_, &AppearanceTab::SignalRestartNeeded, this,
          declare(kRestartCode, appearance_title));

  connect(key_dbs_tab_, &KeyDatabasesTab::SignalDeepRestartNeeded, this,
          declare(kDeepRestartCode, key_dbs_title));

  connect(gnupg_tab_, &GnuPGTab::SignalDeepRestartNeeded, this,
          declare(kDeepRestartCode, gnupg_title));

  // the advanced knobs are only read while the process starts, so applying
  // them means relaunching — a deep restart, which does exactly that
  connect(advanced_tab_, &AdvancedTab::SignalDeepRestartNeeded, this,
          declare(kDeepRestartCode, advanced_title));

  // announce main window
  connect(this, &SettingsDialog::SignalRestartNeeded,
          qobject_cast<MainWindow*>(parent), &MainWindow::SlotSetRestartNeeded);

  this->show();
  this->raise();
  this->activateWindow();
}

void SettingsDialog::showEvent(QShowEvent* event) {
  GeneralDialog::showEvent(event);

  // If the window state has not been restored, move the dialog to the center of
  // the parent window (if has parent) or the screen.
  if (!isRectRestored()) {
    movePosition2CenterOfParent();
  }
}

void SettingsDialog::declare_restart(int mode, const QString& page) {
  // Reverting reloads every tab, which fires the very change signals that got
  // us here. Without this guard a cancelled restart would immediately re-arm
  // itself and the user would be asked again on the next OK.
  if (suppress_restart_declaration_) return;

  // Keep the deepest request: a shallow UI reload must not mask a pending core
  // restart declared by another page.
  restart_mode_ = std::max(restart_mode_, mode);
  if (!restart_pages_.contains(page)) restart_pages_ << page;
}

auto SettingsDialog::confirm_restart() -> bool {
  const auto deep = restart_mode_ >= kDeepRestartCode;

  QMessageBox box(this);
  box.setIcon(QMessageBox::Question);
  box.setWindowTitle(tr("Restart Required"));
  box.setText(deep ? tr("Some of your changes only take effect after "
                        "GpgFrontend restarts.")
                   : tr("Some of your changes only take effect after the "
                        "interface reloads."));
  box.setInformativeText(
      tr("Changes needing this were made on: %1.\n\nChoose Cancel to discard "
         "everything you changed in this dialog and keep the current settings.")
          .arg(restart_pages_.join(QStringLiteral(", "))));

  auto* save_button =
      box.addButton(deep ? tr("Save and Restart") : tr("Save and Reload"),
                    QMessageBox::AcceptRole);
  box.addButton(QMessageBox::Cancel);
  box.setDefaultButton(save_button);
  box.exec();

  return box.clickedButton() == save_button;
}

void SettingsDialog::revert_all_tabs() {
  suppress_restart_declaration_ = true;

  // Every tab reloads its controls straight from the settings store, so
  // whatever the user typed is dropped and nothing was ever written.
  general_tab_->SetSettings();
  appearance_tab_->SetSettings();
  network_tab_->SetSettings();
  key_dbs_tab_->SetSettings();
  gnupg_tab_->SetSettings();
  rpgp_tab_->SetSettings();
  im_tab_->SetSettings();
  advanced_tab_->SetSettings();

  restart_mode_ = kNonRestartCode;
  restart_pages_.clear();

  suppress_restart_declaration_ = false;
}

void SettingsDialog::SlotAccept() {
  // Ask before anything is written. Tabs declare their restart while the user
  // edits, so the answer is already known here — which is what makes a clean
  // "cancel discards everything" possible.
  if (restart_mode_ != kNonRestartCode && !confirm_restart()) {
    revert_all_tabs();
#ifndef Q_OS_MACOS
    // Stay open on the reverted values so the user can adjust. On macOS this
    // runs from QDialog::finished, where the dialog is already going away and
    // there is nothing left to hold open — falling through simply rewrites the
    // unchanged originals and skips the restart.
    return;
#endif
  }

  general_tab_->ApplySettings();
  appearance_tab_->ApplySettings();
  network_tab_->ApplySettings();
  key_dbs_tab_->ApplySettings();

  // The GnuPG page declares a deep restart when edited but its ApplySettings()
  // was never wired up, so those changes were dropped on OK and the restart
  // applied nothing. Guarded like the tab itself, which is only shown when the
  // engine is supported.
  if (GetGSS().IsEngineSupported(OpenPGPEngine::kGNUPG)) {
    gnupg_tab_->ApplySettings();
  }

  if (GetGSS().IsEngineSupported(OpenPGPEngine::kRPGP)) {
    rpgp_tab_->ApplySettings();
  }

  im_tab_->ApplySettings();
  advanced_tab_->ApplySettings();

  emit SignalAppearanceChanged();

  LOG_D() << "flushing qt event loop to ensure all pending events are "
             "processed before applying the settings";
  QCoreApplication::sendPostedEvents(nullptr, 0);
  QCoreApplication::processEvents(QEventLoop::AllEvents);

  if (restart_mode_ != kNonRestartCode) {
    emit SignalRestartNeeded(restart_mode_);
  }

  close();
}

auto SettingsDialog::ListLanguages() -> QHash<QString, QString> {
  QHash<QString, QString> languages;
  languages.insert(QString(), tr("System Default"));

  auto filenames = QDir(QLatin1String(":/i18n")).entryList();
  for (const auto& file : filenames) {
    auto start = file.indexOf('.') + 1;
    auto end = file.lastIndexOf('.');
    if (start < 0 || end < 0 || start >= end) continue;

    auto locale = file.mid(start, end - start);
    QLocale const q_locale(locale);

#if QT_VERSION >= QT_VERSION_CHECK(6, 2, 0)
    if (q_locale.nativeTerritoryName().isEmpty()) continue;
#else
    if (q_locale.nativeCountryName().isEmpty()) continue;
#endif

    auto language = q_locale.nativeLanguageName() + " (" + locale + ")";
    languages.insert(q_locale.name(), language);
  }
  return languages;
}

}  // namespace GpgFrontend::UI
