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

auto NormalizeTabTitle(QString title) -> QString {
  title = title.trimmed();

  while (title.startsWith("*")) {
    title.remove(0, 1);
    title = title.trimmed();
  }

  return title;
}

class ScopedRecoverySuspend {
 public:
  explicit ScopedRecoverySuspend(QObject* object) : object_(object) {
    if (object_ != nullptr) {
      object_->setProperty("recovery_suspended", true);
    }
  }

  ~ScopedRecoverySuspend() {
    if (object_ != nullptr) {
      object_->setProperty("recovery_suspended", false);
    }
  }

 private:
  QObject* object_ = nullptr;
};

auto ContainsPagePointer(
    const QList<QPointer<GpgFrontend::UI::PlainTextEditorPage>>& pages,
    GpgFrontend::UI::PlainTextEditorPage* page) -> bool {
  if (page == nullptr) return false;

  return std::any_of(
      pages.cbegin(), pages.cend(),
      [page](const QPointer<GpgFrontend::UI::PlainTextEditorPage>& p) {
        return p == page;
      });
}

constexpr auto kEditorPagesCacheKey = "editor_pages_cache";

void SaveEditorPagesRecoveryCache(const QJsonObject& object,
                                  bool flush = true) {
  CacheManager::GetInstance().SaveDurableCache(kEditorPagesCacheKey,
                                               QJsonDocument(object), flush);
}

void ClearEditorPagesRecoveryCache(bool flush = true) {
  CacheManager::GetInstance().SaveDurableCache(
      kEditorPagesCacheKey, QJsonDocument(QJsonObject()), flush);
}

}  // namespace

