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

#include "ui/widgets/InfoBoardDocFrame.h"
#include "ui/widgets/InfoBoardWidget.h"
#include "ui_InfoBoard.h"

namespace GpgFrontend::UI {

auto InfoBoardWidget::build_header_html(InfoBoardStatus status,
                                        const QString& subtitle,
                                        const QString& accent) const
    -> QString {
  const QString text_color = palette().color(QPalette::WindowText).name();
  const int base = font().pointSize();
  const int small_pt = std::max(7, base - 1);
  const int large_pt = base + 2;
  QString html = QStringLiteral(
                     "<span style='font-size:%3pt; color:%1; "
                     "letter-spacing:2px;'>%2</span>")
                     .arg(text_color, tr("GPGFRONTEND SECURITY REPORT"),
                          QString::number(small_pt));
  if (!subtitle.isEmpty()) {
    html +=
        QStringLiteral("<br/><span style='font-size:%3pt; color:%1;'>%2</span>")
            .arg(text_color, subtitle, QString::number(base));
  }
  const QString symbol = build_status_symbol(status);
  html += QStringLiteral(
              "<br/><br/><span style='font-size:%3pt; font-weight:bold; "
              "color:%1;'>%2</span>")
              .arg(accent, symbol, QString::number(large_pt));
  return html;
}

auto InfoBoardWidget::build_card_stylesheet(const QColor& color) const
    -> QString {
  return QStringLiteral(
             "#DocResultRow { background-color: rgba(%1,%2,%3,10);"
             " border: 1px solid rgba(%1,%2,%3,35);"
             " border-left: 3px solid %4; border-radius: 4px; }")
      .arg(color.red())
      .arg(color.green())
      .arg(color.blue())
      .arg(color.name());
}

auto InfoBoardWidget::build_chip_stylesheet(const QColor& color) const
    -> QString {
  const QString chip_bg = QStringLiteral("rgba(%1,%2,%3,12)")
                              .arg(color.red())
                              .arg(color.green())
                              .arg(color.blue());
  const QString chip_border = QStringLiteral("rgba(%1,%2,%3,35)")
                                  .arg(color.red())
                                  .arg(color.green())
                                  .arg(color.blue());
  return QStringLiteral(
             "background-color: %1; border: 1px solid %2;"
             " border-radius: 4px; padding: 3px 8px;")
      .arg(chip_bg, chip_border);
}

auto InfoBoardWidget::create_card(QWidget* parent, InfoBoardStatus status) const
    -> QFrame* {
  const QColor ic = StatusColor(status);
  auto* card = new QFrame(parent);
  card->setObjectName(QStringLiteral("DocResultRow"));
  card->setStyleSheet(build_card_stylesheet(ic));
  auto* cl = new QVBoxLayout(card);
  cl->setContentsMargins(10, 6, 10, 6);
  cl->setSpacing(3);
  return card;
}

void InfoBoardWidget::add_card_header(QVBoxLayout* card_layout, QWidget* parent,
                                      InfoBoardStatus status,
                                      const QString& title) const {
  const QColor ic = StatusColor(status);
  auto* hdr = new QHBoxLayout();
  hdr->setContentsMargins(0, 0, 0, 2);
  hdr->setSpacing(6);
  auto* icon = new QLabel(parent);
  icon->setFixedSize(16, 16);
  icon->setPixmap(
      QPixmap(StatusIconPath(status))
          .scaled(14, 14, Qt::KeepAspectRatio, Qt::SmoothTransformation));
  auto* lbl = new QLabel(title, parent);
  lbl->setStyleSheet(
      QStringLiteral("color: %1; font-weight: bold;").arg(ic.name()));
  hdr->addWidget(icon, 0, Qt::AlignVCenter);
  hdr->addWidget(lbl, 1);
  card_layout->addLayout(hdr);
}

void InfoBoardWidget::add_card_field(QVBoxLayout* card_layout, QWidget* parent,
                                     const QString& key,
                                     const QString& value) const {
  if (value.isEmpty()) return;
  auto* row = new QHBoxLayout();
  row->setContentsMargins(0, 1, 0, 1);
  row->setSpacing(6);
  auto* kl = new QLabel(key, parent);
  QFont kf = kl->font();
  kf.setBold(true);
  kf.setPointSize(std::max(7, kf.pointSize() - 1));
  kl->setFont(kf);
  kl->setStyleSheet(QStringLiteral("color: palette(text);"));
  // Grow the key column with the label text (up to a max) instead of clipping.
  const int key_w =
      std::min(std::max(StyleConstants::kCardKeyWidth,
                        QFontMetrics(kf).horizontalAdvance(key) + 8),
               StyleConstants::kCardKeyMaxWidth);
  kl->setFixedWidth(key_w);
  kl->setWordWrap(true);
  kl->setAlignment(Qt::AlignRight | Qt::AlignTop);
  auto* vl = new QLabel(value, parent);
  vl->setWordWrap(true);
  vl->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
  QFont vf = vl->font();
  vf.setPointSize(std::max(7, vf.pointSize() - 1));
  vl->setFont(vf);
  vl->setStyleSheet(QStringLiteral("color: palette(text);"));
  row->addWidget(kl);
  row->addWidget(vl, 1);
  card_layout->addLayout(row);
}

auto InfoBoardWidget::create_detail_chip(QWidget* parent, const QString& text,
                                         InfoBoardStatus status) const
    -> QLabel* {
  const QColor accent = StatusColor(status);
  auto* chip = new QLabel(text, parent);
  chip->setWordWrap(true);
  chip->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
  chip->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
  chip->setStyleSheet(build_chip_stylesheet(accent));
  return chip;
}

void InfoBoardWidget::setup_status_page_layout(QVBoxLayout* page_layout) {
  page_layout->setContentsMargins(0, 0, 0, 0);
  page_layout->setSpacing(8);

  placeholder_label_ = new QLabel(
      tr("No operation yet.\nResults will appear here as a summary document."),
      ui_->page_1);
  placeholder_label_->setAlignment(Qt::AlignCenter);
  placeholder_label_->setWordWrap(true);
  placeholder_label_->setStyleSheet(
      QStringLiteral("color: palette(windowText); font-style: italic;"));

  doc_frame_ = new DocFrame(ui_->page_1);
  doc_frame_->setObjectName(QStringLiteral("DocFrame"));
}

void InfoBoardWidget::create_field_rows(QWidget* parent,
                                        QVBoxLayout* fields_layout) {
  fields_layout->setContentsMargins(0, 2, 0, 2);
  fields_layout->setSpacing(5);

  auto make_key_label = [&](const QString& text, QWidget* p) -> QLabel* {
    auto* lbl = new QLabel(text, p);
    lbl->setFixedWidth(StyleConstants::kKeyColumnWidth);
    lbl->setAlignment(Qt::AlignRight | Qt::AlignTop);
    QFont kf = lbl->font();
    kf.setBold(true);
    kf.setPointSize(std::max(7, kf.pointSize() - 1));
    lbl->setFont(kf);
    lbl->setStyleSheet(QStringLiteral("color: palette(text);"));
    return lbl;
  };

  auto make_vsep = [&](QWidget* p) -> QFrame* {
    auto* s = new QFrame(p);
    s->setFrameShape(QFrame::VLine);
    s->setFrameShadow(QFrame::Plain);
    s->setFixedWidth(1);
    s->setStyleSheet(QStringLiteral("color: palette(mid);"));
    return s;
  };

  auto make_row = [&](const QString& key_text, QFrame*& row_out,
                      QLabel*& val_out) {
    row_out = new QFrame(parent);
    auto* rl = new QHBoxLayout(row_out);
    rl->setContentsMargins(0, 0, 0, 0);
    rl->setSpacing(10);
    val_out = new QLabel(row_out);
    val_out->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    val_out->setWordWrap(true);
    val_out->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    rl->addWidget(make_key_label(key_text, row_out));
    rl->addWidget(make_vsep(row_out));
    rl->addWidget(val_out, 1);
    fields_layout->addWidget(row_out);
  };

  make_row(tr("OPERATION"), row_operation_, val_operation_);
  make_row(tr("STATUS"), row_status_, val_status_);
  make_row(tr("ENGINE"), row_engine_, val_engine_);

  // Details row
  row_details_ = new QFrame(parent);
  row_details_->setVisible(false);
  auto* details_row_layout = new QHBoxLayout(row_details_);
  details_row_layout->setContentsMargins(0, 0, 0, 0);
  details_row_layout->setSpacing(10);

  key_details_ = make_key_label(tr("DETAILS"), row_details_);

  details_container_ = new QWidget(row_details_);
  details_container_->setSizePolicy(QSizePolicy::Expanding,
                                    QSizePolicy::Minimum);
  auto* dc_layout = new QVBoxLayout(details_container_);
  dc_layout->setContentsMargins(0, 0, 0, 0);
  dc_layout->setSpacing(3);

  details_row_layout->addWidget(key_details_);
  details_row_layout->addWidget(make_vsep(row_details_));
  details_row_layout->addWidget(details_container_, 1);
  fields_layout->addWidget(row_details_);
}

void InfoBoardWidget::create_footer(QVBoxLayout* doc_layout) {
  auto* sep_bot = new QFrame(doc_frame_);
  sep_bot->setFrameShape(QFrame::HLine);
  sep_bot->setFrameShadow(QFrame::Plain);
  sep_bot->setStyleSheet(QStringLiteral("color: palette(mid);"));

  auto* footer_row = new QHBoxLayout();
  footer_row->setContentsMargins(0, 0, 0, 0);
  footer_row->setSpacing(8);

  QFont footer_font = font();
  footer_font.setPointSize(std::max(7, footer_font.pointSize() - 2));
  const QString footer_style = QStringLiteral("color: palette(text);");

  id_label_ = new QLabel(doc_frame_);
  id_label_->setFont(footer_font);
  id_label_->setStyleSheet(footer_style);
  id_label_->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

  hash_label_ = new QLabel(doc_frame_);
  hash_label_->setFont(footer_font);
  hash_label_->setStyleSheet(footer_style);
  hash_label_->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);

