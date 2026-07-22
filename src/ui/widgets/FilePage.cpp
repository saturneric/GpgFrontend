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

#include "ui/widgets/FilePage.h"

#include "core/function/GlobalSettingStation.h"
#include "ui/UISignalStation.h"
#include "ui/UserInterfaceUtils.h"
#include "ui/main_window/MainWindow.h"
#include "ui_FilePage.h"

namespace {

auto VolumeKey(const QStorageInfo& s) -> QString {
  return QCryptographicHash::hash(
             (s.device() + "|" + s.rootPath() + "|" + s.displayName()).toUtf8(),
             QCryptographicHash::Sha1)
      .toHex();
}

auto FormatStorageActionText(const QStorageInfo& storage) -> QString {
  auto name = storage.displayName().trimmed();

  if (name.isEmpty()) {
    name = storage.name().trimmed();
  }

  if (name.isEmpty()) {
    name = storage.rootPath();
  }

  const auto total = storage.bytesTotal();
  const auto available = storage.bytesAvailable();

  if (total > 0 && available >= 0) {
    const auto used = total - available;
    const auto used_percent = static_cast<int>((used * 100) / total);

    return QObject::tr("%1  ·  %2% used  ·  %3")
        .arg(name)
        .arg(used_percent)
        .arg(storage.rootPath());
  }

  return QObject::tr("%1  ·  %2").arg(name, storage.rootPath());
}

}  // namespace

