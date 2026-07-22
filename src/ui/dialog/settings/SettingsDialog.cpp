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

namespace {

/// Marks a row as a section header rather than a page.
constexpr int kSectionHeaderRole = Qt::UserRole + 1;

/// Widest the sidebar may get before rows start eliding, in pixels. Long
/// translated titles must not be able to push the whole dialog wide.
constexpr int kNavListMaxWidth = 220;

}  // namespace

SettingsDialog::SettingsDialog(QWidget* parent)
    : GeneralDialog(typeid(SettingsDialog).name(), parent) {
  // A flat tab bar stopped scaling once the dialog passed a handful of pages:
  // the ones that did not fit were only reachable through the bar's scroll
  // buttons. A sidebar shows every destination at once, groups related pages,
  // and leaves room to grow.
  nav_list_ = new QListWidget(this);
  nav_list_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  nav_list_->setTextElideMode(Qt::ElideRight);
  nav_list_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);

  page_stack_ = new QStackedWidget(this);

  general_tab_ = new GeneralTab();
  appearance_tab_ = new AppearanceTab();
  network_tab_ = new NetworkTab();
  key_dbs_tab_ = new KeyDatabasesTab();
  gnupg_tab_ = new GnuPGTab();
  rpgp_tab_ = new RpgpTab();
  im_tab_ = new InstantMessagingTab();
  advanced_tab_ = new AdvancedTab();

  // Searching by page name alone only helps someone who already knows how the
  // pages are cut up. The keywords name the settings themselves, so typing what
  // you are looking for ("proxy", "pin") lands on the page that holds it.
  const auto application_section = tr("Application");
  const auto engines_section = tr("Keys & Engines");
  const auto features_section = tr("Features");
  const auto system_section = tr("System");

  // Named so the restart confirmation can tell the user which pages hold a
  // change that needs one.
  const auto general_title = tr("General");
  const auto appearance_title = tr("Appearance");
  const auto key_dbs_title = tr("Key Databases");
  const auto gnupg_title = tr("GnuPG");
  const auto advanced_title = tr("Advanced");

  add_page(general_tab_, general_title, application_section,
           {tr("startup"), tr("confirm import"), tr("language"), tr("locale"),
            tr("translation"), tr("data"), tr("cache")});
  add_page(appearance_tab_, appearance_title, application_section,
           {tr("theme"), tr("icon"), tr("font size"), tr("toolbar"),
            tr("text editor"), tr("status panel")});

  // network settings is not available in sandbox environment, so only add the
  // page when not running in sandbox
  if (!IsRunningInSandBox()) {
    add_page(network_tab_, tr("Network"), application_section,
             {tr("proxy"), tr("socks"), tr("http"), tr("timeout"),
              tr("connection")});
  }

  add_page(key_dbs_tab_, key_dbs_title, engines_section,
           {tr("keyring"), tr("gpg home"), tr("database path")});

  if (GetGSS().IsEngineSupported(OpenPGPEngine::kGNUPG)) {
    add_page(
        gnupg_tab_, gnupg_title, engines_section,
        {tr("gpgme"), tr("gpgconf"), tr("binary path"), tr("custom install")});
  }

  if (GetGSS().IsEngineSupported(OpenPGPEngine::kRPGP)) {
    add_page(rpgp_tab_, tr("rPGP"), engines_section,
             {tr("rust"), tr("engine")});
  }

  add_page(im_tab_, tr("Instant Messaging"), features_section,
           {tr("message book"), tr("phrase"), tr("fingerprint"), tr("token")});
  add_page(advanced_tab_, advanced_title, system_section,
           {tr("security level"), tr("PIN"), tr("keychain"), tr("log level"),
            tr("ring buffer"), tr("integrity check"), tr("ENV.ini")});

  // Sized once every row exists, so the sidebar is exactly as wide as its
  // widest title — up to a cap, past which titles elide rather than widen the
  // dialog.
  nav_list_->setFixedWidth(
      std::min(nav_list_->sizeHintForColumn(0) + (2 * nav_list_->frameWidth()) +
                   nav_list_->verticalScrollBar()->sizeHint().width(),
               kNavListMaxWidth));

  search_edit_ = new QLineEdit(this);
  search_edit_->setClearButtonEnabled(true);
  search_edit_->setPlaceholderText(tr("Search settings…"));

  // The selected tab used to say which page you were on; without a tab bar the
  // heading carries that.
  page_title_label_ = new QLabel(this);
  auto title_font = page_title_label_->font();
  title_font.setBold(true);
  page_title_label_->setFont(title_font);

  auto* title_separator = new QFrame(this);
  title_separator->setFrameShape(QFrame::HLine);
  title_separator->setFrameShadow(QFrame::Sunken);

  auto* page_layout = new QVBoxLayout();
  page_layout->setContentsMargins(0, 0, 0, 0);
  page_layout->addWidget(page_title_label_);
  page_layout->addWidget(title_separator);
  page_layout->addWidget(page_stack_);

  auto* content_layout = new QHBoxLayout();
  content_layout->setContentsMargins(0, 0, 0, 0);
  content_layout->addWidget(nav_list_);
  content_layout->addLayout(page_layout, 1);

  auto* main_layout = new QVBoxLayout();
  main_layout->addWidget(search_edit_);
  main_layout->addLayout(content_layout, 1);

  connect(nav_list_, &QListWidget::currentRowChanged, this, [this](int row) {
    if (row < 0) return;
    const auto index = nav_list_->item(row)->data(Qt::UserRole).toInt();
    page_stack_->setCurrentIndex(index);
    page_title_label_->setText(pages_.at(index).title);
  });

  connect(search_edit_, &QLineEdit::textChanged, this,
          &SettingsDialog::filter_pages);
  search_edit_->installEventFilter(this);

  select_first_visible_page();

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

