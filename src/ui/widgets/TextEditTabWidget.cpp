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

#include "core/function/GlobalSettingStation.h"
#include "core/model/CacheObject.h"
#include "ui/UISignalStation.h"
#include "ui/widgets/PlainTextEditorPage.h"
#include "widgets/FilePage.h"

namespace GpgFrontend::UI {

TextEditTabWidget::TextEditTabWidget(QWidget* parent) : QTabWidget(parent) {
  setAcceptDrops(true);
}

void TextEditTabWidget::dragEnterEvent(QDragEnterEvent* event) {
  event->acceptProposedAction();
}

void TextEditTabWidget::dropEvent(QDropEvent* event) {
  if (!event->mimeData()->hasUrls()) return;

  auto urls = event->mimeData()->urls();

  for (const auto& url : urls) {
    QString local_file = url.toLocalFile();

    QFileInfo file_info(local_file);
    if (file_info.size() > static_cast<qint64>(1024 * 1024)) {
      QMessageBox::warning(
          this, tr("File Too Large"),
          tr("The file \"%1\" is larger than 1MB and will not be opened.")
              .arg(file_info.fileName()));
      continue;
    }

    if (file_info.isFile()) {
      QFile file(local_file);
      if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, tr("File Open Error"),
                             tr("The file \"%1\" could not be opened.")
                                 .arg(file_info.fileName()));
        continue;
      }
      QByteArray file_data = file.read(1024);
      file.close();

      if (file_data.contains('\0')) {
        QMessageBox::warning(this, tr("Binary File Detected"),
                             tr("The file \"%1\" appears to be a binary file "
                                "and will not be opened.")
                                 .arg(file_info.fileName()));
        continue;
      }

      SlotOpenFile(local_file);
    }

    if (file_info.isDir()) {
      if (!file_info.isReadable()) {
        QMessageBox::warning(
            this, tr("Directory Permission Denied"),
            tr("You do not have permission to access the directory \"%1\".")
                .arg(file_info.fileName()));
        continue;
      }
      SlotOpenDirectory(file_info.absoluteFilePath());
    }
  }

  event->acceptProposedAction();
}

void TextEditTabWidget::SlotOpenFile(const QString& path) {
  QFile file(path);
  auto result = file.open(QIODevice::ReadOnly | QIODevice::Text);
  if (result) {
    auto* page = new PlainTextEditorPage(path);
    connect(page->GetTextPage()->document(),
            &QTextDocument::modificationChanged, this,
            &TextEditTabWidget::SlotShowModified);
    // connect to cache recovery fucntion
    connect(page->GetTextPage()->document(), &QTextDocument::contentsChanged,
            this, &TextEditTabWidget::slot_save_status_to_cache_for_recovery);

    QApplication::setOverrideCursor(Qt::WaitCursor);
    auto index = this->addTab(page, stripped_name(path));
    this->setTabIcon(index, QIcon(":/icons/file.png"));
    this->setCurrentIndex(this->count() - 1);
    QApplication::restoreOverrideCursor();
    page->GetTextPage()->setFocus();
    page->ReadFile();
  } else {
    QMessageBox::warning(
        this, tr("Warning"),
        tr("Cannot read file %1:\n%2.").arg(path).arg(file.errorString()));
  }

  file.close();
}
void TextEditTabWidget::SlotShowModified(bool changed) {
  // get current tab
  int index = this->currentIndex();
  QString title = this->tabText(index);

  // if changed
  if (!changed) {
    this->setTabText(index, title.remove(0, 2));
    return;
  }

  // if doc is modified now, add leading * to title,
  // otherwise remove the leading * from the title
  if (CurTextPage()->GetTextPage()->document()->isModified()) {
    this->setTabText(index, title.trimmed().prepend("* "));
  } else {
    this->setTabText(index, title.remove(0, 2));
  }
}
auto TextEditTabWidget::CurTextPage() const -> PlainTextEditorPage* {
  return qobject_cast<PlainTextEditorPage*>(this->currentWidget());
}

auto TextEditTabWidget::SlotCurPageTextEdit() -> PlainTextEditorPage* {
  auto* cur_page = qobject_cast<PlainTextEditorPage*>(this->currentWidget());
  return cur_page;
}

auto TextEditTabWidget::CurFilePage() const -> FilePage* {
  auto* cur_file_page = qobject_cast<FilePage*>(this->currentWidget());
  if (cur_file_page != nullptr) {
    return cur_file_page;
  }
  return nullptr;
}

