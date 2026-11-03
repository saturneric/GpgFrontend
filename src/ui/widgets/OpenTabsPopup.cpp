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
#include "ui/widgets/OpenTabsPopup.h"

#include "ui/function/TabManagement.h"
#include "ui/function/UIStyle.h"
#include "ui/widgets/PlainTextEditorPage.h"
#include "ui/widgets/TextEdit.h"
#include "ui/widgets/TextEditTabWidget.h"

namespace GpgFrontend::UI {

namespace {

constexpr int kIdRole = Qt::UserRole + 1;
constexpr int kDetailRole = Qt::UserRole + 2;
constexpr int kModifiedRole = Qt::UserRole + 3;

constexpr int kPopupWidth = 460;
constexpr int kMaxVisibleRows = 14;
constexpr int kRowPadding = 6;
constexpr int kCloseArea = 22;
constexpr int kIconSize = 16;

/// One row: icon, name, where it is (muted), and at the end a dot for unsaved
/// changes, or a cross under the pointer.
class OpenTabDelegate : public QStyledItemDelegate {
 public:
  explicit OpenTabDelegate(QListWidget* list)
      : QStyledItemDelegate(list), list_(list) {}

  void paint(QPainter* p, const QStyleOptionViewItem& option,
             const QModelIndex& index) const override {
    QStyleOptionViewItem opt(option);
    initStyleOption(&opt, index);
    const auto text = opt.text;
    const auto icon = opt.icon;
    opt.text.clear();
    opt.icon = QIcon();
    auto* style = list_->style();
    style->drawControl(QStyle::CE_ItemViewItem, &opt, p, list_);

    const bool selected = (opt.state & QStyle::State_Selected) != 0;
    const auto& pal = opt.palette;
    const auto fg =
        pal.color(selected ? QPalette::HighlightedText : QPalette::Text);
    auto muted = selected ? fg : MutedTextColor(pal);

    auto r = opt.rect.adjusted(kRowPadding, 0, -kRowPadding, 0);
    if (!icon.isNull()) {
      icon.paint(p, QRect(r.left(), r.center().y() - (kIconSize / 2), kIconSize,
                          kIconSize));
    }
    r.setLeft(r.left() + kIconSize + kRowPadding);

    // The end of the row: unsaved dot, or a close cross under the pointer.
    const QRect end(r.right() - kCloseArea + 1, r.top(), kCloseArea,
                    r.height());
    r.setRight(end.left() - kRowPadding);
    const auto cursor = list_->viewport()->mapFromGlobal(QCursor::pos());
    const bool hovered = opt.rect.contains(cursor);
    p->save();
    p->setRenderHint(QPainter::Antialiasing);
    if (hovered) {
      p->setPen(QPen(fg, 1.4, Qt::SolidLine, Qt::RoundCap));
      const auto c = QRectF(end).center();
      constexpr qreal kArm = 3.5;
      p->drawLine(QPointF(c.x() - kArm, c.y() - kArm),
                  QPointF(c.x() + kArm, c.y() + kArm));
      p->drawLine(QPointF(c.x() + kArm, c.y() - kArm),
                  QPointF(c.x() - kArm, c.y() + kArm));
    } else if (index.data(kModifiedRole).toBool()) {
      p->setPen(Qt::NoPen);
      p->setBrush(fg);
      p->drawEllipse(QRectF(end).center(), 4.0, 4.0);
    }
    p->restore();

    const QFontMetrics fm(opt.font);
    const auto title = fm.elidedText(text, Qt::ElideRight, r.width());
    p->setPen(fg);
    p->drawText(r, Qt::AlignVCenter | Qt::AlignLeft, title);

    const int used = fm.horizontalAdvance(title) + (2 * kRowPadding);
    const auto detail = index.data(kDetailRole).toString();
    if (!detail.isEmpty() && used < r.width()) {
      QRect dr = r.adjusted(used, 0, 0, 0);
      p->setPen(muted);
      // The end of a path is the part that tells two apart.
      p->drawText(dr, Qt::AlignVCenter | Qt::AlignLeft,
                  fm.elidedText(detail, Qt::ElideLeft, dr.width()));
    }
  }