  time_label_ = new QLabel(doc_frame_);
  time_label_->setFont(footer_font);
  time_label_->setStyleSheet(footer_style);
  time_label_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

  footer_row->addWidget(id_label_, 1);
  footer_row->addWidget(hash_label_, 1);
  footer_row->addWidget(time_label_, 1);

  doc_layout->addWidget(sep_bot);
  doc_layout->addLayout(footer_row);
}

void InfoBoardWidget::init_status_page() {
  auto* page_layout = qobject_cast<QVBoxLayout*>(ui_->page_1->layout());
  if (page_layout == nullptr) return;

  setup_status_page_layout(page_layout);

  auto* doc_layout = new QVBoxLayout(doc_frame_);
  doc_layout->setContentsMargins(12, 10, 12, 6);
  doc_layout->setSpacing(4);

  // Header row
  auto* header_row = new QHBoxLayout();
  header_row->setContentsMargins(0, 0, 0, 0);
  header_row->setSpacing(12);

  header_label_ = new QLabel(doc_frame_);
  header_label_->setTextFormat(Qt::RichText);
  header_label_->setAlignment(Qt::AlignLeft | Qt::AlignBottom);

  stamp_label_ = new QLabel(doc_frame_);
  stamp_label_->setFixedSize(StyleConstants::kStampSize,
                             StyleConstants::kStampSize);
  stamp_label_->setAlignment(Qt::AlignCenter);

  header_row->addWidget(header_label_, 1);
  header_row->addWidget(stamp_label_, 0, Qt::AlignTop | Qt::AlignRight);

  sep_top_ = new QFrame(doc_frame_);
  sep_top_->setFrameShape(QFrame::HLine);
  sep_top_->setFrameShadow(QFrame::Plain);
  sep_top_->setStyleSheet(QStringLiteral("color: palette(mid);"));

  // Field rows
  auto* fields_widget = new QWidget(doc_frame_);
  auto* fields_layout = new QVBoxLayout(fields_widget);
  create_field_rows(fields_widget, fields_layout);

  extra_sep_ = new QFrame(doc_frame_);
  extra_sep_->setFrameShape(QFrame::HLine);
  extra_sep_->setFrameShadow(QFrame::Plain);
  extra_sep_->setStyleSheet(QStringLiteral("color: palette(mid);"));
  extra_sep_->setVisible(false);

  extra_widget_ = new QWidget(doc_frame_);
  auto* extra_layout = new QVBoxLayout(extra_widget_);
  extra_layout->setContentsMargins(0, 0, 0, 0);
  extra_layout->setSpacing(4);
  extra_widget_->setVisible(false);

  doc_layout->addLayout(header_row);
  doc_layout->addWidget(sep_top_);
  doc_layout->addWidget(fields_widget);
  doc_layout->addWidget(extra_sep_);
  doc_layout->addWidget(extra_widget_);
  doc_layout->addSpacing(4);

  create_footer(doc_layout);

  // Wrap the report card in a top-aligned container so that, when the content
  // is shorter than the viewport, the leftover space falls outside the bordered
  // DocFrame instead of stretching it.
  auto* scroll_content = new QWidget(ui_->page_1);
  auto* scroll_layout = new QVBoxLayout(scroll_content);
  scroll_layout->setContentsMargins(0, 0, 0, 0);
  scroll_layout->setSpacing(0);
  scroll_layout->addWidget(doc_frame_);
  scroll_layout->addStretch(1);

  doc_scroll_ = new QScrollArea(ui_->page_1);
  doc_scroll_->setWidgetResizable(true);
  doc_scroll_->setFrameShape(QFrame::NoFrame);
  doc_scroll_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  doc_scroll_->setWidget(scroll_content);
  doc_scroll_->setVisible(false);

  page_layout->addWidget(placeholder_label_, 1, Qt::AlignCenter);
  page_layout->addWidget(doc_scroll_, 1);
}

