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
#include "ui/main_window/MainWindow.h"
#include "ui_FilePage.h"

namespace {

auto VolumeKey(const QStorageInfo& s) -> QString {
  return QCryptographicHash::hash(
             (s.device() + "|" + s.rootPath() + "|" + s.displayName()).toUtf8(),
             QCryptographicHash::Sha1)
      .toHex();
}

auto NormalizeUserPath(QString path, const QString& current_path) -> QString {
  path = path.trimmed();

  if (path.isEmpty()) {
    return current_path;
  }

  if (path == "~") {
    return QDir::homePath();
  }

  if (path.startsWith("~/") || path.startsWith("~\\")) {
    return QDir::homePath() + path.mid(1);
  }

  QFileInfo info(path);
  if (info.isRelative()) {
    return QDir(current_path).absoluteFilePath(path);
  }

  return path;
}

auto ToDisplayUserPath(const QString& path) -> QString {
  auto clean_path = QDir::cleanPath(path);

#ifdef Q_OS_WIN
  clean_path.replace("\\", "/");
#endif

  const auto home_path = QDir::cleanPath(QDir::homePath());

  if (clean_path == home_path) {
    return QStringLiteral("~");
  }

  if (clean_path.startsWith(home_path + "/")) {
    return QStringLiteral("~") + clean_path.mid(home_path.size());
  }

  return clean_path;
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

auto LowerSuffix(const QFileInfo& info) -> QString {
  return info.suffix().toLower();
}

auto IsOpenPGPMessageFile(const QFileInfo& info) -> bool {
  const auto suffix = LowerSuffix(info);
  return suffix == "gpg" || suffix == "pgp" || suffix == "asc";
}

auto IsOpenPGPRelatedFile(const QFileInfo& info) -> bool {
  const auto suffix = LowerSuffix(info);
  return suffix == "gpg" || suffix == "pgp" || suffix == "sig" ||
         suffix == "asc";
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
  connect(this->ui_->newDirButton, &QToolButton::clicked, ui_->treeView,
          &FileTreeView::SlotMkdir);

  ui_->treeView->SetPath(target_path);
  ui_->pathEdit->setText(ui_->treeView->GetCurrentPath());

  path_complete_model_ = new QStringListModel(this);

  path_edit_completer_ = new QCompleter(path_complete_model_, this);
  path_edit_completer_->setCaseSensitivity(Qt::CaseInsensitive);
  path_edit_completer_->setCompletionMode(QCompleter::PopupCompletion);
  path_edit_completer_->setFilterMode(Qt::MatchStartsWith);
  path_edit_completer_->setMaxVisibleItems(12);

  ui_->pathEdit->setCompleter(path_edit_completer_);

  connect(ui_->pathEdit, &QLineEdit::returnPressed, this,
          &FilePage::SlotGoPath);

  connect(ui_->pathEdit, &QLineEdit::textEdited, this,
          [this](const QString& text) { update_path_completion(text); });

  connect(ui_->pathEdit, &QLineEdit::returnPressed, this,
          &FilePage::SlotGoPath);

  connect(path_edit_completer_,
          QOverload<const QString&>::of(&QCompleter::activated), this,
          [this](const QString& path) {
            ui_->pathEdit->setText(ToDisplayUserPath(path));
            ui_->pathEdit->setCursorPosition(ui_->pathEdit->text().size());
          });

  ui_->pathEdit->setCompleter(path_edit_completer_);

  option_popup_menu_ = new QMenu(this);
  auto* show_hidden_act = new QAction(tr("Show Hidden Files"), this);
  show_hidden_act->setCheckable(true);
  connect(show_hidden_act, &QAction::triggered, ui_->treeView,
          &FileTreeView::SlotShowHiddenFile);
  option_popup_menu_->addAction(show_hidden_act);

  auto* show_system_act = new QAction(tr("Show System Files"), this);
  show_system_act->setCheckable(true);
  connect(show_system_act, &QAction::triggered, ui_->treeView,
          &FileTreeView::SlotShowSystemFile);
  option_popup_menu_->addAction(show_system_act);

  auto* switch_asc_mode_act = new QAction(tr("Use ASCII Armor"), this);
  switch_asc_mode_act->setToolTip(
      tr("Use ASCII armored output for file operations."));
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
            this->ui_->pathEdit->setText(ToDisplayUserPath(path));
            this->ui_->pathEdit->setToolTip(path);
          });
  connect(ui_->treeView, &FileTreeView::SignalPathChanged, this,
          &FilePage::SignalPathChanged);
  connect(ui_->treeView, &FileTreeView::SignalOpenFile,
          UISignalStation::GetInstance(),
          &UISignalStation::SignalMainWindowOpenFile);
  connect(ui_->treeView, &FileTreeView::SignalSelectedChanged, this,
          [this](const QStringList& selected_paths) {
            update_main_basic_opera_menu(selected_paths);

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

  ui_->pathEdit->setClearButtonEnabled(true);
  ui_->pathEdit->setPlaceholderText(tr("Type a folder path, e.g. ~/Documents"));
  ui_->pathEdit->setMinimumHeight(30);

  const auto setup_tool_button = [](QToolButton* button,
                                    const QString& tooltip) {
    button->setToolTip(tooltip);
    button->setAutoRaise(false);
    button->setMinimumSize(30, 30);
    button->setIconSize(QSize(18, 18));
    button->setToolButtonStyle(Qt::ToolButtonIconOnly);
    button->setFocusPolicy(Qt::NoFocus);
  };

  setup_tool_button(ui_->upPathButton, tr("Go to Parent Directory"));
  setup_tool_button(ui_->refreshButton, tr("Refresh"));
  setup_tool_button(ui_->newDirButton, tr("Create New Directory"));
  setup_tool_button(ui_->hardDiskButton, tr("Mounted Volumes"));
  setup_tool_button(ui_->optionsButton, tr("File View Options"));
  setup_tool_button(ui_->batchModeButton, tr("Enable Batch Mode"));

  ui_->hardDiskButton->setPopupMode(QToolButton::InstantPopup);
  ui_->optionsButton->setPopupMode(QToolButton::InstantPopup);

  ui_->batchModeButton->setCheckable(true);
  ui_->batchModeButton->setAutoRaise(false);

  ui_->treeView->setAlternatingRowColors(true);
  ui_->treeView->setUniformRowHeights(true);
  ui_->treeView->setAnimated(false);
  ui_->treeView->setSortingEnabled(true);
}

auto FilePage::GetSelected() const -> QStringList {
  return ui_->treeView->GetSelectedPaths();
}

void FilePage::SlotGoPath() {
  const auto path =
      NormalizeUserPath(ui_->pathEdit->text(), ui_->treeView->GetCurrentPath());
  const auto clean_path = QDir::cleanPath(path);

  QFileInfo info(clean_path);
  if (!info.exists() || !info.isDir() || !info.isReadable()) {
    const auto message = tr("The folder does not exist or cannot be opened.");

    ui_->pathEdit->setToolTip(message);
    ui_->pathEdit->selectAll();

    QToolTip::showText(
        ui_->pathEdit->mapToGlobal(QPoint(0, ui_->pathEdit->height())), message,
        ui_->pathEdit);

    return;
  }

  ui_->pathEdit->setToolTip(clean_path);
  ui_->pathEdit->setText(ToDisplayUserPath(clean_path));
  ui_->treeView->SlotGoPath(clean_path);
}

void FilePage::SlotRefreshState() {
  update_harddisk_menu();
  SlotGoPath();
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
        const auto suffix = LowerSuffix(info);
        return info.isFile() && (suffix == "sig" || suffix == "gpg" ||
                                 suffix == "pgp" || suffix == "asc");
      });