  [[nodiscard]] auto sizeHint(const QStyleOptionViewItem& option,
                              const QModelIndex& /*index*/) const
      -> QSize override {
    // Every row the same height, icon or not, so the list can be sized for
    // its rows before it is shown.
    return {
        option.rect.width(),
        std::max(option.fontMetrics.height(), kIconSize) + (2 * kRowPadding)};
  }

 private:
  QListWidget* list_;
};

/// Whether @p pos (viewport coordinates) is on the close cross of @p row.
auto OnCloseCross(const QRect& row, const QPoint& pos) -> bool {
  const auto r = row.adjusted(kRowPadding, 0, -kRowPadding, 0);
  return QRect(r.right() - kCloseArea + 1, r.top(), kCloseArea, r.height())
      .contains(pos);
}

/// What a row shows after the name: a file's folder, a folder's own path.
auto DetailFor(QWidget* page, const QString& path) -> QString {
  if (path.isEmpty()) return {};
  if (qobject_cast<PlainTextEditorPage*>(page) != nullptr) {
    return QDir::toNativeSeparators(QFileInfo(path).path());
  }
  return QDir::toNativeSeparators(path);
}

}  // namespace

OpenTabsPopup::OpenTabsPopup(TextEdit* edit)
    : QFrame(edit->window(), Qt::Tool | Qt::FramelessWindowHint), edit_(edit) {
  setFrameShape(QFrame::StyledPanel);
  setAttribute(Qt::WA_DeleteOnClose, false);

  search_ = new QLineEdit(this);
  search_->setPlaceholderText(tr("Search open tabs"));
  search_->setClearButtonEnabled(true);
  search_->installEventFilter(this);

  list_ = new QListWidget(this);
  list_->setItemDelegate(new OpenTabDelegate(list_));
  list_->setUniformItemSizes(true);
  list_->setMouseTracking(true);
  list_->setFrameShape(QFrame::NoFrame);
  list_->setFocusPolicy(Qt::NoFocus);
  list_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  list_->viewport()->installEventFilter(this);

  no_match_ = new QLabel(tr("No open tabs match"), this);
  no_match_->setAlignment(Qt::AlignCenter);
  no_match_->setContentsMargins(0, 12, 0, 12);
  auto pal = no_match_->palette();
  pal.setColor(QPalette::WindowText, MutedTextColor(palette()));
  no_match_->setPalette(pal);
  no_match_->hide();

  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(4, 4, 4, 4);
  layout->setSpacing(4);
  layout->addWidget(search_);
  layout->addWidget(list_);
  layout->addWidget(no_match_);

  connect(search_, &QLineEdit::textChanged, this,
          [this](const QString&) { populate(0, 0); });
  connect(list_, &QListWidget::itemClicked, this, [this](QListWidgetItem* it) {
    list_->setCurrentItem(it);
    ActivateSelected();
  });

  // Tabs opened or closed elsewhere while the list is up.
  connect(edit_->TabWidget(), &TextEditTabWidget::SignalTabCountChanged, this,
          [this]() {
            if (isVisible() && !closing_entry_) {
              populate(SelectedId(), list_->currentRow());
            }
          });
}

void OpenTabsPopup::Open() {
  if (!isVisible()) {
    const auto* focus = QApplication::focusWidget();
    restore_focus_ = (focus != nullptr && !isAncestorOf(focus))
                         ? const_cast<QWidget*>(focus)
                         : nullptr;
  }

  search_->blockSignals(true);
  search_->clear();
  search_->blockSignals(false);
  populate(TextEditTabWidget::TabIdOf(edit_->TabWidget()->currentWidget()), 0);

  place();
  show();
  raise();
  activateWindow();
  search_->setFocus();
}

auto OpenTabsPopup::ListedIds() const -> QList<quint64> {
  QList<quint64> ids;
  for (int i = 0; i < list_->count(); ++i) {
    ids.append(list_->item(i)->data(kIdRole).toULongLong());
  }
  return ids;
}

auto OpenTabsPopup::SelectedId() const -> quint64 {
  auto* item = list_->currentItem();
  return item == nullptr ? 0 : item->data(kIdRole).toULongLong();
}

void OpenTabsPopup::SetQuery(const QString& query) { search_->setText(query); }

void OpenTabsPopup::populate(quint64 select, int fallback_row) {
  auto* tabs = edit_->TabWidget();

  QList<TabListEntry> entries;
  for (const auto id : tabs->MruTabIds()) {
    auto* page = tabs->PageForTab(id);
    if (page == nullptr) continue;
    entries.append({id, TextEditTabWidget::TabTitleOf(page),
                    tabs->PathForTab(tabs->indexOf(page))});
  }

  const auto shown = FilterTabEntries(entries, search_->text());

  list_->clear();
  for (const auto& e : shown) {
    auto* page = tabs->PageForTab(e.id);
    auto* item =
        new QListWidgetItem(tabs->tabIcon(tabs->indexOf(page)), e.title, list_);
    item->setData(kIdRole, e.id);
    item->setData(kDetailRole, DetailFor(page, e.path));
    item->setData(kModifiedRole, TextEditTabWidget::IsPageModified(page));
    item->setToolTip(tabs->tabToolTip(tabs->indexOf(page)));
  }

  int row = -1;
  for (int i = 0; i < list_->count(); ++i) {
    if (list_->item(i)->data(kIdRole).toULongLong() == select) row = i;
  }
  if (row < 0 && list_->count() > 0) {
    row = std::clamp(fallback_row, 0, list_->count() - 1);
  }
  if (row >= 0) list_->setCurrentRow(row);

  const bool none = list_->count() == 0;
  list_->setVisible(!none);
  no_match_->setVisible(none);

  if (isVisible()) place();
}

void OpenTabsPopup::place() {
  auto* tabs = edit_->TabWidget();
  const int rows = std::clamp(list_->count(), 1, kMaxVisibleRows);
  const int row_height =
      list_->count() > 0 ? list_->sizeHintForRow(0) : fontMetrics().height();
  // isHidden(), not isVisible(): before the first show nothing is visible.
  const int list_height = !list_->isHidden() ? (rows * row_height) + 4
                                             : no_match_->sizeHint().height();
  QSize size(kPopupWidth, search_->sizeHint().height() + list_height + 16);

  // Below the end of the strip: under its button, or the strip's right end.
  QPoint anchor;
  if (auto* corner = tabs->cornerWidget(Qt::TopRightCorner);
      corner != nullptr && corner->isVisible()) {
    anchor = corner->mapToGlobal(QPoint(corner->width(), corner->height()));
  } else {
    anchor = tabs->mapToGlobal(QPoint(tabs->width(), tabs->tabBar()->height()));
  }
  QRect geometry(QPoint(anchor.x() - size.width(), anchor.y()), size);

  // Inside the screen the window is on, whatever it takes.
  auto* screen = tabs->screen();
  if (screen != nullptr) {
    const auto avail = screen->availableGeometry();
    geometry.setWidth(std::min(geometry.width(), avail.width()));
    geometry.setHeight(std::min(geometry.height(), avail.height()));
    geometry.moveLeft(std::clamp(geometry.left(), avail.left(),
                                 avail.right() - geometry.width() + 1));
    geometry.moveTop(std::clamp(geometry.top(), avail.top(),
                                avail.bottom() - geometry.height() + 1));
  }
  setGeometry(geometry);
}

void OpenTabsPopup::ActivateSelected() {
  auto* tabs = edit_->TabWidget();
  auto* page = tabs->PageForTab(SelectedId());
  if (page == nullptr) return;
  restore_focus_ = nullptr;
  hide();
  tabs->window()->activateWindow();
  tabs->setCurrentWidget(page);
  if (auto* text_page = qobject_cast<PlainTextEditorPage*>(page)) {
    if (auto* view = text_page->PrimaryView()) {
      view->setFocus();
    } else {
      text_page->GetTextPage()->setFocus();
    }
  } else {
    page->setFocus();
  }
}

void OpenTabsPopup::CloseSelected() { close_row(list_->currentRow()); }

void OpenTabsPopup::close_row(int row) {
  if (row < 0 || row >= list_->count()) return;
  const auto id = list_->item(row)->data(kIdRole).toULongLong();

  // A save prompt may take the focus; the list must not take that for a
  // dismissal.
  closing_entry_ = true;
  edit_->CloseTabs({id});
  closing_entry_ = false;

  if (edit_->TabWidget()->count() == 0) {
    dismiss();
    return;
  }

  // The same query, and the entry that took the closed one's place.
  populate(0, row);
  if (isVisible()) {
    raise();
    activateWindow();
    search_->setFocus();
  }
}

void OpenTabsPopup::dismiss() {
  hide();
  edit_->window()->activateWindow();
  if (!restore_focus_.isNull() && restore_focus_->isVisible()) {
    restore_focus_->setFocus();
  }
  restore_focus_ = nullptr;
}

auto OpenTabsPopup::eventFilter(QObject* watched, QEvent* event) -> bool {
  if (watched == search_) {
    // A tool window's shortcuts are its parent window's: without this, Ctrl+W
    // here would close the main window's current tab, not the selected entry.
    if (event->type() == QEvent::ShortcutOverride &&
        static_cast<QKeyEvent*>(event)->matches(QKeySequence::Close)) {
      event->accept();
      return true;
    }
    if (event->type() == QEvent::KeyPress &&
        handle_search_key(static_cast<QKeyEvent*>(event))) {
      return true;
    }
  }

  if (watched == list_->viewport() && handle_list_event(event)) return true;

  return QFrame::eventFilter(watched, event);
}

auto OpenTabsPopup::handle_search_key(QKeyEvent* key) -> bool {
  if (key->matches(QKeySequence::Close)) {
    CloseSelected();
    return true;
  }
  switch (key->key()) {
    case Qt::Key_Down:
    case Qt::Key_Up: {
      if (list_->count() == 0) return true;
      const int step = key->key() == Qt::Key_Down ? 1 : -1;
      list_->setCurrentRow((list_->currentRow() + step + list_->count()) %
                           list_->count());
      return true;
    }
    case Qt::Key_PageDown:
    case Qt::Key_PageUp:
      QApplication::sendEvent(list_, key);
      return true;
    case Qt::Key_Return:
    case Qt::Key_Enter:
      ActivateSelected();
      return true;
    case Qt::Key_Escape:
      dismiss();
      return true;
    default:
      return false;
  }
}

auto OpenTabsPopup::handle_list_event(QEvent* event) -> bool {
  switch (event->type()) {
    case QEvent::MouseMove:
    case QEvent::Leave:
      list_->viewport()->update();
      return false;
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease: {
      auto* mouse = static_cast<QMouseEvent*>(event);
      const auto pos = mouse->position().toPoint();
      auto* item = list_->itemAt(pos);
      if (item == nullptr) return false;
      const bool closes = mouse->button() == Qt::MiddleButton ||
                          (mouse->button() == Qt::LeftButton &&
                           OnCloseCross(list_->visualItemRect(item), pos));
      if (!closes) return false;
      // Acts on release; the press is eaten too, so it never counts as a
      // click that activates the row.
      if (event->type() == QEvent::MouseButtonRelease) {
        close_row(list_->row(item));
      }
      return true;
    }
    default:
      return false;
  }
}

void OpenTabsPopup::changeEvent(QEvent* event) {
  if (event->type() == QEvent::ActivationChange && !isActiveWindow() &&
      !closing_entry_ && isVisible()) {
    // Clicked away: gone, like a menu. Deferred, so a click that moves the
    // activation to one of its own children does not count.
    QTimer::singleShot(0, this, [this]() {
      if (!isActiveWindow() && !closing_entry_ && isVisible()) {
        hide();
        restore_focus_ = nullptr;
      }
    });
  }
  QFrame::changeEvent(event);
}

}  // namespace GpgFrontend::UI