TextEditTabWidget::TextEditTabWidget(QWidget* parent) : QTabWidget(parent) {
  init_tab_style();

  recovery_cache_timer_ = new QTimer(this);
  recovery_cache_timer_->setSingleShot(true);
  recovery_cache_timer_->setInterval(1200);

  connect(recovery_cache_timer_, &QTimer::timeout, this,
          [this]() -> void { flush_recovery_cache(false); });

  connect(qApp, &QCoreApplication::aboutToQuit, this,
          [this]() -> void { flush_recovery_cache(true); });

  connect(this, &QTabWidget::currentChanged, this, [this](int) -> void {
    if (ContainsPagePointer(recovery_dirty_pages_, last_current_text_page_)) {
      flush_recovery_cache(false);
    }

    last_current_text_page_ = CurTextPage();
  });

  tabBar()->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(
      tabBar(), &QTabBar::customContextMenuRequested, this,
      [this](const QPoint& pos) -> void {
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

  last_current_text_page_ = CurTextPage();
}

void TextEditTabWidget::init_tab_style() {
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

auto TextEditTabWidget::can_open_as_text_file(const QFileInfo& file_info,
                                              QString* error_message) -> bool {
  if (!file_info.exists() || !file_info.isFile()) {
    if (error_message != nullptr) {
      *error_message = tr("The file does not exist.");
    }
    return false;
  }

  constexpr auto kMaxTextEditorFileSize =
      static_cast<const qint64>(1024 * 1024);
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
    open_dropped_url(url);
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

  const int existing_index = find_tab_by_file_path(path);
  if (existing_index >= 0) {
    setCurrentIndex(existing_index);
    if (auto* page =
            qobject_cast<PlainTextEditorPage*>(widget(existing_index))) {
      page->GetTextPage()->setFocus();
    }
    return;
  }

  QString error_message;
  if (!can_open_as_text_file(file_info, &error_message)) {
    QMessageBox::warning(this, tr("File Open Error"), error_message);
    return;
  }

  ScopedOverrideCursor wait_cursor(Qt::WaitCursor);
  auto* page = create_plain_text_tab(stripped_name(path), path,
                                     QIcon(":/icons/file.png"), {});

  {
    ScopedRecoverySuspend recovery_suspend(page);
    page->ReadFile();
  }
}

auto TextEditTabWidget::find_tab_by_file_path(const QString& path) const
    -> int {
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

  update_tab_modified_mark(page, changed);
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

auto TextEditTabWidget::SlotNewPlainTextTab() -> QWidget* {
  const auto header = generate_new_title("untitled", "txt");
  return create_plain_text_tab(header, {}, QIcon(":/icons/file.png"), {});
}

auto TextEditTabWidget::SlotNewTab(const QString& type, const QString& title,
                                   const QIcon& icon, const QString& icon_name)
    -> QWidget* {
  auto* page = create_plain_text_tab(title, {}, icon, icon_name);
  page->setProperty("type", type);

  if (!icon_name.trimmed().isEmpty()) {
    page->setProperty("icon_name", icon_name.trimmed());
    const int index = indexOf(page);
    if (index >= 0) {
      setTabIcon(index, QIcon(icon_name.trimmed()));
    }
  }

  return page;
}

void TextEditTabWidget::SlotNewTabWithGFBuffer(QString title,
                                               const GFBuffer& buffer) {
  QString header = NormalizeTabTitle(std::move(title));
  if (header.isEmpty()) {
    header = generate_new_title("untitled", "txt");
  }

  auto* page = create_plain_text_tab(header, {}, QIcon(":/icons/file.png"), {});

  {
    ScopedRecoverySuspend recovery_suspend(page);
    page->GetTextPage()->document()->setPlainText(buffer.ConvertToQString());
    page->GetTextPage()->document()->setModified(true);
    update_tab_modified_mark(page, true);
  }

  page->GetTextPage()->setFocus();
}

void TextEditTabWidget::SlotNewTabWithContent(QString title,
                                              const QString& content) {
  auto header = NormalizeTabTitle(std::move(title));
  if (header.isEmpty()) {
    header = generate_new_title("untitled", "txt");
  }

  auto* page = create_plain_text_tab(header, {}, QIcon(":/icons/file.png"), {});

  {
    ScopedRecoverySuspend recovery_suspend(page);
    page->GetTextPage()->document()->setPlainText(content);
    page->GetTextPage()->document()->setModified(true);
    update_tab_modified_mark(page, true);
  }
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

auto TextEditTabWidget::find_file_page_by_path(const QString& path) const
    -> int {
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
  const int existing_index = find_file_page_by_path(target_path);
  if (existing_index >= 0) {
    setCurrentIndex(existing_index);
    return;
  }

  const auto abs_path = QFileInfo(target_path).absoluteFilePath();

  auto* page = new FilePage(qobject_cast<QWidget*>(parent()), target_path);
  page->setProperty("type", "file");
  page->setProperty("initial_path", abs_path);

  const auto title = workspace_title_from_path(abs_path);
  page->setProperty("base_title", title);

  const int index = addTab(page, QIcon(":/icons/workspace.png"), title);
  setTabToolTip(index, abs_path);
  setCurrentIndex(index);

  connect(page, &FilePage::SignalPathChanged, this,
          [this, page](const QString& path) {
            update_file_page_tab_title(page, path);
          });

  page->SlotGoPath();
}

void TextEditTabWidget::update_file_page_tab_title(QWidget* page,
                                                   const QString& path) {
  const int index = indexOf(page);
  if (index < 0) return;

  if (path.isEmpty()) {
    setTabText(index, tr("Workspace"));
    setTabToolTip(index, {});
    return;
  }

  const auto absolute_path = QFileInfo(path).absoluteFilePath();
  const auto title = workspace_title_from_path(absolute_path);
  page->setProperty("base_title", title);
  setTabText(index, title);
  setTabToolTip(index, absolute_path);

  emit UISignalStation::GetInstance()->SignalMainWindowUpdateBasicOperaMenu(0);
}

void TextEditTabWidget::SlotCacheTextEditors() {
  const int tab_count = this->count();

  struct TextEditorStatus {
    int index;
    QString title;
    QString file_path;
    QString page_type;
    QString icon_name;
    GFBuffer content;
  };

  QContainer<TextEditorStatus> unsaved_pages;

  for (int i = 0; i < tab_count; i++) {
    auto* target_page = qobject_cast<PlainTextEditorPage*>(this->widget(i));
    if (target_page == nullptr) continue;

    auto* document = target_page->GetTextPage()->document();
    auto tab_title = NormalizeTabTitle(this->tabText(i));

    if (!target_page->isEnabled() || document == nullptr ||
        !document->isModified()) {
      continue;
    }

    const bool is_file_backed = !target_page->GetFilePath().isEmpty();
    if (is_file_backed && !target_page->ReadDone()) {
      continue;
    }

    auto content = document->toRawText();

    auto page_type = target_page->property("type").toString().trimmed();
    if (page_type.isEmpty()) {
      page_type = "text";
    }

    auto icon_name = target_page->property("icon_name").toString().trimmed();
    if (icon_name.isEmpty()) {
      icon_name = QStringLiteral(":/icons/file.png");
    }

    unsaved_pages.push_back({
        i,
        tab_title,
        target_page->GetFilePath(),
        page_type,
        icon_name,
        GFBuffer(content),
    });

    content.fill('X');
    content.clear();
  }

  if (unsaved_pages.empty()) {
    ClearEditorPagesRecoveryCache(true);
    return;
  }

  auto& gss = GlobalSettingStation::GetInstance();
  QJsonArray unsaved_page_array;

  for (const auto& page : unsaved_pages) {
    QJsonObject page_json;
    page_json["index"] = page.index;
    page_json["type"] = page.page_type;
    page_json["recovery_type"] = "text_editor";
    page_json["title"] = page.title;
    page_json["file_path"] = page.file_path;
    page_json["icon_name"] = page.icon_name;

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

  if (unsaved_page_array.isEmpty()) {
    ClearEditorPagesRecoveryCache(true);
    return;
  }

  QJsonObject root;
  root["version"] = 2;
  root["updated_at"] =
      QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
  root["pages"] = unsaved_page_array;

  LOG_D() << "caching text editor status, unsaved page count:"
          << unsaved_pages.size();

  SaveEditorPagesRecoveryCache(root, true);
}

void TextEditTabWidget::SlotRestoreTextEditorsCache() {
  QTimer::singleShot(200, this,
                     [this]() -> void { SlotRestoreTextEditorsCacheNow(); });
}

void TextEditTabWidget::SlotRestoreTextEditorsCacheNow() {
  const bool restore_text_editor_page =
      GetSettings().value("basic/restore_text_editor_page", false).toBool();
  if (!restore_text_editor_page) {
    ClearEditorPagesRecoveryCache(true);
    return;
  }

  LOG_D() << "restoring text editor cache...";

  auto json_data =
      CacheManager::GetInstance().LoadDurableCache(kEditorPagesCacheKey);

  LOG_D() << "editor_pages_cache loaded"
          << ", json content:" << json_data.toJson();

  if (json_data.isEmpty()) return;

  QJsonArray json_array;
  if (json_data.isArray()) {
    json_array = json_data.array();
  } else if (json_data.isObject()) {
    json_array = json_data.object().value("pages").toArray();
  } else {
    return;
  }

  if (json_array.isEmpty()) {
    ClearEditorPagesRecoveryCache(true);
    return;
  }

  recovery_restoring_ = true;
  auto restore_guard = qScopeGuard([this]() {
    recovery_restoring_ = false;
    recovery_dirty_pages_.clear();

    if (recovery_cache_timer_ != nullptr) {
      recovery_cache_timer_->stop();
    }
  });

  auto& gss = GlobalSettingStation::GetInstance();
  QJsonArray next_recovery_pages;
  QPointer<PlainTextEditorPage> last_restored_page;
  int restored_count = 0;

  for (const auto& value_ref : json_array) {
    bool restored = false;

    if (!value_ref.isObject()) {
      next_recovery_pages.push_back(value_ref);
      continue;
    }

    auto json = value_ref.toObject();

    if (!json.contains("title") || !json.contains("content")) {
      next_recovery_pages.push_back(value_ref);
      continue;
    }

    const auto title = json["title"].toString();
    const auto base64_content = json["content"].toString();
    const auto file_path = json["file_path"].toString();

    auto icon_name = json["icon_name"].toString().trimmed();
    if (icon_name.isEmpty()) {
      icon_name = json["icon"].toString().trimmed();
    }
    if (icon_name.isEmpty()) {
      icon_name = QStringLiteral(":/icons/file.png");
    }

    auto page_type = json["type"].toString().trimmed();
    auto page_type_key = page_type.toLower();

    auto recovery_type = json["recovery_type"].toString().trimmed();
    auto recovery_type_key = recovery_type.toLower();

    // Backward compatibility:
    // old cache used "type": "text_editor".
    if (recovery_type_key.isEmpty() && page_type_key == "text_editor") {
      recovery_type = "text_editor";
      recovery_type_key = "text_editor";
      page_type = "text";
      page_type_key = "text";
    }

    if (recovery_type_key.isEmpty()) {
      recovery_type = "text_editor";
      recovery_type_key = "text_editor";
    }

    if (page_type.isEmpty()) {
      page_type = "text";
      page_type_key = "text";
    }

    // Only recovery_type decides which recovery handler to use.
    // page_type is semantic type, e.g. "text", "email", module-defined type.
    if (recovery_type_key != "text_editor") {
      next_recovery_pages.push_back(value_ref);
      continue;
    }

    auto encrypted_content =
        GFBufferFactory::FromBase64(GFBuffer(base64_content));
    if (!encrypted_content) {
      next_recovery_pages.push_back(value_ref);
      continue;
    }

    auto key_id = QByteArray::fromHex(json["key_id"].toString().toLatin1());
    auto key = gss.GetAppSecureKey(GFBuffer(key_id));
    key_id.fill('X');
    key_id.clear();

    if (!key.Empty()) {
      auto content = GFBufferFactory::Decrypt(key, *encrypted_content);
      if (content) {
        const auto restore_title = NormalizeTabTitle(title).isEmpty()
                                       ? generate_new_title("untitled", "txt")
                                       : NormalizeTabTitle(title);

        LOG_D() << "restoring text editor tab"
                << ", title:" << restore_title << ", type:" << page_type
                << ", recovery_type:" << recovery_type
                << ", icon:" << icon_name;

        auto* page = create_plain_text_tab(restore_title, file_path,
                                           QIcon(icon_name), icon_name);

        {
          ScopedRecoverySuspend recovery_suspend(page);

          page->setProperty("type", page_type);
          page->setProperty("recovery_type", recovery_type);
          page->setProperty("recovered_from_cache", true);
          page->setProperty("icon_name", icon_name);

          const int page_index = indexOf(page);
          if (page_index >= 0) {
            setTabIcon(page_index, QIcon(icon_name));
          }

          page->GetTextPage()->document()->setPlainText(
              content->ConvertToQString());
          page->GetTextPage()->document()->setModified(true);
          update_tab_modified_mark(page, true);
        }

        page->GetTextPage()->setFocus();

        last_restored_page = page;
        ++restored_count;

        LOG_D() << "restored text editor page"
                << ", title:" << restore_title << ", index:" << indexOf(page)
                << ", tab count:" << count();

        // Important:
        // Keep the recovery entry after successful restore. The restored page
        // is still unsaved. If the app crashes again before the user
        // edits/saves/closes it, this entry must still be available.
        next_recovery_pages.push_back(value_ref);

        restored = true;
      }
    }

    if (!restored) {
      next_recovery_pages.push_back(value_ref);
    }
  }

  if (next_recovery_pages.isEmpty()) {
    ClearEditorPagesRecoveryCache(true);
  } else {
    QJsonObject root;
    root["version"] = 2;
    root["updated_at"] =
        QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
    root["pages"] = next_recovery_pages;
    SaveEditorPagesRecoveryCache(root, true);
  }

  if (last_restored_page != nullptr) {
    const int restored_index = indexOf(last_restored_page);
    if (restored_index >= 0) {
      setCurrentIndex(restored_index);
      last_restored_page->GetTextPage()->setFocus();

      LOG_D() << "activated restored text editor tab"
              << ", index:" << restored_index
              << ", restored count:" << restored_count
              << ", tab count:" << count();
    } else {
      LOG_W() << "restored text editor page is no longer in tab widget"
              << ", restored count:" << restored_count
              << ", tab count:" << count();
    }
  }
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

void TextEditTabWidget::update_tab_modified_mark(QWidget* page, bool modified) {
  const int index = indexOf(page);
  if (index < 0) return;

  auto base_title = page->property("base_title").toString().trimmed();
  if (base_title.isEmpty()) {
    base_title = NormalizeTabTitle(tabText(index));
    page->setProperty("base_title", base_title);
  }

  setTabText(index, modified ? "* " + base_title : base_title);
}

auto TextEditTabWidget::create_plain_text_tab(const QString& title,
                                              const QString& file_path,
                                              const QIcon& icon,
                                              const QString& icon_name)
    -> PlainTextEditorPage* {
  auto clean_title = NormalizeTabTitle(title);
  if (clean_title.isEmpty()) {
    clean_title = generate_new_title("untitled", "txt");
  }

  const bool has_explicit_icon_name = !icon_name.trimmed().isEmpty();
  const auto effective_icon_name = has_explicit_icon_name
                                       ? icon_name.trimmed()
                                       : QStringLiteral(":/icons/file.png");

  const QIcon effective_icon =
      has_explicit_icon_name ? QIcon(effective_icon_name) : icon;

  auto* page = new PlainTextEditorPage(file_path);
  page->setProperty("type", "text");
  page->setProperty("base_title", clean_title);
  page->setProperty("icon_name", effective_icon_name);

  const int index = addTab(page, effective_icon, clean_title);
  setCurrentIndex(index);
  setTabToolTip(index, file_path.isEmpty() ? clean_title : file_path);

  connect(page->GetTextPage()->document(), &QTextDocument::modificationChanged,
          this, [this, page](bool modified) {
            update_tab_modified_mark(page, modified);
          });

  connect(page->GetTextPage()->document(), &QTextDocument::contentsChanged,
          this, [this, page]() -> void { schedule_recovery_cache(page); });

  page->GetTextPage()->setFocus();
  return page;
}

void TextEditTabWidget::open_dropped_url(const QUrl& url) {
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

auto TextEditTabWidget::workspace_title_from_path(const QString& path)
    -> QString {
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

void TextEditTabWidget::schedule_recovery_cache(PlainTextEditorPage* page) {
  if (page == nullptr) return;
  if (recovery_restoring_) return;
  if (page->property("recovery_suspended").toBool()) return;

  const bool restore_text_editor_page =
      GetSettings().value("basic/restore_text_editor_page", false).toBool();
  if (!restore_text_editor_page) return;

  auto* document = page->GetTextPage()->document();
  if (document == nullptr || !document->isModified()) return;

  if (!ContainsPagePointer(recovery_dirty_pages_, page)) {
    recovery_dirty_pages_.push_back(QPointer<PlainTextEditorPage>(page));
  }

  if (recovery_cache_timer_ != nullptr) {
    recovery_cache_timer_->start();
  }
}

void TextEditTabWidget::flush_recovery_cache(bool force) {
  const bool restore_text_editor_page =
      GetSettings().value("basic/restore_text_editor_page", false).toBool();

  if (!restore_text_editor_page) return;

  if (!force && recovery_dirty_pages_.isEmpty()) return;

  SlotCacheTextEditors();

  recovery_dirty_pages_.clear();

  if (recovery_cache_timer_ != nullptr) {
    recovery_cache_timer_->stop();
  }
}

void TextEditTabWidget::SlotRefreshRecoveryCache() {
  if (recovery_cache_timer_ != nullptr) {
    recovery_cache_timer_->stop();
  }

  recovery_dirty_pages_.clear();

  SlotCacheTextEditors();
}
void TextEditTabWidget::SlotTabClosedForRecovery() {
  if (recovery_cache_timer_ != nullptr) {
    recovery_cache_timer_->stop();
  }

  recovery_dirty_pages_.clear();

  SlotCacheTextEditors();
}

}  // namespace GpgFrontend::UI