auto SettingsDialog::eventFilter(QObject* watched, QEvent* event) -> bool {
  if (watched == search_edit_ && event->type() == QEvent::KeyPress) {
    auto* key_event = static_cast<QKeyEvent*>(event);
    switch (key_event->key()) {
      case Qt::Key_Return:
      case Qt::Key_Enter:
        // Swallowed: QLineEdit would ignore it and the dialog's default button
        // would fire, saving and closing on someone who was only searching.
        // Moving into the list is what pressing Enter on a search means here.
        nav_list_->setFocus();
        return true;
      case Qt::Key_Up:
      case Qt::Key_Down:
        // Let the list do the stepping, so it skips the section headers, while
        // the text stays where the user left it.
        QCoreApplication::sendEvent(nav_list_, event);
        return true;
      default:
        break;
    }
  }
  return GeneralDialog::eventFilter(watched, event);
}

void SettingsDialog::add_page(QWidget* page, const QString& title,
                              const QString& section,
                              const QStringList& keywords) {
  // Emit the section header only when its first available page arrives: with
  // pages conditional on the sandbox and on engine support, a section can end
  // up with nothing under it, and an empty header would be noise.
  const auto section_present = std::any_of(
      pages_.cbegin(), pages_.cend(),
      [&section](const SettingsPage& p) { return p.section == section; });
  if (!section_present) {
    auto* header = new QListWidgetItem(section, nav_list_);
    header->setData(kSectionHeaderRole, true);
    // No flags at all: the row is neither selectable nor focusable, so both the
    // mouse and the arrow keys step straight over it.
    header->setFlags(Qt::NoItemFlags);
    auto header_font = header->font();
    header_font.setBold(true);
    header->setFont(header_font);
  }

  // Wrap each page in a scroll area so its content never dictates a minimum
  // dialog width; the user can shrink the window and the page scrolls instead.
  auto* area = new QScrollArea(page_stack_);
  area->setWidgetResizable(true);
  area->setFrameShape(QFrame::NoFrame);
  area->setWidget(page);

  const auto index = page_stack_->addWidget(area);
  pages_.append(SettingsPage{page, title, section, keywords});

  auto* item = new QListWidgetItem(title, nav_list_);
  item->setData(Qt::UserRole, index);
  item->setToolTip(title);
  nav_row_of_page_.insert(page, nav_list_->row(item));
}

void SettingsDialog::filter_pages(const QString& text) {
  const auto needle = text.trimmed();

  // Walking backwards lets a header be hidden as soon as we know none of the
  // rows below it survived — the rows of a section always follow its header.
  auto section_has_match = false;
  for (auto row = nav_list_->count() - 1; row >= 0; --row) {
    auto* item = nav_list_->item(row);

    if (item->data(kSectionHeaderRole).toBool()) {
      item->setHidden(!needle.isEmpty() && !section_has_match);
      section_has_match = false;
      continue;
    }

    const auto& page = pages_.at(item->data(Qt::UserRole).toInt());
    const auto matches =
        needle.isEmpty() || page.title.contains(needle, Qt::CaseInsensitive) ||
        std::any_of(page.keywords.cbegin(), page.keywords.cend(),
                    [&needle](const QString& keyword) {
                      return keyword.contains(needle, Qt::CaseInsensitive);
                    });

    item->setHidden(!matches);
    if (matches) section_has_match = true;
  }

  // Only the list is filtered — every page stays alive in the stack, so edits
  // made on a page that is now hidden are still applied on OK. But the stack
  // must not keep showing a page the user can no longer see a row for.
  auto* current = nav_list_->currentItem();
  if (current == nullptr || current->isHidden()) select_first_visible_page();
}

void SettingsDialog::select_first_visible_page() {
  for (auto row = 0; row < nav_list_->count(); ++row) {
    auto* item = nav_list_->item(row);
    if (item->isHidden() || item->data(kSectionHeaderRole).toBool()) continue;
    nav_list_->setCurrentRow(row);
    return;
  }
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

void SettingsDialog::SelectPageFor(QWidget* page) {
  const auto it = nav_row_of_page_.constFind(page);
  if (it == nav_row_of_page_.constEnd()) return;

  // A leftover filter could be hiding exactly the row we are about to select,
  // so drop it first. Clearing runs filter_pages(), which unhides everything.
  search_edit_->clear();
  nav_list_->setCurrentRow(*it);
}

}  // namespace GpgFrontend::UI
