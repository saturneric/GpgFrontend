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

#include "ui/widgets/MetaListPanel.h"

#include "ui/dialog/help/AboutStatusInfo.h"
#include "ui/function/UIStyle.h"

namespace GpgFrontend::UI {

namespace {

/// The rows are read, not filled in, so they sit a little further apart than a
/// form's -- but only a little: this list is usually one of three on a page,
/// and padding is what turned four readings into a screenful.
constexpr int kRowPadding = 4;

/// The air between one column's widest entry and the next column's text.
constexpr int kColumnGap = 16;

/// A row icon, and the room it takes in the caption column.
constexpr int kIconWidth = 22;

/// Below this, capping the caption column would do more harm than the long
/// caption it is protecting the values from.
constexpr int kMinCaptionWidth = 120;

/**
 * @brief A list that says when its own width changed.
 *
 * The panel's resizeEvent is the wrong moment to measure: a parent is resized
 * before its children are, so the view still has its old width when the panel
 * hears about the new one -- and the caption column was being fitted to a
 * hundred pixels that no longer existed.
 *
 * No Q_OBJECT: it declares no signals or slots.
 */
class RowsView : public QTreeWidget {
 public:
  RowsView(std::function<void()> on_resize, QWidget* parent)
      : QTreeWidget(parent), on_resize_(std::move(on_resize)) {}

 protected:
  void resizeEvent(QResizeEvent* event) override {
    QTreeWidget::resizeEvent(event);
    if (on_resize_) on_resize_();
  }