  if (c_verify) operation_type |= MainWindow::OperationMenu::kVerify;

  emit SignalMainWindowUpdateBasicOperaMenu(operation_type);
}

auto FilePage::IsBatchMode() const -> bool {
  return ui_->batchModeButton->isChecked();
}

auto FilePage::IsASCIIMode() const -> bool { return ascii_mode_; }

void FilePage::update_path_completion(const QString& input) {
  if (input.trimmed().isEmpty()) {
    path_complete_model_->setStringList({});
    return;
  }

  QString typed_prefix;

  const auto current_path = ui_->treeView->GetCurrentPath();
  const auto normalized_input = NormalizeUserPath(input, current_path);

  QFileInfo normalized_info(normalized_input);

  QDir base_dir;
  if (normalized_info.exists() && normalized_info.isDir()) {
    base_dir = QDir(normalized_info.absoluteFilePath());
    typed_prefix.clear();
  } else {
    base_dir = normalized_info.dir();
    typed_prefix = normalized_info.fileName();
  }

  if (!base_dir.exists() || !base_dir.isReadable()) {
    path_complete_model_->setStringList({});
    return;
  }

  const auto entries =
      base_dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::Readable,
                             QDir::Name | QDir::IgnoreCase | QDir::DirsFirst);

  QStringList candidates;
  candidates.reserve(entries.size());

  for (const auto& entry : entries) {
    const auto file_name = entry.fileName();

    if (!typed_prefix.isEmpty() &&
        !file_name.startsWith(typed_prefix, Qt::CaseInsensitive)) {
      continue;
    }

    auto candidate_path = QDir::cleanPath(entry.absoluteFilePath());

#ifdef Q_OS_WIN
    candidate_path.replace("\\", "/");
#endif

    if (input.trimmed().startsWith("~")) {
      candidates.append(ToDisplayUserPath(candidate_path));
    } else if (QFileInfo(input).isRelative() && !input.startsWith("/") &&
               !input.startsWith("\\")) {
      candidates.append(QDir(current_path).relativeFilePath(candidate_path));
    } else {
      candidates.append(candidate_path);
    }
  }

  path_complete_model_->setStringList(candidates);

  if (!candidates.isEmpty()) {
    path_edit_completer_->setCompletionPrefix(input);
    path_edit_completer_->complete();
  }
}

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

  if (harddisk_popup_menu_ != nullptr) {
    harddisk_popup_menu_->deleteLater();
    harddisk_popup_menu_ = nullptr;
  }

  harddisk_popup_menu_ = new QMenu(this);

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
      const auto path = device_act->data().toString();
      ui_->pathEdit->setText(ToDisplayUserPath(path));
      SlotGoPath();
    });

    harddisk_popup_menu_->addAction(device_act);
  }

  if (harddisk_popup_menu_->isEmpty()) {
    auto* empty_act =
        harddisk_popup_menu_->addAction(tr("No Available Volumes"));
    empty_act->setEnabled(false);
  }

  ui_->hardDiskButton->setMenu(harddisk_popup_menu_);
}

auto FilePage::update_harddisk_menu_periodic() -> void {
  update_harddisk_menu();
  QTimer::singleShot(3000, this, &FilePage::update_harddisk_menu_periodic);
}

}  // namespace GpgFrontend::UI