namespace GpgFrontend::UI {

FilePage::FilePage(QWidget* parent, const QString& target_path)
    : QWidget(parent),
      ui_(GpgFrontend::SecureCreateSharedObject<Ui_FilePage>()) {
  ui_->setupUi(this);
  init_ui_style();

  connect(ui_->upPathButton, &QToolButton::clicked, ui_->treeView,
          &FileTreeView::SlotUpLevel);
  connect(ui_->refreshButton, &QToolButton::clicked, this,
          &FilePage::SlotRefreshState);

  ui_->treeView->SetPath(target_path);
  ui_->pathBar->SetPath(ui_->treeView->GetCurrentPath());

  connect(ui_->pathBar, &FilePathBar::SignalPathRequested, this,
          &FilePage::SlotGoPath);

  connect(ui_->filterEdit, &QLineEdit::textChanged, ui_->treeView,
          &FileTreeView::SlotSetNameFilter);
  connect(ui_->treeView, &FileTreeView::SignalItemCountChanged, this,
          &FilePage::update_status_strip);

  // The toolbar keeps only what is used while browsing; everything that is
  // occasional lives one click deeper, in this menu.
  option_popup_menu_ = new QMenu(this);
  // Qt drops action tooltips in menus unless this is asked for explicitly, so
  // every explanation written below would otherwise never be shown.
  option_popup_menu_->setToolTipsVisible(true);

  auto* new_dir_act = new QAction(
      QIcon::fromTheme(QStringLiteral("folder-new")), tr("New Folder"), this);
  new_dir_act->setToolTip(tr("Create a new folder in the current folder."));
  connect(new_dir_act, &QAction::triggered, ui_->treeView,
          &FileTreeView::SlotMkdir);
  option_popup_menu_->addAction(new_dir_act);

  auto* new_file_act =
      new QAction(QIcon::fromTheme(QStringLiteral("document-new")),
                  tr("New Empty File"), this);
  new_file_act->setToolTip(tr("Create an empty file in the current folder."));
  connect(new_file_act, &QAction::triggered, ui_->treeView,
          &FileTreeView::SlotTouch);
  option_popup_menu_->addAction(new_file_act);

  option_popup_menu_->addSeparator();

  harddisk_popup_menu_ = new QMenu(this);
  harddisk_popup_menu_->setToolTipsVisible(true);
  ui_->hardDiskButton->setMenu(harddisk_popup_menu_);

  // The three toggles all answer the same question — what the list shows — so
  // they belong together instead of spread down the menu.
  show_popup_menu_ = new QMenu(tr("Show"), this);
  show_popup_menu_->setToolTipsVisible(true);
  show_popup_menu_->setToolTip(tr("Choose what the file list shows."));
  option_popup_menu_->addMenu(show_popup_menu_);

  auto* show_hidden_act = new QAction(tr("Hidden Files"), this);
  show_hidden_act->setCheckable(true);
  show_hidden_act->setToolTip(
      tr("List files and folders whose name starts with a dot."));
  connect(show_hidden_act, &QAction::triggered, ui_->treeView,
          &FileTreeView::SlotShowHiddenFile);
  show_popup_menu_->addAction(show_hidden_act);

  auto* show_system_act = new QAction(tr("System Files"), this);
  show_system_act->setCheckable(true);
  show_system_act->setToolTip(
      tr("List system files such as devices and sockets."));
  connect(show_system_act, &QAction::triggered, ui_->treeView,
          &FileTreeView::SlotShowSystemFile);
  show_popup_menu_->addAction(show_system_act);

  auto* show_type_column_act = new QAction(tr("Type Column"), this);
  show_type_column_act->setCheckable(true);
  show_type_column_act->setToolTip(tr("Show the file type as its own column."));
  show_type_column_act->setChecked(
      GetSettings().value("file_panel/show_type_column", false).toBool());
  connect(show_type_column_act, &QAction::triggered, this, [this](bool on) {
    ui_->treeView->SetTypeColumnVisible(on);
    GetSettings().setValue("file_panel/show_type_column", on);
  });
  show_popup_menu_->addAction(show_type_column_act);
  ui_->treeView->SetTypeColumnVisible(show_type_column_act->isChecked());

  option_popup_menu_->addSeparator();

  auto* switch_asc_mode_act = new QAction(tr("Use ASCII Armor"), this);
  switch_asc_mode_act->setToolTip(
      tr("Write the result of encrypting or signing as printable text (.asc) "
         "instead of binary."));
  switch_asc_mode_act->setCheckable(true);
  connect(switch_asc_mode_act, &QAction::triggered, this,
          [=](bool on) { ascii_mode_ = on; });
  option_popup_menu_->addAction(switch_asc_mode_act);

  ui_->optionsButton->setMenu(option_popup_menu_);

  ascii_mode_ = !(
      GetSettings().value("gnupg/non_ascii_at_file_operation", true).toBool());
  switch_asc_mode_act->setChecked(ascii_mode_);

  connect(this, &FilePage::SignalRefreshInfoBoard,
          UISignalStation::GetInstance(),
          &UISignalStation::SignalRefreshInfoBoard);
  connect(ui_->treeView, &FileTreeView::SignalPathChanged, this,
          [this](const QString& path) {
            ui_->pathBar->SetPath(path);

            // A filter belongs to the folder it was typed in; carrying it into
            // the next folder would silently hide files there.
            ui_->filterEdit->clear();

            update_status_strip();
          });
  connect(ui_->treeView, &FileTreeView::SignalPathChanged, this,
          &FilePage::SignalPathChanged);
  connect(ui_->treeView, &FileTreeView::SignalOpenFile,
          UISignalStation::GetInstance(),
          &UISignalStation::SignalMainWindowOpenFile);
  connect(ui_->treeView, &FileTreeView::SignalSelectedChanged, this,
          [this](const QStringList& selected_paths) {
            update_main_basic_opera_menu(selected_paths);

            selected_count_ = static_cast<int>(selected_paths.size());
            update_status_strip();

            if (selected_paths.isEmpty()) {
              ui_->batchModeButton->setToolTip(
                  IsBatchMode()
                      ? tr("Batch mode is enabled. No file is selected.")
                      : tr("Enable batch mode to select multiple files."));
              return;
            }

            ui_->batchModeButton->setToolTip(
                tr("%1 item(s) selected.").arg(selected_paths.size()));
          });
  connect(this, &FilePage::SignalCurrentTabChanged, this,
          [this]() { update_main_basic_opera_menu(GetSelected()); });
  connect(this, &FilePage::SignalMainWindowUpdateBasicOperaMenu,
          UISignalStation::GetInstance(),
          &UISignalStation::SignalMainWindowUpdateBasicOperaMenu);
  connect(ui_->batchModeButton, &QToolButton::toggled, ui_->treeView,
          &FileTreeView::SlotSwitchBatchMode);
  connect(
      ui_->batchModeButton, &QToolButton::toggled, this, [this](bool checked) {
        ui_->batchModeButton->setToolTip(
            checked
                ? tr("Batch mode is enabled. Multiple files can be selected.")
                : tr("Enable batch mode to select multiple files."));
      });

  QTimer::singleShot(200, this, &FilePage::update_harddisk_menu_periodic);
}

void FilePage::init_ui_style() {
  setObjectName(QStringLiteral("FilePage"));

  ui_->filterEdit->setPlaceholderText(tr("Filter"));
  ui_->filterEdit->setToolTip(
      tr("List only the files and folders whose name contains this text. The "
         "filter applies to the current folder and is cleared when you open "
         "another one."));
  ui_->filterEdit->setMinimumHeight(30);
  ui_->filterEdit->addAction(QIcon::fromTheme(QStringLiteral("edit-find"),
                                              QIcon(":/icons/search.png")),
                             QLineEdit::LeadingPosition);

  // Auto-raised buttons read as one toolbar; the separators between them carry
  // the grouping, so nothing has to be drawn with a stylesheet.
  const auto setup_tool_button = [](QToolButton* button,
                                    const QString& tooltip) {
    button->setToolTip(tooltip);
    button->setAutoRaise(true);
    button->setMinimumSize(30, 30);
    button->setIconSize(QSize(18, 18));
    button->setToolButtonStyle(Qt::ToolButtonIconOnly);
    button->setFocusPolicy(Qt::NoFocus);
  };

  setup_tool_button(ui_->upPathButton,
                    tr("Go to the parent folder (Backspace)"));
  setup_tool_button(ui_->refreshButton, tr("Read this folder from disk again"));
  setup_tool_button(ui_->hardDiskButton,
                    tr("Go to a mounted volume or removable drive"));
  setup_tool_button(ui_->optionsButton,
                    tr("Create items and choose what the list shows"));
  setup_tool_button(ui_->batchModeButton,
                    tr("Enable batch mode to select multiple files."));

  ui_->hardDiskButton->setPopupMode(QToolButton::InstantPopup);
  ui_->optionsButton->setPopupMode(QToolButton::InstantPopup);
  ui_->batchModeButton->setCheckable(true);

  for (auto* separator : {ui_->navSeparator, ui_->viewSeparator}) {
    separator->setFrameShape(QFrame::VLine);
    separator->setFrameShadow(QFrame::Plain);
    separator->setLineWidth(1);
    // Mid is invisible against the window on several platform palettes; Dark
    // is the first role that reliably reads in both themes.
    separator->setForegroundRole(QPalette::Dark);
    separator->setFixedWidth(1);
    separator->setFixedHeight(18);
  }

  // The strip is secondary information, but it still has to be readable in a
  // dark theme, so it keeps the normal text colour and only steps down in
  // size.
  for (auto* label : {ui_->statusLabel, ui_->capacityLabel}) {
    auto status_font = label->font();
    status_font.setPointSizeF(status_font.pointSizeF() * 0.9);
    label->setFont(status_font);
    label->setForegroundRole(QPalette::WindowText);
  }

  ui_->statusLabel->setToolTip(
      tr("Entries listed in this folder, and how many of them are selected. "
         "Entries hidden by the filter are not counted."));
  ui_->capacityLabel->setToolTip(
      tr("Space still available on the volume holding this folder."));

  ui_->statusSeparator->setFrameShape(QFrame::HLine);
  ui_->statusSeparator->setFrameShadow(QFrame::Plain);
  ui_->statusSeparator->setLineWidth(1);
  ui_->statusSeparator->setForegroundRole(QPalette::Dark);
  ui_->statusSeparator->setFixedHeight(1);
}

auto FilePage::GetSelected() const -> QStringList {
  return ui_->treeView->GetSelectedPaths();
}

void FilePage::SlotGoPath(const QString& path) {
  const auto clean_path = QDir::cleanPath(path);

  QFileInfo info(clean_path);
  if (!info.exists() || !info.isDir() || !info.isReadable()) {
    ui_->pathBar->ShowPathError(
        tr("The folder does not exist or cannot be opened."));
    return;
  }

  ui_->pathBar->SetPath(clean_path);
  ui_->treeView->SlotGoPath(clean_path);
}

void FilePage::SlotRefreshState() {
  update_harddisk_menu();
  SlotGoPath(ui_->treeView->GetCurrentPath());
}

void FilePage::update_main_basic_opera_menu(const QStringList& selected_paths) {
  if (selected_paths.isEmpty()) {
    emit SignalMainWindowUpdateBasicOperaMenu(MainWindow::OperationMenu::kNone);
    return;
  }

  MainWindow::OperationMenu::OperationType operation_type =
      MainWindow::OperationMenu::kNone;

  LOG_D() << "selected path size: " << selected_paths.size();
  LOG_D() << "selected paths: " << selected_paths;

  QContainer<QFileInfo> infos;

  for (const auto& path : selected_paths) {
    infos.append(QFileInfo(path));
  }

  bool c_encr =
      std::all_of(infos.cbegin(), infos.cend(), [](const QFileInfo& info) {
        return (info.isDir() || info.isFile()) && !IsOpenPGPRelatedFile(info);
      });

  if (c_encr) {
    operation_type |= MainWindow::OperationMenu::kEncrypt |
                      MainWindow::OperationMenu::kEncryptAndSign |
                      MainWindow::OperationMenu::kSymmetricEncrypt;
  }

  bool c_decr =
      std::all_of(infos.cbegin(), infos.cend(), [](const QFileInfo& info) {
        return info.isFile() && IsOpenPGPMessageFile(info);
      });

  if (c_decr) {
    operation_type |= MainWindow::OperationMenu::kDecrypt |
                      MainWindow::OperationMenu::kDecryptAndVerify;
  }

  bool c_sign =
      std::all_of(infos.cbegin(), infos.cend(), [](const QFileInfo& info) {
        return info.isFile() && !IsOpenPGPRelatedFile(info);
      });

  if (c_sign) operation_type |= MainWindow::OperationMenu::kSign;

  bool c_verify =
      std::all_of(infos.cbegin(), infos.cend(), [](const QFileInfo& info) {
        return info.isFile() && IsOpenPGPRelatedFile(info);
      });

  if (c_verify) operation_type |= MainWindow::OperationMenu::kVerify;

  emit SignalMainWindowUpdateBasicOperaMenu(operation_type);
}

auto FilePage::IsBatchMode() const -> bool {
  return ui_->batchModeButton->isChecked();
}

auto FilePage::IsASCIIMode() const -> bool { return ascii_mode_; }

[[nodiscard]] auto FilePage::GetCurrentPath() const -> QString {
  return ui_->treeView->GetCurrentPath();
}

auto FilePage::update_harddisk_menu() -> void {
  const auto vols = QStorageInfo::mountedVolumes();

  QSet<QString> keys;
  keys.reserve(vols.size());
  for (const auto& s : vols) {
    if (!s.isValid() || !s.isReady()) continue;
    if (s.rootPath().isEmpty()) continue;

    const auto key = VolumeKey(s);
    keys.insert(key);
  }

  if (keys == last_volume_keys_) return;

  const QSet<QString> added = keys - last_volume_keys_;
  const QSet<QString> removed = last_volume_keys_ - keys;

  if (!added.isEmpty() || !removed.isEmpty()) {
    for (const auto& k : added) LOG_D() << "mounted: " << k;
    for (const auto& k : removed) LOG_D() << "unmounted: " << k;
  }

  last_volume_keys_ = std::move(keys);

  LOG_D() << "updating harddisk menu...";

  if (harddisk_popup_menu_ == nullptr) return;

  harddisk_popup_menu_->clear();

  for (const auto& storage_device : vols) {
    if (!storage_device.isValid() || !storage_device.isReady()) continue;
    if (storage_device.rootPath().isEmpty()) continue;

    LOG_D() << "found storage device: " << storage_device.rootPath() << " "
            << storage_device.displayName() << " " << storage_device.isRoot();

    auto* device_act = new QAction(FormatStorageActionText(storage_device),
                                   harddisk_popup_menu_);
    device_act->setToolTip(storage_device.rootPath());
    device_act->setData(storage_device.rootPath());

    connect(device_act, &QAction::triggered, this, [this, device_act]() {
      SlotGoPath(device_act->data().toString());
    });

    harddisk_popup_menu_->addAction(device_act);
  }

  if (harddisk_popup_menu_->isEmpty()) {
    auto* empty_act =
        harddisk_popup_menu_->addAction(tr("No Available Volumes"));
    empty_act->setEnabled(false);
  }
}

auto FilePage::update_harddisk_menu_periodic() -> void {
  update_harddisk_menu();
  update_status_strip();
  QTimer::singleShot(3000, this, &FilePage::update_harddisk_menu_periodic);
}

void FilePage::update_status_strip() {
  QStringList segments;

  const auto item_count = ui_->treeView->GetVisibleItemCount();
  segments.append(tr("%n item(s)", "", item_count));

  if (selected_count_ > 0) {
    segments.append(tr("%1 selected").arg(selected_count_));
  }

  ui_->statusLabel->setText(segments.join(QStringLiteral("  ·  ")));

  // Capacity belongs to the volume rather than to the folder, so it sits at
  // the far end of the strip instead of in the same run of counters.
  QString capacity;

  const auto current_path = ui_->treeView->GetCurrentPath();
  if (!current_path.isEmpty()) {
    const QStorageInfo storage(current_path);
    if (storage.isValid() && storage.isReady() &&
        storage.bytesAvailable() >= 0) {
      capacity = tr("%1 free").arg(
          QLocale().formattedDataSize(storage.bytesAvailable()));
    }
  }

  ui_->capacityLabel->setText(capacity);
}

}  // namespace GpgFrontend::UI