 private:
  std::function<void()> on_resize_;
};

}  // namespace

auto UnverifiedRowsCaveat() -> QString {
  return QObject::tr(
      "These come from the file's unencrypted header, which anyone holding the "
      "file can change.");
}

auto MetaListNeedsCaveat(const QVector<MetaListRow>& rows) -> bool {
  return std::any_of(rows.begin(), rows.end(),
                     [](const MetaListRow& row) { return row.unverified; });
}

auto MetaListSummaryText(const QVector<MetaListRow>& rows) -> QString {
  QStringList lines;

  for (const auto& row : rows) {
    switch (row.kind) {
      case MetaRowKind::kRule:
        continue;
      case MetaRowKind::kSection:
        if (!lines.isEmpty()) lines << QString();
        lines << QStringLiteral("[%1]").arg(row.caption);
        continue;
      case MetaRowKind::kValue:
        break;
    }

    auto value = row.value;
    if (row.bytes >= 0) {
      value = value.isEmpty()
                  ? HumanSize(row.bytes)
                  : QStringLiteral("%1 %2").arg(value, HumanSize(row.bytes));
    }
    lines << QStringLiteral("%1 %2").arg(row.caption, value).trimmed();
    if (!row.detail.isEmpty()) {
      lines << QStringLiteral("    %1").arg(row.detail);
    }
  }

  if (MetaListNeedsCaveat(rows)) {
    lines << QString();
    lines << UnverifiedRowsCaveat();
  }

  return lines.join("\n");
}

auto ToMetaListRows(const QContainer<QPair<QString, QString>>& fields)
    -> QVector<MetaListRow> {
  QVector<MetaListRow> rows;
  rows.reserve(static_cast<int>(fields.size()));
  for (const auto& field : fields) {
    if (field.second.isEmpty()) continue;
    rows.append({.caption = field.first, .value = field.second});
  }
  return rows;
}

MetaListPanel::MetaListPanel(QWidget* parent) : QWidget(parent) {
  // Exactly as tall as its rows, and no taller. A list that accepts vertical
  // stretch gets handed the leftover height of whatever it sits in, and since
  // the view inside it is fixed to its contents, that leftover turns into a
  // band of empty card above and below the readings.
  setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(6);

  // A view rather than a grid of labels. Every row is then the same height and
  // every column lines up by construction, the platform draws the rows the way
  // it draws every other list in the system, and a reader can select what they
  // are looking at and copy it -- none of which a hand-built grid gave, and all
  // of which is what these lists are for.
  tree_ = new RowsView([this]() { fit_caption_column(); }, this);
  tree_->setHeaderHidden(true);
  tree_->setRootIsDecorated(false);
  tree_->setIndentation(0);
  tree_->setUniformRowHeights(false);
  tree_->setWordWrap(false);
  tree_->setFrameShape(QFrame::NoFrame);
  tree_->setSelectionMode(QAbstractItemView::ExtendedSelection);
  tree_->setSelectionBehavior(QAbstractItemView::SelectRows);
  tree_->setEditTriggers(QAbstractItemView::NoEditTriggers);
  tree_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  tree_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

  // The card behind it paints the background, so the view takes the window's
  // own colour instead of the lighter Base a list normally paints. Matched
  // rather than made transparent: a checkbox draws its own box out of Base, and
  // a transparent Base is a checkbox nobody can see.
  tree_->viewport()->setAutoFillBackground(false);
  auto blended = tree_->palette();
  blended.setColor(QPalette::Base, blended.color(QPalette::Window));
  tree_->setPalette(blended);

  layout->addWidget(tree_, 0, Qt::AlignTop);

  caveat_ = CreateDetailLabel({}, this);
  caveat_->hide();
  layout->addWidget(caveat_);

  auto* copy = new QShortcut(QKeySequence::Copy, tree_);
  copy->setContext(Qt::WidgetWithChildrenShortcut);
  connect(copy, &QShortcut::activated, this, &MetaListPanel::copy_selection);

  connect(
      tree_, &QTreeWidget::itemChanged, this,
      [this](QTreeWidgetItem* item, int column) {
        if (column != 0) return;

        const auto index = item->data(0, Qt::UserRole).toInt();
        if (index < 0 || index >= rows_.size() || !rows_.at(index).checkable)
          return;

        const auto checked = item->checkState(0) == Qt::Checked;
        if (checked == rows_[index].checked) return;

        rows_[index].checked = checked;
        emit SignalRowToggled(index, checked);
      });
}

void MetaListPanel::SetRows(const QVector<MetaListRow>& rows) {
  rows_ = rows;
  rebuild();
}

auto MetaListPanel::Rows() const -> const QVector<MetaListRow>& {
  return rows_;
}

void MetaListPanel::RefreshValues(const QVector<MetaListRow>& rows) {
  // Only the values may move. A list whose shape changed is a different list,
  // and quietly updating half of it would leave captions describing values that
  // are no longer theirs.
  if (rows.size() != rows_.size()) {
    SetRows(rows);
    return;
  }
  for (int i = 0; i < rows.size(); ++i) {
    if (rows.at(i).kind != rows_.at(i).kind ||
        rows.at(i).checkable != rows_.at(i).checkable) {
      SetRows(rows);
      return;
    }
  }

  // Writing a check state back into an item emits itemChanged, which would
  // announce a toggle nobody made. Blocked here rather than at the call site,
  // so refreshing is silent by construction.
  const QSignalBlocker block(tree_);

  for (int i = 0; i < tree_->topLevelItemCount(); ++i) {
    auto* item = tree_->topLevelItem(i);
    const auto index = item->data(0, Qt::UserRole).toInt();
    if (index < 0 || index >= rows.size()) continue;

    apply_row(item, rows.at(index));
  }

  rows_ = rows;
  fit_to_contents();
}

void MetaListPanel::apply_row(QTreeWidgetItem* item, const MetaListRow& row) {
  if (row.kind != MetaRowKind::kValue) return;

  item->setText(0, row.caption);
  item->setText(1, row.value);
  if (!row.icon.isEmpty()) item->setIcon(0, QIcon(row.icon));
  item->setText(2, row.bytes >= 0 ? HumanSize(row.bytes) : QString());

  if (row.checkable)
    item->setCheckState(0, row.checked ? Qt::Checked : Qt::Unchecked);

  const auto colour = row.danger     ? DangerColor(palette())
                      : row.degraded ? WarningColor(palette())
                      : row.dimmed   ? MutedTextColor(palette())
                                     : palette().color(QPalette::WindowText);

  auto font = tree_->font();
  font.setBold(row.emphasis);

  for (int column = 0; column < tree_->columnCount(); ++column) {
    item->setForeground(column, colour);
    item->setFont(column, font);

    // A sentence about the value, and the whole of a value the column had to
    // cut, are both one hover away -- from anywhere on the row, because the
    // caption is as good a thing to aim at as the value.
    QStringList tip;
    if (row.path || row.value.size() > 40) tip << row.value;
    if (!row.detail.isEmpty()) tip << row.detail;
    item->setToolTip(column, tip.join("\n\n"));
  }

  item->setTextAlignment(2, Qt::AlignRight | Qt::AlignVCenter);
}

void MetaListPanel::rebuild() {
  const QSignalBlocker block(tree_);

  tree_->clear();

  const auto has_size =
      std::any_of(rows_.begin(), rows_.end(),
                  [](const MetaListRow& row) { return row.bytes >= 0; });
  tree_->setColumnCount(has_size ? 3 : 2);

  // A path is cut in the middle, where the cut is obvious; a sentence is cut at
  // the end, where it reads as one. Whichever kind of value this list holds
  // more of decides, because the mode is the view's, not the row's.
  const auto paths =
      std::any_of(rows_.begin(), rows_.end(),
                  [](const MetaListRow& row) { return row.path; });
  tree_->setTextElideMode(paths ? Qt::ElideMiddle : Qt::ElideRight);

  for (int i = 0; i < rows_.size(); ++i) {
    const auto& row = rows_.at(i);

    auto* item = new QTreeWidgetItem(tree_);
    item->setData(0, Qt::UserRole, i);
    item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);

    if (row.kind == MetaRowKind::kRule) {
      item->setFlags(Qt::ItemIsEnabled);
      item->setFirstColumnSpanned(true);

      auto* rule = new QFrame(tree_);
      rule->setFrameShape(QFrame::HLine);
      rule->setFrameShadow(QFrame::Plain);
      auto rule_palette = rule->palette();
      rule_palette.setColor(QPalette::WindowText, BorderColor(rule->palette()));
      rule->setPalette(rule_palette);

      item->setSizeHint(0, QSize(0, 9));
      tree_->setItemWidget(item, 0, rule);
      continue;
    }

    if (row.kind == MetaRowKind::kSection) {
      // A heading is not a row anybody selects, and it earns a little air above
      // it: without that, the group it opens reads as one more row of the group
      // before it.
      item->setFlags(Qt::ItemIsEnabled);
      item->setFirstColumnSpanned(true);
      item->setText(0, row.caption);
      item->setForeground(0, MutedTextColor(palette()));
      item->setSizeHint(0,
                        QSize(0, tree_->fontMetrics().height() +
                                     (i > 0 ? kRowPadding * 3 : kRowPadding)));
      continue;
    }

    if (row.checkable) {
      // The row *is* its checkbox. A checkbox sitting below a list that never
      // mentions what it controls is how someone ends up unsure whether the
      // list they just read was the whole story.
      item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
    }

    apply_row(item, row);
    item->setSizeHint(0, QSize(0, tree_->fontMetrics().height() + kRowPadding));

    if (!row.link.isEmpty()) {
      // The item's own text has to go, or the view paints it underneath the
      // label and the value is drawn twice, a few pixels apart.
      item->setText(1, {});

      // Opened only when the reader clicks it; nothing is ever fetched to
      // render the row.
      auto* label = new QLabel(
          QStringLiteral("<a href=\"%1\">%2</a>").arg(row.link, row.value),
          tree_);
      label->setTextInteractionFlags(Qt::TextBrowserInteraction);
      label->setOpenExternalLinks(true);
      tree_->setItemWidget(item, 1, label);
    }

    // A fallback state keeps its reason on screen, on a quiet line under the
    // value: the whole point of marking a state degraded is that the user did
    // not ask for it and would not otherwise notice. Everything else hands its
    // sentence to the hover, so the list stays one line per reading.
    if (!row.detail.isEmpty() &&
        ShowsDetailInline(row.detail, row.degraded || row.danger)) {
      auto* detail = new QTreeWidgetItem(tree_);
      detail->setData(0, Qt::UserRole, -1);
      detail->setFlags(Qt::ItemIsEnabled);
      detail->setText(1, row.detail);
      detail->setForeground(1, MutedTextColor(palette()));
      detail->setToolTip(1, row.detail);
      detail->setSizeHint(
          0, QSize(0, tree_->fontMetrics().height() + kRowPadding));
    }
  }