auto InfoBoardWidget::make_stamp_pixmap(const QColor& color,
                                        const QString& icon_path, int size)
    -> QPixmap {
  QPixmap pm(size, size);
  pm.fill(Qt::transparent);

  QPainter p(&pm);
  p.setRenderHint(QPainter::Antialiasing);

  const qreal cx = size / 2.0;
  const qreal cy = size / 2.0;
  const qreal r_outer = (size - 4) / 2.0;
  const qreal r_inner = r_outer - 6.0;

  QColor fill = color;
  fill.setAlpha(18);
  p.setBrush(fill);
  p.setPen(Qt::NoPen);
  p.drawEllipse(QPointF(cx, cy), r_outer, r_outer);

  QColor ring = color;
  ring.setAlpha(210);
  p.setBrush(Qt::NoBrush);
  p.setPen(QPen(ring, 2.5));
  p.drawEllipse(QPointF(cx, cy), r_outer, r_outer);

  QColor inner_ring = color;
  inner_ring.setAlpha(130);
  QPen dash_pen(inner_ring, 1.5);
  dash_pen.setStyle(Qt::DashLine);
  p.setPen(dash_pen);
  p.drawEllipse(QPointF(cx, cy), r_inner, r_inner);

  const int icon_sz = static_cast<int>(r_inner * 1.05);
  QPixmap icon = QPixmap(icon_path).scaled(
      icon_sz, icon_sz, Qt::KeepAspectRatio, Qt::SmoothTransformation);
  p.setOpacity(0.88);
  p.drawPixmap(static_cast<int>(cx - (icon_sz / 2.0)),
               static_cast<int>(cy - (icon_sz / 2.0)), icon);

  p.end();
  return pm;
}

