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

#include "ui/widgets/ChoiceTile.h"

#include "ui/function/UIStyle.h"

namespace GpgFrontend::UI {

namespace {

// One padding for every tile. The selected border is thicker, and the padding
// gives back the difference, so choosing a tile never moves its text.
constexpr int kPadH = 12;
constexpr int kPadV = 10;
constexpr int kBorder = 1;
constexpr int kSelectedBorder = 2;

}  // namespace

ChoiceTile::ChoiceTile(const QString& title, const QString& detail,
                       const QString& chip, QWidget* parent)
    : QFrame(parent),
      radio_(new QRadioButton(this)),
      chip_label_(new QLabel(this)),
      secondary_label_(CreateDetailLabel({}, this)) {
  setObjectName(u"GFChoiceTile"_s);
  setCursor(Qt::PointingHandCursor);
  // Never shorter than its text, so a long translation wraps instead of
  // clipping; spare height belongs to the page's stretch, not to the tile.
  setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);

  radio_->setFocusPolicy(Qt::StrongFocus);

  auto* title_label = new QLabel(title, this);
  title_label->setWordWrap(true);
  auto title_font = title_label->font();
  title_font.setBold(true);
  title_label->setFont(title_font);

  // Smaller than the title, so it reads as a note on it and not a second
  // heading; sized from the font, so it scales with the display.
  auto chip_font = chip_label_->font();
  chip_font.setPointSizeF(chip_font.pointSizeF() * 0.85);
  chip_label_->setFont(chip_font);
  chip_label_->setObjectName(u"GFChoiceTileBadge"_s);

  auto* title_row = new QHBoxLayout();
  title_row->setSpacing(8);
  title_row->addWidget(title_label, 1);
  title_row->addWidget(chip_label_, 0, Qt::AlignTop);

  auto* text = new QVBoxLayout();
  text->setSpacing(3);
  text->addLayout(title_row);
  if (!detail.isEmpty()) {
    auto* detail_label = new QLabel(detail, this);
    detail_label->setWordWrap(true);
    text->addWidget(detail_label);
  }
  text->addWidget(secondary_label_);
  secondary_label_->hide();

  // The labels would otherwise take the click for themselves, and a tile
  // that only answers to its radio button is a radio button with a border.
  for (auto* label : findChildren<QLabel*>()) {
    label->setAttribute(Qt::WA_TransparentForMouseEvents);
  }

  auto* layout = new QHBoxLayout(this);
  layout->setSpacing(10);
  layout->addWidget(radio_, 0, Qt::AlignTop);
  layout->addLayout(text, 1);

  SetChip(chip);
  connect(radio_, &QRadioButton::toggled, this, [this]() { restyle(); });
  restyle();
}

auto ChoiceTile::Radio() const -> QRadioButton* { return radio_; }

void ChoiceTile::SetChip(const QString& chip) {
  chip_label_->setText(chip);
  chip_label_->setVisible(!chip.isEmpty());
}

void ChoiceTile::SetSecondary(const QString& secondary) {
  secondary_label_->setText(secondary);
  secondary_label_->setVisible(!secondary.isEmpty());
}

void ChoiceTile::mousePressEvent(QMouseEvent* event) {
  if (isEnabled()) {
    radio_->setChecked(true);
    radio_->setFocus(Qt::MouseFocusReason);
  }
  QFrame::mousePressEvent(event);
}

void ChoiceTile::changeEvent(QEvent* event) {
  if (event->type() == QEvent::EnabledChange ||
      event->type() == QEvent::PaletteChange) {
    restyle();
  }
  QFrame::changeEvent(event);
}

void ChoiceTile::restyle() {
  // Setting a sheet sends this widget a palette change of its own, which
  // would land back here: without the guard that is a recursion to the end of
  // the stack.
  if (restyling_) return;
  restyling_ = true;

  // Palette-derived, like every card in the application, so it follows a
  // dark theme without a second set of colours. The parent's palette, not
  // this tile's: a sheet with a background rewrites the tile's own, and a
  // tile once selected would then draw its unselected border in the tint.
  const auto base =
      parentWidget() != nullptr ? parentWidget()->palette() : QPalette();
  const auto selected = radio_->isChecked() && isEnabled();
  const auto border = selected ? kSelectedBorder : kBorder;
  auto accent = base.color(QPalette::Highlight);
  auto tint = accent;
  tint.setAlpha(22);
  auto badge = accent;
  badge.setAlpha(36);

  const auto give_back = border - kBorder;
  layout()->setContentsMargins(kPadH - give_back, kPadV - give_back,
                               kPadH - give_back, kPadV - give_back);

  // The badge in the accent, as a pill; its colour does not change with the
  // selection, so it never competes with the border for "this one".
  const auto pill = chip_label_->fontMetrics().height() / 2;
  const auto sheet =
      u"QFrame#GFChoiceTile {"
      "  border: %1px solid %2;"
      "  border-radius: 8px;"
      "  background: %3;"
      "}"
      "QLabel#GFChoiceTileBadge {"
      "  color: %4;"
      "  background: %5;"
      "  border-radius: %6px;"
      "  padding: 1px 7px;"
      "}"_s.arg(border)
          .arg(selected ? accent.name() : BorderColor(base).name())
          .arg(selected ? tint.name(QColor::HexArgb) : u"transparent"_s)
          .arg((IsDarkPalette(base) ? accent.lighter(150) : accent.darker(115))
                   .name())
          .arg(badge.name(QColor::HexArgb))
          .arg(pill);
  if (sheet != sheet_) {
    sheet_ = sheet;
    setStyleSheet(sheet);
  }

  restyling_ = false;
}

}  // namespace GpgFrontend::UI
