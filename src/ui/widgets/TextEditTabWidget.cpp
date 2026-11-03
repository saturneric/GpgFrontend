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
#include "core/function/SettingCatalog.h"
#include "core/model/CacheObject.h"
#include "core/model/GFBuffer.h"
#include "core/module/ModuleManager.h"
#include "core/profile/ProfileSession.h"
#include "core/utils/CommonUtils.h"
#include "core/utils/MemoryUtils.h"
#include "ui/UIModuleManager.h"
#include "ui/UISignalStation.h"
#include "ui/function/FilePanelPath.h"
#include "ui/lua/LuaHost.h"
#include "ui/lua/LuaPlacements.h"
#include "ui/lua/NativeWidgetRegistry.h"
#include "ui/widgets/EditorTabBar.h"
#include "ui/widgets/FilePage.h"
#include "ui/widgets/PlainTextEditorPage.h"

namespace GpgFrontend::UI {

namespace {

class ScopedOverrideCursor {
 public:
  explicit ScopedOverrideCursor(const QCursor& cursor) {
    QApplication::setOverrideCursor(cursor);
  }

  ~ScopedOverrideCursor() { QApplication::restoreOverrideCursor(); }
};

// Only trimmed: the unsaved mark is drawn by the tab bar, never written into
// a title, so a "*" or "●" at the start of one is the document's own.
auto NormalizeTabTitle(QString title) -> QString { return title.trimmed(); }

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

  connect(qApp, &QCoreApplication::aboutToQuit, this, [this]() -> void {
    // From here on the tabs are being torn down, so an empty sweep means
    // "they are already gone", not "there was nothing worth keeping".
    recovery_shutting_down_ = true;
    flush_recovery_cache(true);
  });

  connect(this, &QTabWidget::currentChanged, this, [this](int index) -> void {
    if (ContainsPagePointer(recovery_dirty_pages_, last_current_text_page_)) {
      flush_recovery_cache(false);
    }

    last_current_text_page_ = CurTextPage();

    // A semantic event, not a re-broadcast of the Qt signal. A module is told
    // WHICH tab and what kind it is; it is not handed a QTabWidget and left
    // to work out the rest, which would make the module depend on a host
    // widget class it cannot include.
    auto* page = widget(index);
    if (page == nullptr) return;

    // The first tab is current before tabInserted() has named it.
    if (TabIdOf(page) == 0) page->setProperty("gf_tab_id", next_tab_id_++);

    // A Ctrl+Tab walk passes over tabs; only the one it ends on was used.
    if (!switch_session_.Active()) mru_.Touch(TabIdOf(page));

    Module::TriggerEvent(
        "TAB_ACTIVATED",
        {{"tab_index", GFBuffer{QString::number(index)}},
         {"tab_type", GFBuffer{page->property("type").toString()}},
         {"tab_title", GFBuffer{TabTitleOf(page)}}});
  });

  tabBar()->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(tabBar(), &QTabBar::customContextMenuRequested, this,
          [this](const QPoint& pos) -> void {
            const int index = tabBar()->tabAt(pos);
            if (index >= 0) show_tab_menu(index, pos);
          });

  // Middle-click and the close buttons arrive as tabCloseRequested(), which
  // TextEdit turns into the one guarded close path.
  connect(bar_, &EditorTabBar::SignalNewTabRequested, this,
          [this]() -> void { SlotNewPlainTextTab(); });

  // The end of the strip: every open tab, searchable.
  auto* list_button = new QToolButton(this);
  list_button->setObjectName("gfOpenTabsButton");
  list_button->setAutoRaise(true);
  list_button->setArrowType(Qt::DownArrow);
  list_button->setToolButtonStyle(Qt::ToolButtonIconOnly);
  list_button->setToolTip(tr("Show Open Tabs"));
  list_button->setFocusPolicy(Qt::NoFocus);
  connect(list_button, &QToolButton::clicked, this,
          &TextEditTabWidget::SignalOpenTabsListRequested);
  setCornerWidget(list_button, Qt::TopRightCorner);