void InfoBoardWidget::delete_widgets_in_layout(QLayout* layout,
                                               int start_index) {
  if (layout == nullptr) return;

  while (layout->count() > start_index) {
    QLayoutItem* item = layout->takeAt(start_index);
    if (item == nullptr) break;

    if (auto* child_layout = item->layout()) {
      delete_widgets_in_layout(child_layout, 0);
      delete child_layout;
    }

    if (auto* widget = item->widget()) {
      widget->deleteLater();
    }

    delete item;
  }
}

void InfoBoardWidget::render_cards(QVBoxLayout* layout, QWidget* parent,
                                   const QContainer<InfoBoardCard>& cards) {
  if (layout == nullptr || parent == nullptr) return;

  for (const auto& card_data : cards) {
    auto* card = create_card(parent, card_data.status);
    auto* cl = qobject_cast<QVBoxLayout*>(card->layout());
    add_card_header(cl, card, card_data.status, card_data.title);

    // Size the key column to the widest key actually present in this card so
    // long labels (e.g. "Primary Key Algorithm") are not clipped, while still
    // keeping every key in the card right-aligned to a shared column. Clamp to
    // a max so a single long key can't crowd out the value column.
    QFont kf = card->font();
    kf.setBold(true);
    kf.setPointSize(std::max(7, kf.pointSize() - 1));
    const QFontMetrics kfm(kf);
    int key_w = StyleConstants::kCardKeyWidth;
    for (const auto& field : card_data.fields) {
      if (field.second.isEmpty()) continue;
      key_w = std::max(key_w, kfm.horizontalAdvance(field.first));
    }
    // A few px of slack so a key sized exactly to its text advance does not
    // spill onto a second line.
    key_w = std::min(key_w + 8, StyleConstants::kCardKeyMaxWidth);

    for (const auto& field : card_data.fields) {
      // Skip fields with no value so the card stays compact.
      if (field.second.isEmpty()) continue;

      auto* row = new QHBoxLayout();
      row->setContentsMargins(0, 1, 0, 1);
      row->setSpacing(6);

      auto* kl = new QLabel(field.first, card);
      kl->setFont(kf);
      kl->setStyleSheet(QStringLiteral("color: palette(text);"));
      kl->setFixedWidth(key_w);
      // Wrap rather than clip the rare key that still exceeds the max column.
      kl->setWordWrap(true);
      kl->setAlignment(Qt::AlignRight | Qt::AlignTop);

      auto* vl = new QLabel(field.second, card);
      vl->setWordWrap(true);
      vl->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
      QFont vf = vl->font();
      const bool is_hash_col =
          field.first.compare(QStringLiteral("MD5"), Qt::CaseInsensitive) ==
              0 ||
          field.first.compare(QStringLiteral("SHA1"), Qt::CaseInsensitive) ==
              0 ||
          field.first.compare(QStringLiteral("SHA256"), Qt::CaseInsensitive) ==
              0 ||
          field.first.compare(QStringLiteral("SHA-256"), Qt::CaseInsensitive) ==
              0;
      if (is_hash_col) {
        vf.setFamily(QStringLiteral("Monospace"));
      }
      vf.setPointSize(std::max(7, vf.pointSize() - 1));
      vl->setFont(vf);
      vl->setStyleSheet(QStringLiteral("color: palette(text);"));

      row->addWidget(kl);
      row->addWidget(vl, 1);
      cl->addLayout(row);
    }

    layout->addWidget(card);
  }
}

}  // namespace GpgFrontend::UI
