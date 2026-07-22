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

#include "ui/widgets/FileTreeItemDelegate.h"

#include "ui/UserInterfaceUtils.h"

namespace {

auto BlendColor(const QColor& front, const QColor& back, double ratio)
    -> QColor {
  return QColor::fromRgbF(
      (front.redF() * ratio) + (back.redF() * (1.0 - ratio)),
      (front.greenF() * ratio) + (back.greenF() * (1.0 - ratio)),
      (front.blueF() * ratio) + (back.blueF() * (1.0 - ratio)));
}

auto ItemStyle(const QStyleOptionViewItem& option) -> QStyle* {
  return option.widget != nullptr ? option.widget->style()
                                  : QApplication::style();
}

}  // namespace

namespace GpgFrontend::UI {

FileTreeItemDelegate::FileTreeItemDelegate(QObject* parent)
    : QStyledItemDelegate(parent) {}

void FileTreeItemDelegate::paint(QPainter* painter,
                                 const QStyleOptionViewItem& option,
                                 const QModelIndex& index) const {
  QStyleOptionViewItem opt = option;
  initStyleOption(&opt, index);

  if (index.column() > 0) {
    // A selected row is carried by the highlight colours and must keep its
    // full contrast; only unselected metadata is allowed to recede.
    if ((opt.state & QStyle::State_Selected) == 0) mute_text_colors(opt);

    // Sizes are numbers and compare by their last digit, so they line up on
    // the right edge of their column.
    if (index.column() == 1) {
      opt.displayAlignment = Qt::AlignRight | Qt::AlignVCenter;
    }
  }

  ItemStyle(opt)->drawControl(QStyle::CE_ItemViewItem, &opt, painter,
                              opt.widget);

  if (index.column() != 0) return;

  const auto* model = qobject_cast<const QFileSystemModel*>(index.model());
  if (model == nullptr) return;

  paint_openpgp_badge(painter, opt, model->fileInfo(index));
}

auto FileTreeItemDelegate::sizeHint(const QStyleOptionViewItem& option,
                                    const QModelIndex& index) const -> QSize {
  auto size = QStyledItemDelegate::sizeHint(option, index);
  size.setHeight(qMax(size.height() + kRowPadding, kMinimumRowHeight));
  return size;
}

auto FileTreeItemDelegate::openpgp_badge_kind(const QFileInfo& info)
    -> FileTreeItemDelegate::BadgeKind {
  if (!info.isFile()) return BadgeKind::kNONE;
  if (IsOpenPGPSignatureFile(info)) return BadgeKind::kSIGNATURE;
  if (IsOpenPGPMessageFile(info)) return BadgeKind::kENCRYPTED;
  return BadgeKind::kNONE;
}

void FileTreeItemDelegate::paint_openpgp_badge(
    QPainter* painter, const QStyleOptionViewItem& option,
    const QFileInfo& info) {
  const auto kind = openpgp_badge_kind(info);
  if (kind == BadgeKind::kNONE) return;

  const bool signature = kind == BadgeKind::kSIGNATURE;

  const auto label = signature
                         //: Badge on a detached OpenPGP signature file. Keep
                         //: it to about three characters.
                         ? tr("SIG")
                         //: Badge on an encrypted OpenPGP file. Keep it to
                         //: about three characters.
                         : tr("ENC");

  // The badge trails the file name rather than sitting on the file icon:
  // platforms without an installed icon theme give every row an empty
  // decoration rect, and a badge anchored there would never be drawn.
  const auto text_rect = ItemStyle(option)->subElementRect(
      QStyle::SE_ItemViewItemText, &option, option.widget);
  if (text_rect.isEmpty()) return;

  // A bare coloured shape says something is different about the file but not
  // what, so the badge carries its meaning as text.
  auto badge_font = option.font;
  badge_font.setPointSizeF(
      qMax(kBadgeMinPointSize, badge_font.pointSizeF() * kBadgeFontRatio));
  badge_font.setBold(true);

  const QFontMetrics badge_metrics(badge_font);

  const auto badge_width =
      badge_metrics.horizontalAdvance(label) + (2 * kBadgePaddingX);
  const auto badge_height = badge_metrics.height() + kBadgePaddingY;

  const auto name_width = qMin(
      option.fontMetrics.horizontalAdvance(option.text), text_rect.width());

  auto badge_left = text_rect.left() + name_width + kBadgeTextGap;
  if (badge_left + badge_width > text_rect.right()) {
    badge_left = text_rect.right() - badge_width;
  }
  if (badge_left < text_rect.left()) return;

  const QRectF badge(badge_left,
                     text_rect.center().y() - (badge_height / 2.0) + 0.5,
                     badge_width, badge_height);

  const bool selected = (option.state & QStyle::State_Selected) != 0;

  // On a selected row the highlight colour is the row background, so the chip
  // swaps to the highlighted-text colour to stay visible there too.
  const auto accent = selected ? option.palette.color(QPalette::HighlightedText)
                      : signature ? QColor::fromHsv(38, 185, 205)
                                  : option.palette.color(QPalette::Highlight);

  const auto label_color =
      accent.lightnessF() > 0.6 ? QColor(0, 0, 0, 205) : QColor(255, 255, 255);

  painter->save();
  painter->setRenderHint(QPainter::Antialiasing, true);

  painter->setPen(Qt::NoPen);
  painter->setBrush(accent);
  painter->drawRoundedRect(badge, kBadgeRadius, kBadgeRadius);

  painter->setFont(badge_font);
  painter->setPen(label_color);
  painter->drawText(badge, Qt::AlignCenter, label);

  painter->restore();
}

auto FileTreeItemDelegate::helpEvent(QHelpEvent* event, QAbstractItemView* view,
                                     const QStyleOptionViewItem& option,
                                     const QModelIndex& index) -> bool {
  // The chip is small and abbreviated, so hovering it has to spell the meaning
  // out in full.
  if (event != nullptr && event->type() == QEvent::ToolTip &&
      index.column() == 0) {
    const auto* model = qobject_cast<const QFileSystemModel*>(index.model());

    if (model != nullptr) {
      switch (openpgp_badge_kind(model->fileInfo(index))) {
        case BadgeKind::kSIGNATURE:
          QToolTip::showText(event->globalPos(),
                             tr("Detached OpenPGP signature"), view);
          return true;
        case BadgeKind::kENCRYPTED:
          QToolTip::showText(event->globalPos(),
                             tr("Encrypted or armored OpenPGP file"), view);
          return true;
        case BadgeKind::kNONE:
          break;
      }
    }
  }

  return QStyledItemDelegate::helpEvent(event, view, option, index);
}

void FileTreeItemDelegate::mute_text_colors(QStyleOptionViewItem& option) {
  const auto background = option.palette.color(QPalette::Base);

  for (const auto group : {QPalette::Active, QPalette::Inactive}) {
    for (const auto role : {QPalette::Text, QPalette::WindowText}) {
      option.palette.setColor(group, role,
                              BlendColor(option.palette.color(group, role),
                                         background, kMutedTextRatio));
    }
  }
}

}  // namespace GpgFrontend::UI