  last_current_text_page_ = CurTextPage();
}

void TextEditTabWidget::init_tab_style() {
  // Before any tab exists: setTabBar() is only allowed on an empty widget.
  bar_ = new EditorTabBar(this);
  setTabBar(bar_);

  setAcceptDrops(true);
  setMovable(true);
  // The bar draws its own close buttons, which double as the unsaved mark.
  // QTabWidget relays the bar's close requests only while it is closable
  // itself, so they are relayed here.
  setTabsClosable(false);
  connect(bar_, &QTabBar::tabCloseRequested, this,
          &QTabWidget::tabCloseRequested);
  setDocumentMode(true);
  // Document mode turns the bar's own base line back on; the strip's one
  // separator is drawn in paintEvent() instead.
  bar_->setDrawBase(false);
  setUsesScrollButtons(true);
  setElideMode(Qt::ElideRight);

  tabBar()->setProperty("gfTextEditTabBar", true);
  tabBar()->style()->unpolish(tabBar());
  tabBar()->style()->polish(tabBar());

  setStyleSheet(R"(
QTabWidget::pane {
  border: none;
}

QTabBar[gfTextEditTabBar="true"]::tab {
  padding: 4px 6px 5px 10px;
  margin-right: 0px;
  border-top: 2px solid transparent;
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
  border-top: 2px solid palette(highlight);
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
      static_cast<const qint64>(4 * 1024 * 1024);
  if (file_info.size() > kMaxTextEditorFileSize) {
    if (error_message != nullptr) {
      *error_message = tr("The file \"%1\" is larger than 4 MB and will not be "
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

  // A document type a module's UI script mounted an editor for: the Host
  // opens the file itself, in a tab of that type, and the module's view
  // loads it like any other document.
  // Matched on the end of the name, the longest extension winning: a
  // compound one ("textpack.gpg") must not be taken for its last part.
  const auto name = file_info.fileName().toLower();
  const auto mounts =
      Lua::LuaHost::Instance().MountsOf(Lua::AnchorKind::kEDITOR);
  qsizetype best_length = 0;
  const Lua::PlacedMount* best = nullptr;
  for (const auto& m : mounts) {
    for (const auto& ext : m.info.extensions) {
      if (ext.size() > best_length && name.endsWith(u"."_s + ext)) {
        best_length = ext.size();
        best = &m;
      }
    }
  }
  if (best != nullptr) {
    const auto& m = *best;
    if (const int existing = find_tab_by_file_path(path); existing >= 0) {
      setCurrentIndex(existing);
      return;
    }
    const auto new_page = [&]() -> PlainTextEditorPage* {
      const auto entry = NativeWidgetRegistry::Instance().Find(m.info.widget);
      const auto icon = entry.has_value() ? entry->icon : QString();
      return qobject_cast<PlainTextEditorPage*>(
          SlotNewTab(m.info.document_type, stripped_name(path), QIcon(), icon));
    };
    if (m.info.module_persisted) {
      // The view reads the file; the Host does not.
      auto* page = new_page();
      if (page == nullptr) return;
      if (!page->ModuleOpenPath(path)) {
        // Undone the way a closed tab is, so a module that heard it created
        // also hears it gone.
        const auto index = indexOf(page);
        removeTab(index);
        Module::TriggerEvent(
            "TAB_CLOSED",
            {{"tab_index", GFBuffer{QString::number(index)}},
             {"tab_type", GFBuffer{page->property("type").toString()}}});
        page->deleteLater();
        return;
      }
      RefreshTabLabels();
      return;
    }
    QString error_message;
    if (!can_open_as_text_file(file_info, &error_message)) {
      QMessageBox::warning(this, tr("File Open Error"), error_message);
      return;
    }
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
      QMessageBox::warning(this, tr("File Open Error"), file.errorString());
      return;
    }
    auto* page = new_page();
    if (page == nullptr) return;
    page->SetContentFromBytes(file.readAll());
    page->SetFilePath(path);
    page->NotifyFileSaved();
    RefreshTabLabels();
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
  const auto target = FileIdentityKey(path);
  if (target.isEmpty()) return -1;

  for (int i = 0; i < count(); ++i) {
    auto* page = qobject_cast<PlainTextEditorPage*>(widget(i));
    if (page == nullptr) continue;

    const auto page_path = page->GetFilePath();
    if (page_path.isEmpty()) continue;

    if (FileIdentityKey(page_path) == target) return i;
  }

  return -1;
}

void TextEditTabWidget::SlotShowModified(bool /*changed*/) {
  RefreshTabLabels();
}

auto TextEditTabWidget::CurTextPage() const -> PlainTextEditorPage* {
  return qobject_cast<PlainTextEditorPage*>(this->currentWidget());
}

auto TextEditTabWidget::CurPageTextEdit() const -> PlainTextEditorPage* {
  auto* cur_page = qobject_cast<PlainTextEditorPage*>(this->currentWidget());
  return cur_page;
}

auto TextEditTabWidget::CurPageIsPlainText() const -> bool {
  auto* page = CurTextPage();
  return page != nullptr && page->property("type").toString() == "text"_L1;
}

auto TextEditTabWidget::CurPageCryptoOperations(bool& has_opinion) const
    -> QStringList {
  has_opinion = false;
  auto* page = CurTextPage();
  if (page == nullptr) return {};
  return page->PrimaryViewCryptoOperations(has_opinion);
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

  mount_module_view(page, type);

  return page;
}

void TextEditTabWidget::mount_module_view(PlainTextEditorPage* page,
                                          const QString& type) {
  if (page == nullptr) return;

  // A module UI script's editor mount for this document type: the typed
  // native view, in the page's own container.
  for (const auto& m :
       Lua::LuaHost::Instance().MountsOf(Lua::AnchorKind::kEDITOR)) {
    if (m.info.document_type.compare(type, Qt::CaseInsensitive) != 0) continue;
    if (!page->MountNativeView(m.info.widget)) {
      LOG_W() << "the native view for document type" << type
              << "could not be mounted";
      return;
    }
    page->SetPrefersCompactStatusDock(m.info.compact_status);
    page->SetModulePersisted(m.info.module_persisted);
    if (page == currentWidget()) emit SignalCurrentPageViewMounted();
    return;
  }

  // A tab type no module has claimed stays an ordinary plain text tab, which
  // is what every tab type was before module views existed.
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
    RefreshTabLabels();
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
    RefreshTabLabels();
  }
}

void TextEditTabWidget::SlotOpenDefaultPath() {
  const auto mode = FilePanelDefaultPathModeFromString(
      SettingItems::kFilePanelDefaultPathMode.Read(GetSettings()));

  // Created rather than merely resolved, so the panel never falls back to the
  // working directory just because the folder has not been used yet.
  const auto workspace = mode == FilePanelDefaultPathMode::kWORKSPACE
                             ? ProfileSession::Instance().EnsureWorkspace()
                             : QString{};

  auto default_path = ResolveFilePanelDefaultPath(
      mode, workspace, QDir::homePath(), QDir::currentPath());

  // A workspace lives inside the application container, so the sandbox already
  // grants access to it and the "pick a folder" prompt at every startup can
  // simply go away.
  if (IsRunningInSandBox() && workspace.isEmpty()) {
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
  const auto target = FileIdentityKey(path);
  if (target.isEmpty()) return -1;

  for (int i = 0; i < count(); ++i) {
    auto* page = qobject_cast<FilePage*>(widget(i));
    if (page == nullptr) continue;

    const auto current_path = page->GetCurrentPath();
    if (!current_path.isEmpty() && FileIdentityKey(current_path) == target) {
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
  setCurrentIndex(index);

  Module::TriggerEvent("TAB_CREATED",
                       {{"tab_index", GFBuffer{QString::number(index)}},
                        {"tab_type", GFBuffer{QString("file")}},
                        {"tab_title", GFBuffer{title}}});

  connect(page, &FilePage::SignalPathChanged, this,
          [this, page](const QString& path) {
            update_file_page_tab_title(page, path);
          });

  // Re-enter the path now that the tab title is listening, so the tab is named
  // after the folder the page actually settled on.
  page->SlotGoPath(page->GetCurrentPath());
}

void TextEditTabWidget::update_file_page_tab_title(QWidget* page,
                                                   const QString& path) {
  const int index = indexOf(page);
  if (index < 0) return;

  if (path.isEmpty()) {
    SetTabTitle(page, tr("Workspace"));
    return;
  }

  const auto absolute_path = QFileInfo(path).absoluteFilePath();
  SetTabTitle(page, workspace_title_from_path(absolute_path));

  emit UISignalStation::GetInstance()
      -> SignalMainWindowUpdateBasicOperaMenu(0);
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

  QList<TextEditorStatus> unsaved_pages;

  for (int i = 0; i < tab_count; i++) {
    auto* target_page = qobject_cast<PlainTextEditorPage*>(this->widget(i));
    if (target_page == nullptr) continue;

    // Anything a mounted view is still holding has to reach the document
    // first, or what gets cached is the message as it was before the user
    // touched it -- which recovers a tab, but not their work.
    target_page->FlushPrimaryView();

    auto* document = target_page->GetTextPage()->document();
    auto tab_title = TabTitleOf(target_page);

    if (!target_page->isEnabled() || document == nullptr ||
        !document->isModified()) {
      continue;
    }

    const bool is_file_backed = !target_page->GetFilePath().isEmpty();
    if (is_file_backed && !target_page->ReadDone()) {
      continue;
    }

    // The page's BYTES, not the document's text. DocumentBytes() puts back the
    // line endings the document actually has, and it is exactly what saving
    // this tab to a file would write -- so what comes back after a crash is
    // the tab the user had, not a transcription of it.
    //
    // toRawText() lost that: the editor stores bare LF whatever it was given,
    // so a message signed over canonical CRLF came back from recovery with its
    // line endings rewritten, and a signature that was valid before the
    // restart could never verify again. It reads as a forgery, which is the
    // worst possible way to lose a byte.
    auto content = target_page->DocumentBytes();

    auto page_type = target_page->property("type").toString().trimmed();
    if (page_type.isEmpty()) {
      page_type = "text";
    }

    auto icon_name = target_page->property("icon_name").toString().trimmed();
    if (icon_name.isEmpty()) {
      icon_name = u":/icons/file.png"_s;
    }

    unsaved_pages.push_back({
        i,
        tab_title,
        target_page->GetFilePath(),
        page_type,
        icon_name,
        GFBuffer(content),
    });

    WipeByteArray(content);
  }

  if (unsaved_pages.empty()) {
    // Never on the way out. The snapshot that matters was written moments
    // earlier, while the tabs still existed; clearing it here because they
    // have since been destroyed is what silently threw away the user's
    // unsaved work on almost every quit.
    if (!recovery_shutting_down_) ClearEditorPagesRecoveryCache(true);
    return;
  }

  auto& key_mgr = ProfileSession::Instance().Keys();
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
        GFBufferFactory::Encrypt(key_mgr.ActiveKey(), page.content);
    if (!encrypted_content) continue;

    auto base64_content = GFBufferFactory::ToBase64(*encrypted_content);
    if (!base64_content) continue;

    page_json["content"] = base64_content->ConvertToQString();
    page_json["key_id"] = QString::fromLatin1(
        key_mgr.ActiveKeyId().ConvertToQByteArray().toHex());

    unsaved_page_array.push_back(page_json);
  }

  if (unsaved_page_array.isEmpty()) {
    if (!recovery_shutting_down_) ClearEditorPagesRecoveryCache(true);
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
      SettingItems::kRestoreTextEditorPage.Read(GetSettings());
  if (!restore_text_editor_page) {
    ClearEditorPagesRecoveryCache(true);
    return;
  }

  LOG_D() << "restoring text editor cache...";

  auto json_data =
      CacheManager::GetInstance().LoadDurableCache(kEditorPagesCacheKey);

  if (json_data.isEmpty()) {
    LOG_D() << "no text editor recovery cache stored";
    return;
  }

  QJsonArray json_array;
  if (json_data.isArray()) {
    json_array = json_data.array();
  } else if (json_data.isObject()) {
    json_array = json_data.object().value("pages").toArray();
  } else {
    return;
  }

  LOG_D() << "text editor recovery cache loaded, array:" << json_data.isArray()
          << ", object:" << json_data.isObject()
          << ", entries:" << json_array.size();

  if (json_array.isEmpty()) {
    LOG_D() << "text editor recovery cache holds no pages";
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

  auto& key_mgr = ProfileSession::Instance().Keys();
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
      icon_name = u":/icons/file.png"_s;
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
    // page_type is the semantic type, e.g. "text", "email", module-defined
    // type.
    if (recovery_type_key != "text_editor") {
      LOG_D() << "skipping recovery entry handled elsewhere, recovery_type:"
              << recovery_type;
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
    auto key = key_mgr.KeyById(GFBuffer(key_id));
    WipeByteArray(key_id);

    if (key.Empty()) {
      // Losing unsaved work must never be silent: without this the tab simply
      // does not come back and nothing anywhere says why.
      LOG_W() << "cannot restore text editor tab, its encryption key is not "
                 "available, title:"
              << title;
    }

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

          // Through the page, as BYTES: it is the only thing that records
          // which line endings the content has, and a write straight to the
          // document throws that away. A cache entry written before this was
          // fixed simply has no CR in it, and comes back exactly as it did
          // before -- there is nothing there to lose.
          {
            auto bytes = content->ConvertToQByteArray();
            page->SetContentFromBytes(bytes);
            WipeByteArray(bytes);
          }

          // SetContentFromBytes() leaves the document clean, which would say
          // it matches a file on disk. A recovered tab matches nothing: it is
          // unsaved work, and the asterisk and the close prompt depend on
          // this saying so.
          page->GetTextPage()->document()->setModified(true);
          RefreshTabLabels();

          // The page type is restored, so the view that belongs to it has to
          // be restored with it -- otherwise an e-mail comes back as raw text
          // and the tab silently loses everything the module gave it.
          //
          // Mounted after the document is filled in, because mounting reloads
          // the view from the document: doing it first would hand the view an
          // empty page.
          mount_module_view(page, page_type);
        }

        // The raw editor may now be inside the mounted view rather than the
        // page, where focusing it would put the caret somewhere the user
        // cannot see. The view decides what to focus in that case.
        if (page->PrimaryView() == nullptr) page->GetTextPage()->setFocus();

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
      LOG_W() << "failed to restore text editor tab, title:" << title
              << ", type:" << page_type << ", recovery_type:" << recovery_type;
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
      LOG_W() << "restored text editor page is no longer in the tab widget"
              << ", restored count:" << restored_count
              << ", tab count:" << count();
    }
  }
}

auto TextEditTabWidget::generate_new_title(const QString& prefix,
                                           const QString& suffix) -> QString {
  // Never the name of a tab already open (one restored from the recovery
  // cache, say), and never handed out twice.
  QSet<QString> taken;
  for (int i = 0; i < count(); ++i) taken.insert(TabTitleOf(widget(i)));
  return NextUntitledTitle(count_page_, prefix, suffix, taken);
}

auto TextEditTabWidget::CurPage() -> QWidget* { return this->currentWidget(); }

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
  const auto effective_icon_name =
      has_explicit_icon_name ? icon_name.trimmed() : u":/icons/file.png"_s;

  const QIcon effective_icon =
      has_explicit_icon_name ? QIcon(effective_icon_name) : icon;

  auto* page = new PlainTextEditorPage(file_path);
  // Never reused, so a stale id finds nothing instead of the wrong document.
  static qint64 next_document_id = 1;
  page->setProperty("gf_document_id", next_document_id++);
  page->setProperty("type", "text");
  page->setProperty("base_title", clean_title);
  page->setProperty("icon_name", effective_icon_name);

  QPointer<PlainTextEditorPage> guarded(page);
  connect(page->GetTextPage()->document(), &QTextDocument::modificationChanged,
          page, [guarded](bool) {
            if (!guarded.isNull()) {
              Lua::LuaPlacements::Notify("document.state_changed", guarded);
            }
          });

  const int index = addTab(page, effective_icon, clean_title);
  setCurrentIndex(index);

  Module::TriggerEvent("TAB_CREATED",
                       {{"tab_index", GFBuffer{QString::number(index)}},
                        {"tab_type", GFBuffer{QString("text")}},
                        {"tab_title", GFBuffer{clean_title}},
                        {"file_path", GFBuffer{file_path}}});

  // A document that came from a file is a different fact from a tab that was
  // created, and the two coincide only here.
  if (!file_path.isEmpty()) {
    Module::TriggerEvent("DOCUMENT_OPENED",
                         {{"file_path", GFBuffer{file_path}},
                          {"tab_index", GFBuffer{QString::number(index)}}});
  }

  connect(page->GetTextPage()->document(), &QTextDocument::modificationChanged,
          this, [this, page](bool modified) {
            RefreshTabLabels();

            // On the clean-to-dirty TRANSITION, deliberately, rather than on
            // contentsChanged. A module is being told "this document now has
            // unsaved work", which is a state change and happens once;
            // contentsChanged fires per keystroke, and an event posted to the
            // module task runner per keystroke is a different feature with a
            // different cost.
            if (modified) {
              Module::TriggerEvent(
                  "DOCUMENT_CHANGED",
                  {{"file_path", GFBuffer{page->GetFilePath()}},
                   {"tab_index", GFBuffer{QString::number(indexOf(page))}}});
            }

            // A mounted view keeps its edits to itself until something asks
            // for them, so the document's own contentsChanged never fires and
            // the recovery cache below would never learn the tab has unsaved
            // work in it. This is the only notice that arrives.
            if (modified) schedule_recovery_cache(page);
          });

  connect(page->GetTextPage()->document(), &QTextDocument::contentsChanged,
          this, [this, page]() -> void { schedule_recovery_cache(page); });
  // A mounted view's later edits never touch the document until it is asked
  // for them, so they are announced here; the cache timer debounces them.
  connect(page, &PlainTextEditorPage::SignalPrimaryViewEdited, this,
          [this, page]() -> void { schedule_recovery_cache(page); });
  // Only a module-persisted view saves by itself, and says so here.
  connect(page, &PlainTextEditorPage::SignalModuleSaved, this,
          [this, page](const QString& /*path*/, const QString& title) {
            // The path is the page's by now; the title is the tab's.
            SetTabTitle(page, title);
          });

  connect(page, &PlainTextEditorPage::SignalTextDirectionModeChanged, this,
          &TextEditTabWidget::SignalTextDirectionModeChanged);

  connect(page, &PlainTextEditorPage::SignalCryptoOperationRequested, this,
          [this, page](const QString& operation) {
            // The menu-bar operations always act on the current tab, so only
            // the tab the user is actually looking at may drive them. A button
            // clicked on a background tab would otherwise encrypt or decrypt
            // whatever happens to be in front.
            if (currentWidget() != page) return;
            emit SignalCryptoOperationRequested(operation);
          });

  connect(page, &PlainTextEditorPage::SignalCryptoOperationsChanged, this,
          [this, page]() {
            // Same reason as above: the menu bar describes the tab in front.
            if (currentWidget() != page) return;
            emit SignalCryptoOperationsChanged();
          });

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
      SettingItems::kRestoreTextEditorPage.Read(GetSettings());
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
      SettingItems::kRestoreTextEditorPage.Read(GetSettings());

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

auto TextEditTabWidget::PageForDocument(qint64 id) -> QWidget* {
  if (id == 0) return currentWidget();
  for (int i = 0; i < count(); ++i) {
    if (DocumentIdOf(widget(i)) == id) return widget(i);
  }
  return nullptr;
}

auto TextEditTabWidget::DocumentIdOf(const QWidget* page) -> qint64 {
  if (page == nullptr) return 0;
  const auto v = page->property("gf_document_id");
  return v.isValid() ? v.toLongLong() : 0;
}

auto TextEditTabWidget::TabIdOf(const QWidget* page) -> quint64 {
  if (page == nullptr) return 0;
  const auto v = page->property("gf_tab_id");
  return v.isValid() ? v.toULongLong() : 0;
}

auto TextEditTabWidget::PageForTab(quint64 id) const -> QWidget* {
  if (id == 0) return nullptr;
  for (int i = 0; i < count(); ++i) {
    if (TabIdOf(widget(i)) == id) return widget(i);
  }
  return nullptr;
}

auto TextEditTabWidget::TabIds() const -> QList<quint64> {
  QList<quint64> ids;
  ids.reserve(count());
  for (int i = 0; i < count(); ++i) ids.append(TabIdOf(widget(i)));
  return ids;
}

auto TextEditTabWidget::MruTabIds() const -> QList<quint64> {
  auto ids = mru_.Order();
  for (const auto id : TabIds()) {
    if (!ids.contains(id)) ids.append(id);
  }
  return ids;
}

auto TextEditTabWidget::TabTitleOf(const QWidget* page) -> QString {
  if (page == nullptr) return {};
  return page->property("base_title").toString().trimmed();
}

auto TextEditTabWidget::IsPageModified(const QWidget* page) -> bool {
  auto* text_page =
      qobject_cast<PlainTextEditorPage*>(const_cast<QWidget*>(page));
  if (text_page == nullptr) return false;
  auto* edit = text_page->GetTextPage();
  return edit != nullptr && edit->document() != nullptr &&
         edit->document()->isModified();
}

void TextEditTabWidget::SetTabTitle(QWidget* page, const QString& title) {
  if (page == nullptr) return;
  page->setProperty("base_title", NormalizeTabTitle(title));
  RefreshTabLabels();
}

auto TextEditTabWidget::TabLabel(int index) const -> QString {
  // A tab's text is what it shows, with '&' doubled so it is not taken for a
  // mnemonic; this is the text before that.
  return tabText(index).replace(u"&&"_s, u"&"_s);
}

void TextEditTabWidget::RefreshTabLabels() {
  QStringList titles;
  QStringList paths;
  for (int i = 0; i < count(); ++i) {
    titles.append(TabTitleOf(widget(i)));
    paths.append(PathForTab(i));
  }
  const auto suffixes = DisambiguatingSuffixes(titles, paths);

  for (int i = 0; i < count(); ++i) {
    auto* page = widget(i);
    const bool modified = IsPageModified(page);

    auto label = titles[i];
    if (!suffixes[i].isEmpty()) {
      label += u" · "_s + QDir::toNativeSeparators(suffixes[i]);
    }
    label.replace(u'&', u"&&"_s);
    if (tabText(i) != label) setTabText(i, label);

    QStringList tip{titles[i]};
    if (!paths[i].isEmpty()) {
      tip.append(QDir::toNativeSeparators(paths[i]));
    } else if (qobject_cast<PlainTextEditorPage*>(page) != nullptr) {
      tip.append(tr("Not saved to a file"));
    }
    if (modified) tip.append(tr("Unsaved changes"));
    setTabToolTip(i, tip.join(u'\n'));

    if (bar_ != nullptr) bar_->SetTabModified(i, modified);
  }
}

auto TextEditTabWidget::Bar() const -> EditorTabBar* { return bar_; }

void TextEditTabWidget::tabInserted(int index) {
  QTabWidget::tabInserted(index);
  if (auto* page = widget(index); page != nullptr && TabIdOf(page) == 0) {
    page->setProperty("gf_tab_id", next_tab_id_++);
  }
  RefreshTabLabels();
  emit SignalTabCountChanged(count());
}

void TextEditTabWidget::tabRemoved(int index) {
  QTabWidget::tabRemoved(index);
  mru_.Retain(TabIds());
  RefreshTabLabels();
  emit SignalTabCountChanged(count());
}

void TextEditTabWidget::mouseDoubleClickEvent(QMouseEvent* event) {
  // The strip's row beyond its last tab belongs to this widget, not the bar.
  const auto pos = event->position().toPoint();
  const auto bar_rect = tabBar()->geometry();
  if (event->button() == Qt::LeftButton && pos.y() >= bar_rect.top() &&
      pos.y() <= bar_rect.bottom() && !bar_rect.contains(pos)) {
    SlotNewPlainTextTab();
    event->accept();
    return;
  }
  QTabWidget::mouseDoubleClickEvent(event);
}

void TextEditTabWidget::paintEvent(QPaintEvent* /*event*/) {
  // In document mode QTabWidget paints nothing but a tab bar base behind each
  // corner widget, at the corner's own height, which broke the strip's line
  // in two. Instead: one rule under the whole strip, the open-tabs button
  // included, on the bar's last row. The inactive tabs are transparent and
  // show it; the active one covers it and so reads as part of its page.
  if (count() == 0) return;
  QPainter p(this);
  p.setPen(palette().color(QPalette::Mid));
  const int y = tabBar()->geometry().bottom();
  p.drawLine(0, y, width(), y);
}

void TextEditTabWidget::show_tab_menu(int index, const QPoint& pos) {
  const auto ids = TabIds();
  QList<bool> modified;
  for (int i = 0; i < count(); ++i) modified.append(IsPageModified(widget(i)));

  QMenu menu(this);
  QHash<QAction*, QList<quint64>> closes;
  const auto add_close = [&](const QString& text, TabCloseScope scope) {
    QList<quint64> targets;
    for (const auto i : TabsToClose(scope, index, modified)) {
      targets.append(ids[i]);
    }
    auto* act = menu.addAction(text);
    act->setEnabled(!targets.isEmpty());
    closes.insert(act, targets);
  };

  add_close(tr("Close"), TabCloseScope::kTHIS);
  add_close(tr("Close Others"), TabCloseScope::kOTHERS);
  add_close(tr("Close to the Right"), TabCloseScope::kRIGHT);
  add_close(tr("Close to the Left"), TabCloseScope::kLEFT);
  add_close(tr("Close Unmodified"), TabCloseScope::kUNMODIFIED);
  add_close(tr("Close All"), TabCloseScope::kALL);
  menu.addSeparator();

  const auto title = TabTitleOf(widget(index));
  const auto tab_path = PathForTab(index);
  auto* copy_name_act = menu.addAction(tr("Copy Name"));
  copy_name_act->setEnabled(!title.isEmpty());
  auto* copy_path_act = menu.addAction(tr("Copy Path"));
  copy_path_act->setEnabled(!tab_path.isEmpty());
  auto* reveal_act = menu.addAction(tr("Reveal in File Browser"));
  reveal_act->setEnabled(!tab_path.isEmpty());

  // Acted on once the menu is gone, so a save prompt never opens over it.
  const QPointer<QWidget> page = widget(index);
  auto* selected = menu.exec(tabBar()->mapToGlobal(pos));
  if (selected == nullptr) return;

  if (closes.contains(selected)) {
    emit SignalCloseTabsRequested(closes.value(selected));
  } else if (selected == copy_name_act) {
    QGuiApplication::clipboard()->setText(title);
  } else if (selected == copy_path_act) {
    QGuiApplication::clipboard()->setText(QDir::toNativeSeparators(tab_path));
  } else if (selected == reveal_act && !page.isNull()) {
    const QFileInfo info(tab_path);
    const auto reveal_path =
        info.isFile() ? info.absolutePath() : info.absoluteFilePath();
    QDesktopServices::openUrl(QUrl::fromLocalFile(reveal_path));
  }
}

void TextEditTabWidget::StepSwitchSession(int delta) {
  // A dialog has the keyboard: whatever walk there was is over.
  if (QApplication::activeModalWidget() != nullptr) {
    EndSwitchSession();
    return;
  }

  if (!switch_session_.Active()) {
    switch_session_.Begin(MruTabIds());
    qApp->installEventFilter(this);
  }

  const auto target = switch_session_.Step(
      delta, [this](quint64 id) { return PageForTab(id) != nullptr; });
  if (target.has_value()) setCurrentWidget(PageForTab(*target));
}

void TextEditTabWidget::EndSwitchSession() {
  if (!switch_session_.Active()) return;
  qApp->removeEventFilter(this);

  const auto ended_on = switch_session_.End();
  auto* page = ended_on.has_value() ? PageForTab(*ended_on) : nullptr;
  if (page == nullptr) page = currentWidget();
  if (page != nullptr) mru_.Touch(TabIdOf(page));
}

auto TextEditTabWidget::SwitchSessionActive() const -> bool {
  return switch_session_.Active();
}

auto TextEditTabWidget::eventFilter(QObject* watched, QEvent* event) -> bool {
  // Installed on the application only while a Ctrl+Tab session runs.
  if (switch_session_.Active()) {
    switch (event->type()) {
      case QEvent::KeyRelease: {
        const auto key = static_cast<QKeyEvent*>(event)->key();
        if (key == Qt::Key_Control || key == Qt::Key_Meta) EndSwitchSession();
        break;
      }
      case QEvent::ApplicationStateChange:
        if (QGuiApplication::applicationState() != Qt::ApplicationActive) {
          EndSwitchSession();
        }
        break;
      case QEvent::WindowDeactivate:
        if (watched == window()) EndSwitchSession();
        break;
      case QEvent::MouseButtonPress:
        EndSwitchSession();
        break;
      default:
        break;
    }
  }
  return QTabWidget::eventFilter(watched, event);
}

}  // namespace GpgFrontend::UI
