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

#include "ModuleListView.h"

#include "core/model/SettingsObject.h"
#include "core/module/ModuleManager.h"
#include "core/struct/settings_object/ModuleSO.h"

namespace GpgFrontend::UI {

namespace {

constexpr int kCardPaddingH = 10;
constexpr int kCardPaddingV = 8;
constexpr int kDotSize = 8;
constexpr int kChipPaddingH = 5;
constexpr int kChipSpacing = 4;
constexpr int kLineSpacing = 3;

auto IsDarkPalette(const QPalette& palette) -> bool {
  return palette.color(QPalette::Base).lightness() < 128;
}

auto ActiveColor(const QPalette& palette) -> QColor {
  return IsDarkPalette(palette) ? QColor(102, 187, 106) : QColor(46, 125, 50);
}

auto DimColor(const QPalette& palette, bool selected) -> QColor {
  auto color = selected ? palette.color(QPalette::HighlightedText)
                        : palette.color(QPalette::Text);
  color.setAlpha(selected ? 200 : 140);
  return color;
}

/**
 * @brief Paint a small rounded chip and return the rect it occupied.
 */
auto PaintChip(QPainter* painter, const QPoint& top_left, const QString& text,
               const QColor& color, const QFont& font) -> QRect {
  const QFontMetrics fm(font);
  const auto width = fm.horizontalAdvance(text) + 2 * kChipPaddingH;
  const auto height = fm.height() + 2;
  const QRect rect(top_left.x(), top_left.y(), width, height);

  auto fill = color;
  fill.setAlpha(38);
  auto border = color;
  border.setAlpha(110);

  painter->save();
  painter->setRenderHint(QPainter::Antialiasing, true);
  painter->setPen(QPen(border, 1));
  painter->setBrush(fill);
  painter->drawRoundedRect(QRectF(rect).adjusted(0.5, 0.5, -0.5, -0.5), 4, 4);
  painter->setFont(font);
  painter->setPen(color);
  painter->drawText(rect, Qt::AlignCenter, text);
  painter->restore();

  return rect;
}

}  // namespace

ModuleItemDelegate::ModuleItemDelegate(QObject* parent)
    : QStyledItemDelegate(parent) {}

void ModuleItemDelegate::paint(QPainter* painter,
                               const QStyleOptionViewItem& option,
                               const QModelIndex& index) const {
  QStyleOptionViewItem opt = option;
  initStyleOption(&opt, index);
  opt.text.clear();

  const auto& palette = opt.palette;
  const auto selected = (opt.state & QStyle::State_Selected) != 0;

  painter->save();
  painter->setRenderHint(QPainter::Antialiasing, true);

  // background
  const QRect card = opt.rect.adjusted(3, 2, -3, -2);
  if (selected) {
    painter->setPen(Qt::NoPen);
    painter->setBrush(palette.color(QPalette::Highlight));
    painter->drawRoundedRect(card, 5, 5);
  } else if ((opt.state & QStyle::State_MouseOver) != 0) {
    auto hover = palette.color(QPalette::Highlight);
    hover.setAlpha(28);
    painter->setPen(Qt::NoPen);
    painter->setBrush(hover);
    painter->drawRoundedRect(card, 5, 5);
  }

  const auto active = index.data(kModuleActiveRole).toBool();
  const auto integrated = index.data(kModuleIntegratedRole).toBool();
  const auto auto_activate = index.data(kModuleAutoActivateRole).toBool();
  const auto name = index.data(kModuleNameRole).toString();
  const auto version = index.data(kModuleVersionRole).toString();
  const auto description = index.data(kModuleDescriptionRole).toString();

  const auto text_color = selected ? palette.color(QPalette::HighlightedText)
                                   : palette.color(QPalette::Text);
  const auto accent =
      active ? ActiveColor(palette) : DimColor(palette, selected);

  auto x = card.left() + kCardPaddingH;
  auto y = card.top() + kCardPaddingV;
  const auto right = card.right() - kCardPaddingH;

  QFont name_font = opt.font;
  name_font.setBold(true);
  const QFontMetrics name_fm(name_font);

  QFont small_font = opt.font;
  small_font.setPointSizeF(qMax(7.0, opt.font.pointSizeF() - 1.0));
  const QFontMetrics small_fm(small_font);

  // line 1: status dot + name + version
  const auto line_height = name_fm.height();
  const QRect dot_rect(x, y + (line_height - kDotSize) / 2, kDotSize, kDotSize);
  painter->setPen(Qt::NoPen);
  painter->setBrush(active ? accent : Qt::NoBrush);
  if (!active) {
    painter->setPen(QPen(accent, 1));
    painter->setBrush(Qt::NoBrush);
  }
  painter->drawEllipse(dot_rect);

  auto version_width = 0;
  if (!version.isEmpty()) {
    version_width = small_fm.horizontalAdvance(version) + 6;
    painter->setFont(small_font);
    painter->setPen(DimColor(palette, selected));
    painter->drawText(
        QRect(right - version_width + 6, y, version_width, line_height),
        Qt::AlignRight | Qt::AlignVCenter, version);
  }

  const auto name_x = dot_rect.right() + 8;
  painter->setFont(name_font);
  painter->setPen(text_color);
  painter->drawText(
      QRect(name_x, y, qMax(0, right - version_width - name_x), line_height),
      Qt::AlignLeft | Qt::AlignVCenter,
      name_fm.elidedText(name, Qt::ElideRight,
                         qMax(0, right - version_width - name_x)));

  y += line_height + kLineSpacing;

  // line 2: description
  if (!description.isEmpty()) {
    painter->setFont(small_font);
    painter->setPen(DimColor(palette, selected));
    painter->drawText(
        QRect(name_x, y, qMax(0, right - name_x), small_fm.height()),
        Qt::AlignLeft | Qt::AlignVCenter,
        small_fm.elidedText(description, Qt::ElideRight,
                            qMax(0, right - name_x)));
    y += small_fm.height() + kLineSpacing;
  }

  // line 3: chips
  auto chip_x = name_x;
  const auto type_color = selected ? palette.color(QPalette::HighlightedText)
                          : palette.color(QPalette::Link).isValid()
                              ? palette.color(QPalette::Link)
                              : palette.color(QPalette::Text);
  chip_x = PaintChip(painter, QPoint(chip_x, y),
                     integrated ? tr("Integrated") : tr("External"), type_color,
                     small_font)
               .right() +
           kChipSpacing + 1;

  if (auto_activate) {
    PaintChip(painter, QPoint(chip_x, y), tr("Auto"),
              selected ? palette.color(QPalette::HighlightedText)
                       : ActiveColor(palette),
              small_font);
  }

  painter->restore();
}

auto ModuleItemDelegate::sizeHint(const QStyleOptionViewItem& option,
                                  const QModelIndex& index) const -> QSize {
  QFont name_font = option.font;
  name_font.setBold(true);
  QFont small_font = option.font;
  small_font.setPointSizeF(qMax(7.0, option.font.pointSizeF() - 1.0));

  const QFontMetrics name_fm(name_font);
  const QFontMetrics small_fm(small_font);

  auto height = 2 * kCardPaddingV + name_fm.height() + kLineSpacing +
                small_fm.height() + 2 + 4;
  if (!index.data(kModuleDescriptionRole).toString().isEmpty()) {
    height += small_fm.height() + kLineSpacing;
  }

  return {200, height};
}

ModuleListProxyModel::ModuleListProxyModel(QObject* parent)
    : QSortFilterProxyModel(parent) {
  setFilterRole(kModuleSearchTextRole);
  setFilterCaseSensitivity(Qt::CaseInsensitive);
}

void ModuleListProxyModel::SetCategory(ModuleCategory category) {
  if (category_ == category) return;
  category_ = category;
  invalidateFilter();
}

auto ModuleListProxyModel::filterAcceptsRow(
    int source_row, const QModelIndex& source_parent) const -> bool {
  if (!QSortFilterProxyModel::filterAcceptsRow(source_row, source_parent)) {
    return false;
  }

  const auto index = sourceModel()->index(source_row, 0, source_parent);
  if (!index.isValid()) return false;

  switch (category_) {
    case ModuleCategory::kActive:
      return index.data(kModuleActiveRole).toBool();
    case ModuleCategory::kInactive:
      return !index.data(kModuleActiveRole).toBool();
    case ModuleCategory::kIntegrated:
      return index.data(kModuleIntegratedRole).toBool();
    case ModuleCategory::kExternal:
      return !index.data(kModuleIntegratedRole).toBool();
    case ModuleCategory::kAll:
    default:
      return true;
  }
}

auto ModuleListProxyModel::lessThan(const QModelIndex& left,
                                    const QModelIndex& right) const -> bool {
  const auto left_active = left.data(kModuleActiveRole).toBool();
  const auto right_active = right.data(kModuleActiveRole).toBool();
  if (left_active != right_active) return left_active;

  const auto left_integrated = left.data(kModuleIntegratedRole).toBool();
  const auto right_integrated = right.data(kModuleIntegratedRole).toBool();
  if (left_integrated != right_integrated) return left_integrated;

  return QString::compare(left.data(kModuleNameRole).toString(),
                          right.data(kModuleNameRole).toString(),
                          Qt::CaseInsensitive) < 0;
}

ModuleListView::ModuleListView(QWidget* parent)
    : QListView(parent),
      model_(new QStandardItemModel(this)),
      proxy_model_(new ModuleListProxyModel(this)) {
  proxy_model_->setSourceModel(model_);
  setModel(proxy_model_);
  setItemDelegate(new ModuleItemDelegate(this));

  init_view_style();
  load_module_information();
}

void ModuleListView::init_view_style() {
  setEditTriggers(QAbstractItemView::NoEditTriggers);
  setSelectionMode(QAbstractItemView::SingleSelection);
  setMouseTracking(true);
  setUniformItemSizes(true);
  setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

  // the delegate paints selection and hover itself, all from the palette
  setFrameShape(QFrame::StyledPanel);
}

void ModuleListView::currentChanged(const QModelIndex& current,
                                    const QModelIndex& previous) {
  QListView::currentChanged(current, previous);

  const auto source_index = proxy_model_->mapToSource(current);
  auto* item = model_->itemFromIndex(source_index);
  emit this->SignalSelectModule(
      item != nullptr ? item->data(kModuleIdRole).toString() : QString());
}

void ModuleListView::load_module_information() {
  auto& module_manager = Module::ModuleManager::GetInstance();
  auto module_ids = module_manager.ListAllRegisteredModuleID();

  model_->clear();

  auto active_count = 0;
  for (const auto& module_id : module_ids) {
    auto module = module_manager.SearchModule(module_id);
    if (module == nullptr) continue;

    auto integrated_module = module_manager.IsIntegratedModule(module_id);
    auto activated = module_manager.IsModuleActivated(module_id);
    auto meta_data = module->GetModuleMetaData();

    SettingsObject so(QString("module.%1.so").arg(module_id));
    ModuleSO const module_so(so);

    if (activated) active_count++;

    const auto name = meta_data.value("Name", module_id);
    const auto description = meta_data.value("Description");
    const auto author = meta_data.value("Author");

    auto* item = new QStandardItem(name);
    item->setData(module_id, kModuleIdRole);
    item->setData(name, kModuleNameRole);
    item->setData(description, kModuleDescriptionRole);
    item->setData(author, kModuleAuthorRole);
    item->setData(module->GetModuleVersion(), kModuleVersionRole);
    item->setData(integrated_module, kModuleIntegratedRole);
    item->setData(activated, kModuleActiveRole);
    item->setData(module_so.auto_activate, kModuleAutoActivateRole);
    item->setData(QStringList{name, module_id, description, author}.join(' '),
                  kModuleSearchTextRole);
    item->setToolTip(description.isEmpty() ? module_id : description);

    model_->appendRow(item);
  }

  proxy_model_->sort(0, Qt::AscendingOrder);

  emit SignalCountsChanged(static_cast<int>(module_ids.size()), active_count);
}

void ModuleListView::Refresh() {
  const auto previous = GetCurrentModuleID();

  load_module_information();

  if (!previous.isEmpty()) SelectModule(previous);
}

void ModuleListView::SelectModule(const Module::ModuleIdentifier& module_id) {
  for (auto row = 0; row < proxy_model_->rowCount(); ++row) {
    const auto index = proxy_model_->index(row, 0);
    if (index.data(kModuleIdRole).toString() == module_id) {
      setCurrentIndex(index);
      return;
    }
  }

  // selection is gone (filtered out or removed)
  setCurrentIndex({});
  emit SignalSelectModule({});
}

void ModuleListView::SetSearchFilter(const QString& text) {
  const auto previous = GetCurrentModuleID();
  proxy_model_->setFilterFixedString(text);
  if (!previous.isEmpty()) SelectModule(previous);
}

void ModuleListView::SetCategoryFilter(ModuleCategory category) {
  const auto previous = GetCurrentModuleID();
  proxy_model_->SetCategory(category);
  if (!previous.isEmpty()) SelectModule(previous);
}

auto ModuleListView::ModuleCount() const -> int { return model_->rowCount(); }

auto ModuleListView::ActiveModuleCount() const -> int {
  auto count = 0;
  for (auto row = 0; row < model_->rowCount(); ++row) {
    if (model_->item(row)->data(kModuleActiveRole).toBool()) count++;
  }
  return count;
}

auto ModuleListView::GetCurrentModuleID() -> Module::ModuleIdentifier {
  auto* item = model_->itemFromIndex(proxy_model_->mapToSource(currentIndex()));
  return item != nullptr ? item->data(kModuleIdRole).toString() : "";
}
};  // namespace GpgFrontend::UI