  // The one thing this panel decides for itself. A claim rendered without the
  // sentence saying it is only a claim reads as a fact, so the caveat is not
  // left to the caller to remember.
  caveat_->setText(UnverifiedRowsCaveat());
  caveat_->setVisible(MetaListNeedsCaveat(rows_));

  // The columns are sized here rather than left to ResizeToContents. The header
  // is hidden, and a hidden header sizes a column to *its* contents -- which is
  // nothing -- so the caption column collapsed to a sliver and every name in
  // the list vanished with it.
  const auto metrics = tree_->fontMetrics();
  const auto box = style()->pixelMetric(QStyle::PM_IndicatorWidth) +
                   style()->pixelMetric(QStyle::PM_CheckBoxLabelSpacing);

  int captions = 0;
  int sizes = 0;
  for (const auto& row : rows_) {
    if (row.kind != MetaRowKind::kValue) continue;

    captions = std::max(captions, metrics.horizontalAdvance(row.caption) +
                                      (row.checkable ? box : 0) +
                                      (row.icon.isEmpty() ? 0 : kIconWidth));
    if (row.bytes >= 0) {
      sizes = std::max(sizes, metrics.horizontalAdvance(HumanSize(row.bytes)));
    }
  }

  auto* header = tree_->header();
  header->setStretchLastSection(false);
  header->setSectionResizeMode(0, QHeaderView::Fixed);
  header->setSectionResizeMode(1, QHeaderView::Stretch);
  if (has_size) {
    header->setSectionResizeMode(2, QHeaderView::Fixed);
    tree_->setColumnWidth(2, sizes + kColumnGap);
  }

