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

#include "TextEditTabWidget.h"

#include "core/function/CacheManager.h"
#include "core/function/GFBufferFactory.h"
#include "core/function/GlobalSettingStation.h"
#include "core/model/CacheObject.h"
#include "core/model/GFBuffer.h"
#include "core/module/ModuleManager.h"
#include "core/utils/CommonUtils.h"
#include "ui/UIModuleManager.h"
#include "ui/UISignalStation.h"
#include "ui/widgets/FilePage.h"
#include "ui/widgets/PlainTextEditorPage.h"

namespace GpgFrontend::UI {

namespace {

auto NormalizedExistingFilePath(const QString& path) -> QString {
  QFileInfo info(path);
  auto canonical = info.canonicalFilePath();
  if (!canonical.isEmpty()) return QDir::cleanPath(canonical);
  return QDir::cleanPath(info.absoluteFilePath());
}

class ScopedOverrideCursor {
 public:
  explicit ScopedOverrideCursor(const QCursor& cursor) {
    QApplication::setOverrideCursor(cursor);
  }

  ~ScopedOverrideCursor() { QApplication::restoreOverrideCursor(); }
};

}  // namespace

TextEditTabWidget::TextEditTabWidget(QWidget* parent) : QTabWidget(parent) {
  InitTabStyle();

  tabBar()->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(
      tabBar(), &QTabBar::customContextMenuRequested, this,
      [this](const QPoint& pos) {
        const int index = tabBar()->tabAt(pos);
        if (index < 0) return;

        QMenu menu(this);
        auto* close_act = menu.addAction(tr("Close"));
        auto* copy_path_act = menu.addAction(tr("Copy Path"));
        auto* reveal_act = menu.addAction(tr("Reveal in File Browser"));
        auto* text_page = qobject_cast<PlainTextEditorPage*>(widget(index));
        auto* file_page = qobject_cast<FilePage*>(widget(index));

        const auto tab_path = PathForTab(index);
        copy_path_act->setEnabled(!tab_path.isEmpty());
        reveal_act->setEnabled(!tab_path.isEmpty());

        const bool has_path =
            (text_page != nullptr && !text_page->GetFilePath().isEmpty()) ||
            (file_page != nullptr && !file_page->GetCurrentPath().isEmpty());

        copy_path_act->setEnabled(has_path);

        const auto* selected = menu.exec(tabBar()->mapToGlobal(pos));
        if (selected == close_act) {
          emit tabCloseRequested(index);
        } else if (selected == copy_path_act) {
          const auto tab_path = PathForTab(index);
          if (!tab_path.isEmpty()) {
            QGuiApplication::clipboard()->setText(tab_path);
          }
        } else if (selected == reveal_act) {
          const auto tab_path = PathForTab(index);
          if (!tab_path.isEmpty()) {
            const QFileInfo info(tab_path);
            const auto reveal_path =
                info.isFile() ? info.absolutePath() : info.absoluteFilePath();
            QDesktopServices::openUrl(QUrl::fromLocalFile(reveal_path));
          }
        }
      });
}

void TextEditTabWidget::InitTabStyle() {
  setAcceptDrops(true);
  setMovable(true);
  setTabsClosable(true);
  setDocumentMode(true);
  setUsesScrollButtons(true);
  setElideMode(Qt::ElideRight);
  tabBar()->setExpanding(false);
  tabBar()->setDrawBase(false);
  tabBar()->setMovable(true);

  tabBar()->setProperty("gfTextEditTabBar", true);
  tabBar()->style()->unpolish(tabBar());
  tabBar()->style()->polish(tabBar());

  setStyleSheet(R"(
QTabWidget::pane {
  border-top: 1px solid palette(mid);
}

QTabBar[gfTextEditTabBar="true"]::tab {
  padding: 5px 12px;
  margin-right: 0px;
  border-right: 1px solid palette(mid);
  color: palette(window-text);
  background: transparent;
}

QTabBar[gfTextEditTabBar="true"]::tab:first {
  border-left: 1px solid palette(mid);
}

QTabBar[gfTextEditTabBar="true"]::tab:selected {
  background: palette(base);
  color: palette(window-text);
  font-weight: 600;
}

QTabBar[gfTextEditTabBar="true"]::tab:hover:!selected {
  background: palette(alternate-base);
}

QTabBar[gfTextEditTabBar="true"]::tab:disabled {
  color: palette(mid);
}
)");

  style()->unpolish(this);
  style()->polish(this);
}

auto TextEditTabWidget::PathForTab(int index) const -> QString {
  if (index < 0 || index >= count()) return {};

  if (auto* page = qobject_cast<PlainTextEditorPage*>(widget(index))) {
    return page->GetFilePath();
  }

  if (auto* page = qobject_cast<FilePage*>(widget(index))) {
    return page->GetCurrentPath();
  }

  return {};
}

auto TextEditTabWidget::CanOpenAsTextFile(const QFileInfo& file_info,
                                          QString* error_message) const
    -> bool {
  if (!file_info.exists() || !file_info.isFile()) {
    if (error_message != nullptr) {
      *error_message = tr("The file does not exist.");
    }
    return false;
  }

  constexpr qint64 kMaxTextEditorFileSize = 1024 * 1024;
  if (file_info.size() > kMaxTextEditorFileSize) {
    if (error_message != nullptr) {
      *error_message = tr("The file \"%1\" is larger than 1 MB and will not be "
                          "opened in the text editor.")
                           .arg(file_info.fileName());
    }
    return false;
  }

  QFile file(file_info.absoluteFilePath());
  if (!file.open(QIODevice::ReadOnly)) {
    if (error_message != nullptr) {
      *error_message =
          tr("The file \"%1\" could not be opened.").arg(file_info.fileName());
    }
    return false;
  }

  const QByteArray sample = file.read(4096);
  if (sample.contains('\0')) {
    if (error_message != nullptr) {
      *error_message = tr("The file \"%1\" appears to be a binary file and "
                          "will not be opened.")
                           .arg(file_info.fileName());
    }
    return false;
  }

  return true;
}

auto TextEditTabWidget::NormalizeTabTitle(QString title) -> QString {
  title = title.trimmed();

  while (title.startsWith("*")) {
    title.remove(0, 1);
    title = title.trimmed();
  }

  return title;
}

void TextEditTabWidget::dragEnterEvent(QDragEnterEvent* event) {
  if (!event->mimeData()->hasUrls()) {
    event->ignore();
    return;
  }

  const auto urls = event->mimeData()->urls();
  const bool has_local_file =
      std::any_of(urls.cbegin(), urls.cend(),
                  [](const QUrl& url) { return url.isLocalFile(); });

  if (!has_local_file) {
    event->ignore();
    return;
  }

  event->acceptProposedAction();
}

void TextEditTabWidget::dropEvent(QDropEvent* event) {
  if (!event->mimeData()->hasUrls()) {
    event->ignore();
    return;
  }

  for (const auto& url : event->mimeData()->urls()) {
    OpenDroppedUrl(url);
  }

  event->acceptProposedAction();
}

void TextEditTabWidget::SlotOpenFile(const QString& path) {
  QFileInfo file_info(path);
  if (!file_info.exists() || !file_info.isFile()) {
    QMessageBox::warning(
        this, tr("File Open Error"),
        tr("The file \"%1\" does not exist.").arg(file_info.fileName()));
    return;
  }

  auto event_id = FileExtensionEventId(file_info.suffix(), "OPEN_FILE");
  if (!event_id.isEmpty() && Module::IsEventListening(event_id)) {
    Module::TriggerEvent(event_id, {{"file_path", GFBuffer{path}}}, {});
    return;
  }

  const int existing_index = FindTabByFilePath(path);
  if (existing_index >= 0) {
    setCurrentIndex(existing_index);
    if (auto* page =
            qobject_cast<PlainTextEditorPage*>(widget(existing_index))) {
      page->GetTextPage()->setFocus();
    }
    return;
  }

  QString error_message;
  if (!CanOpenAsTextFile(file_info, &error_message)) {
    QMessageBox::warning(this, tr("File Open Error"), error_message);
    return;
  }

  ScopedOverrideCursor wait_cursor(Qt::WaitCursor);
  auto* page =
      CreatePlainTextTab(stripped_name(path), path, QIcon(":/icons/file.png"));
  page->ReadFile();
}

auto TextEditTabWidget::FindTabByFilePath(const QString& path) const -> int {
  const auto target_path = NormalizedExistingFilePath(path);

  for (int i = 0; i < count(); ++i) {
    auto* page = qobject_cast<PlainTextEditorPage*>(widget(i));
    if (page == nullptr) continue;

    const auto page_path = page->GetFilePath();
    if (page_path.isEmpty()) continue;

    if (NormalizedExistingFilePath(page_path) == target_path) {
      return i;
    }
  }

  return -1;
}

void TextEditTabWidget::SlotShowModified(bool changed) {
  auto* page = CurTextPage();
  if (page == nullptr) return;

  UpdateTabModifiedMark(page, changed);
}

auto TextEditTabWidget::CurTextPage() const -> PlainTextEditorPage* {
  return qobject_cast<PlainTextEditorPage*>(this->currentWidget());
}

auto TextEditTabWidget::CurPageTextEdit() const -> PlainTextEditorPage* {
  auto* cur_page = qobject_cast<PlainTextEditorPage*>(this->currentWidget());
  return cur_page;
}

auto TextEditTabWidget::CurFilePage() const -> FilePage* {
  auto* cur_file_page = qobject_cast<FilePage*>(this->currentWidget());
  return cur_file_page;
}

auto TextEditTabWidget::stripped_name(const QString& full_file_name)
    -> QString {
  return QFileInfo(full_file_name).fileName();
}

void TextEditTabWidget::slot_save_status_to_cache_for_recovery() {
  if (this->text_page_data_modified_count_++ % 8 != 0) return;

  bool restore_text_editor_page =
      GetSettings().value("basic/restore_text_editor_page", false).toBool();
  if (!restore_text_editor_page) return;

  SlotCacheTextEditors();
}

auto TextEditTabWidget::SlotNewPlainTextTab() -> QWidget* {
  const auto header = generate_new_title("untitled", "txt");
  return CreatePlainTextTab(header, {}, QIcon(":/icons/file.png"));
}

auto TextEditTabWidget::SlotNewTab(const QString& type, const QString& title,
                                   const QIcon& icon) -> QWidget* {
  auto* page = CreatePlainTextTab(title, {}, icon);
  page->setProperty("type", type);
  return page;
}

void TextEditTabWidget::SlotNewTabWithGFBuffer(QString title,
                                               const GFBuffer& buffer) {
  QString header = NormalizeTabTitle(title);
  if (header.isEmpty()) {
    header = generate_new_title("untitled", "txt");
  }

  auto* page = CreatePlainTextTab(header, {}, QIcon(":/icons/file.png"));
  page->GetTextPage()->document()->setPlainText(buffer.ConvertToQString());
  page->GetTextPage()->document()->setModified(true);
  UpdateTabModifiedMark(page, true);
  page->GetTextPage()->setFocus();
}

void TextEditTabWidget::SlotNewTabWithContent(QString title,
                                              const QString& content) {
  auto header = NormalizeTabTitle(title);
  if (header.isEmpty()) {
    header = generate_new_title("untitled", "txt");
  }

  auto* page = CreatePlainTextTab(header, {}, QIcon(":/icons/file.png"));
  page->GetTextPage()->document()->setPlainText(content);
  page->GetTextPage()->document()->setModified(true);
  UpdateTabModifiedMark(page, true);
}

void TextEditTabWidget::SlotOpenDefaultPath() {
  auto home_path_as_file_panel_default_path =
      GetSettings()
          .value("basic/home_path_as_file_panel_default_path", true)
          .toBool();

  auto default_path = home_path_as_file_panel_default_path
                          ? QDir::homePath()
                          : QDir::currentPath();

  if (IsRunningInSandBox()) {
    default_path = QFileDialog::getExistingDirectory(
        this, tr("Select Default Path"), default_path);
    if (default_path.isEmpty()) {
      LOG_W() << "No default path selected.";
      return;
    }
  }

  SlotOpenPath(default_path);
}

auto TextEditTabWidget::FindFilePageByPath(const QString& path) const -> int {
  const auto target_path = QDir::cleanPath(QFileInfo(path).absoluteFilePath());

  for (int i = 0; i < count(); ++i) {
    auto* page = qobject_cast<FilePage*>(widget(i));
    if (page == nullptr) continue;

    const auto current_path =
        QDir::cleanPath(QFileInfo(page->GetCurrentPath()).absoluteFilePath());

    if (!current_path.isEmpty() && current_path == target_path) {
      return i;
    }
  }

  return -1;
}

void TextEditTabWidget::SlotOpenPath(const QString& target_path) {
  const int existing_index = FindFilePageByPath(target_path);
  if (existing_index >= 0) {
    setCurrentIndex(existing_index);
    return;
  }

  const auto abs_path = QFileInfo(target_path).absoluteFilePath();

  auto* page = new FilePage(qobject_cast<QWidget*>(parent()), target_path);
  page->setProperty("type", "file");
  page->setProperty("initial_path", abs_path);

  const auto title = WorkspaceTitleFromPath(abs_path);
  page->setProperty("base_title", title);

  const int index = addTab(page, QIcon(":/icons/workspace.png"), title);
  setTabToolTip(index, abs_path);
  setCurrentIndex(index);

  connect(page, &FilePage::SignalPathChanged, this,
          [this, page](const QString& path) {
            UpdateFilePageTabTitle(page, path);
          });

  page->SlotGoPath();
}

void TextEditTabWidget::UpdateFilePageTabTitle(QWidget* page,
                                               const QString& path) {
  const int index = indexOf(page);
  if (index < 0) return;

  if (path.isEmpty()) {
    setTabText(index, tr("Workspace"));
    setTabToolTip(index, {});
    return;
  }

  const auto absolute_path = QFileInfo(path).absoluteFilePath();
  const auto title = WorkspaceTitleFromPath(absolute_path);
  page->setProperty("base_title", title);
  setTabText(index, title);
  setTabToolTip(index, absolute_path);

  emit UISignalStation::GetInstance()->SignalMainWindowUpdateBasicOperaMenu(0);
}

void TextEditTabWidget::SlotCacheTextEditors() {
  int tab_count = this->count();

  struct TextEditorStatus {
    int index;
    QString title;
    GFBuffer content;
    QString type;
  };

  QContainer<TextEditorStatus> unsaved_pages;

  for (int i = 0; i < tab_count; i++) {
    auto* target_page = qobject_cast<PlainTextEditorPage*>(this->widget(i));

    // if this page is no textedit, there should be nothing to save
    if (target_page == nullptr) {
      continue;
    }

    auto* document = target_page->GetTextPage()->document();
    auto tab_title = NormalizeTabTitle(this->tabText(i));
    if (!target_page->ReadDone() || !target_page->isEnabled() ||
        !document->isModified()) {
      continue;
    }

    auto content = document->toRawText();

    unsaved_pages.push_back({
        i,
        tab_title,
        GFBuffer(content),
        "text_editor",
    });

    content.fill('X');
    content.clear();
  }

  auto& gss = GlobalSettingStation::GetInstance();
  CacheObject cache("editor_pages_cache");
  QJsonArray unsaved_page_array;
  for (const auto& page : unsaved_pages) {
    QJsonObject page_json;
    page_json["index"] = page.index;
    page_json["type"] = page.type;
    page_json["title"] = page.title;

    auto encrypted_content =
        GFBufferFactory::Encrypt(gss.GetActiveAppSecureKey(), page.content);
    if (!encrypted_content) continue;

    auto base64_content = GFBufferFactory::ToBase64(*encrypted_content);
    if (!base64_content) continue;

    page_json["content"] = base64_content->ConvertToQString();
    page_json["key_id"] =
        QString::fromLatin1(gss.GetActiveKeyId().ConvertToQByteArray().toHex());

    unsaved_page_array.push_back(page_json);
  }

  cache.setArray(unsaved_page_array);
}

void TextEditTabWidget::SlotRestoreTextEditorsCache() {
  bool restore_text_editor_page =
      GetSettings().value("basic/restore_text_editor_page", false).toBool();
  if (!restore_text_editor_page) {
    CacheObject cache("editor_pages_cache");
    cache.setObject(QJsonObject());
    return;
  }

  auto json_data =
      CacheManager::GetInstance().LoadDurableCache("editor_pages_cache");

  if (json_data.isEmpty() || !json_data.isArray()) return;

  auto& gss = GlobalSettingStation::GetInstance();
  auto json_array = json_data.array();
  for (const auto& value_ref : json_array) {
    if (!value_ref.isObject()) continue;
    auto json = value_ref.toObject();

    if (!json.contains("title") || !json.contains("content")) {
      continue;
    }

    const auto title = json["title"].toString();
    const auto base64_content = json["content"].toString();
    const auto type = json["type"].toString().trimmed().toLower();
    if (!type.isEmpty() && type != "text_editor") {
      continue;
    }

    auto encrypted_content =
        GFBufferFactory::FromBase64(GFBuffer(base64_content));
    if (!encrypted_content) continue;

    auto key_id = QByteArray::fromHex(json["key_id"].toString().toLatin1());
    auto key = gss.GetAppSecureKey(GFBuffer(key_id));
    key_id.fill('X');
    key_id.clear();

    if (key.Empty()) continue;

    auto content = GFBufferFactory::Decrypt(key, *encrypted_content);
    if (!content) continue;

    LOG_D() << "restoring text editor tab, title: " << title;
    SlotNewTabWithGFBuffer(title, *content);
  }

  CacheObject cache("editor_pages_cache");
  cache.setObject(QJsonObject());
}

auto TextEditTabWidget::generate_new_title(const QString& prefix,
                                           const QString& suffix) -> QString {
  const auto normalized_suffix = suffix.trimmed();

  if (normalized_suffix.isEmpty()) {
    return prefix + QString::number(++count_page_);
  }

  return prefix + QString::number(++count_page_) + "." + normalized_suffix;
}

auto TextEditTabWidget::CurPage() -> QWidget* { return this->currentWidget(); }

void TextEditTabWidget::UpdateTabModifiedMark(QWidget* page, bool modified) {
  const int index = indexOf(page);
  if (index < 0) return;

  auto base_title = page->property("base_title").toString().trimmed();
  if (base_title.isEmpty()) {
    base_title = NormalizeTabTitle(tabText(index));
    page->setProperty("base_title", base_title);
  }

  setTabText(index, modified ? "* " + base_title : base_title);
}

auto TextEditTabWidget::CreatePlainTextTab(const QString& title,
                                           const QString& file_path,
                                           const QIcon& icon)
    -> PlainTextEditorPage* {
  auto clean_title = NormalizeTabTitle(title);
  if (clean_title.isEmpty()) {
    clean_title = generate_new_title("untitled", "txt");
  }

  auto* page = new PlainTextEditorPage(file_path);
  page->setProperty("type", "text");
  page->setProperty("base_title", clean_title);

  const int index = addTab(page, icon, clean_title);
  setCurrentIndex(index);
  setTabToolTip(index, file_path.isEmpty() ? clean_title : file_path);

  connect(page->GetTextPage()->document(), &QTextDocument::modificationChanged,
          this, [this, page](bool modified) {
            UpdateTabModifiedMark(page, modified);
          });

  connect(page->GetTextPage()->document(), &QTextDocument::contentsChanged,
          this, &TextEditTabWidget::slot_save_status_to_cache_for_recovery);

  page->GetTextPage()->setFocus();
  return page;
}

void TextEditTabWidget::OpenDroppedUrl(const QUrl& url) {
  if (!url.isLocalFile()) return;

  const QString local_file = url.toLocalFile();
  QFileInfo file_info(local_file);

  if (!file_info.exists()) return;

  if (file_info.isDir()) {
    if (!file_info.isReadable() || !file_info.isExecutable()) {
      QMessageBox::warning(
          this, tr("Directory Permission Denied"),
          tr("You do not have permission to access the directory \"%1\".")
              .arg(file_info.fileName()));
      return;
    }

    SlotOpenPath(file_info.absoluteFilePath());
    return;
  }

  if (file_info.isFile()) {
    SlotOpenFile(local_file);
  }
}

auto TextEditTabWidget::WorkspaceTitleFromPath(const QString& path) -> QString {
  const QFileInfo info(path);
  auto name = info.fileName();

  if (name.isEmpty()) {
    const auto clean_path = QDir::cleanPath(path);
    if (clean_path == QDir::rootPath()) {
      return tr("Root");
    }
    name = clean_path;
  }

  return name.isEmpty() ? tr("Workspace") : name;
}

}  // namespace GpgFrontend::UI