auto TextEditTabWidget::stripped_name(const QString& full_file_name)
    -> QString {
  return QFileInfo(full_file_name).fileName();
}

void TextEditTabWidget::slot_save_status_to_cache_for_recovery() {
  if (this->text_page_data_modified_count_++ % 8 != 0) return;

  auto settings = GlobalSettingStation::GetInstance().GetSettings();

  bool restore_text_editor_page =
      settings.value("basic/restore_text_editor_page", false).toBool();
  if (!restore_text_editor_page) {
    FLOG_D("restore_text_editor_page is false, ignoring...");
    return;
  }

  int tab_count = this->count();
  std::vector<std::tuple<int, QString, QString>> unsaved_pages;

  for (int i = 0; i < tab_count; i++) {
    auto* target_page = qobject_cast<PlainTextEditorPage*>(this->widget(i));

    // if this page is no textedit, there should be nothing to save
    if (target_page == nullptr) {
      continue;
    }

    auto* document = target_page->GetTextPage()->document();
    auto tab_title = this->tabText(i);
    if (!target_page->ReadDone() || !target_page->isEnabled() ||
        !document->isModified()) {
      continue;
    }

    unsaved_pages.emplace_back(i, tab_title, document->toRawText());
  }

  CacheObject cache("editor_unsaved_pages");
  QJsonArray unsaved_page_array;
  for (const auto& page : unsaved_pages) {
    const auto [index, title, content] = page;

    QJsonObject page_json;
    page_json["index"] = index;
    page_json["title"] = title;
    page_json["content"] = content;

    unsaved_page_array.push_back(page_json);
  }

  cache.setArray(unsaved_page_array);
}

void TextEditTabWidget::SlotNewTab() {
  QString header = tr("untitled") + QString::number(++count_page_) + ".txt";

  auto* page = new PlainTextEditorPage();
  auto index = this->addTab(page, header);
  this->setTabIcon(index, QIcon(":/icons/file.png"));
  this->setCurrentIndex(this->count() - 1);
  page->GetTextPage()->setFocus();
  connect(page->GetTextPage()->document(), &QTextDocument::modificationChanged,
          this, &TextEditTabWidget::SlotShowModified);
  connect(page->GetTextPage()->document(), &QTextDocument::contentsChanged,
          this, &TextEditTabWidget::slot_save_status_to_cache_for_recovery);
}
void TextEditTabWidget::SlotNewTabWithContent(QString title,
                                              const QString& content) {
  QString header = tr("untitled") + QString::number(++count_page_) + ".txt";
  if (!title.isEmpty()) {
    // modify title
    if (!title.isEmpty() && title[0] == '*') {
      title.remove(0, 1);
    }
    // set title
    header = title;
  }

  auto* page = new PlainTextEditorPage();
  auto index = this->addTab(page, header);
  this->setTabIcon(index, QIcon(":/icons/file.png"));
  this->setCurrentIndex(this->count() - 1);
  page->GetTextPage()->setFocus();
  connect(page->GetTextPage()->document(), &QTextDocument::modificationChanged,
          this, &TextEditTabWidget::SlotShowModified);
  connect(page->GetTextPage()->document(), &QTextDocument::contentsChanged,
          this, &TextEditTabWidget::slot_save_status_to_cache_for_recovery);

  // set content with modified status
  page->GetTextPage()->document()->setPlainText(content);
}

void TextEditTabWidget::SlotOpenDirectory(const QString& target_directory) {
  auto* page = new FilePage(qobject_cast<QWidget*>(parent()), target_directory);
  auto index = this->addTab(page, QString());
  this->setTabIcon(index, QIcon(":/icons/file-browser.png"));
  this->setCurrentIndex(this->count() - 1);
  connect(page, &FilePage::SignalPathChanged, this,
          &TextEditTabWidget::slot_file_page_path_changed);
  page->SlotGoPath();
}

void TextEditTabWidget::slot_file_page_path_changed(const QString& path) {
  int index = this->currentIndex();
  QString m_path;
  QFileInfo file_info(path);
  QString t_path = file_info.absoluteFilePath();
  if (path.size() > 18) {
    m_path = t_path.mid(t_path.size() - 18, 18).prepend("...");
  } else {
    m_path = t_path;
  }
  this->setTabText(index, m_path);

  emit UISignalStation::GetInstance()
      -> SignalMainWindowlUpdateBasicalOperaMenu(0);
}

}  // namespace GpgFrontend::UI