  caption_width_ = captions + kColumnGap;
  fit_caption_column();

  // Nothing is selected to begin with. A view selects its first row on its own,
  // and a highlighted first reading looks like an answer to a question nobody
  // asked -- here it would be whichever fact happens to be listed first.
  tree_->setCurrentItem(nullptr);
  tree_->clearSelection();

  fit_to_contents();
}

void MetaListPanel::fit_caption_column() {
  // Never more than its share of the width. One long name -- "Saved state, key
  // groups and categories" -- would otherwise take the room the values need and
  // leave every value on the list cut down to a word and an ellipsis.
  const auto share = static_cast<int>(tree_->viewport()->width() * 0.45);
  const auto width = share > kMinCaptionWidth ? std::min(caption_width_, share)
                                              : caption_width_;

  tree_->setColumnWidth(0, width);
}

void MetaListPanel::fit_to_contents() {
  int height = 0;
  for (int i = 0; i < tree_->topLevelItemCount(); ++i) {
    height += tree_->sizeHintForRow(i);
  }

  // Plus a hair, so the last row is not clipped by a rounding difference
  // between the view's idea of a row and the style's.
  tree_->setFixedHeight(height + 4);
}

void MetaListPanel::copy_selection() const {
  QVector<MetaListRow> selected;
  for (auto* item : tree_->selectedItems()) {
    const auto index = item->data(0, Qt::UserRole).toInt();
    if (index >= 0 && index < rows_.size()) selected.append(rows_.at(index));
  }

  // Copying nothing in particular copies the list, which is what somebody
  // pressing Ctrl+C on a page of readings almost always means.
  const auto text = MetaListSummaryText(selected.isEmpty() ? rows_ : selected);
  if (!text.isEmpty()) QGuiApplication::clipboard()->setText(text);
}

}  // namespace GpgFrontend::UI
