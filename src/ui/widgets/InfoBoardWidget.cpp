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

#include "ui/widgets/InfoBoardWidget.h"

#include "core/function/result_analyse/GpgOpResultInfo.h"
#include "core/model/SettingsObject.h"
#include "ui/UISignalStation.h"
#include "ui/struct/settings_object/AppearanceSO.h"
#include "ui_InfoBoard.h"

namespace {

// Internal QFrame subclass that paints a logo watermark behind children.
class DocFrame final : public QFrame {
 public:
  using QFrame::QFrame;

  void SetWatermark(const QString& /*text*/, const QColor& /*color*/) {
    watermark_active_ = true;
    if (watermark_pixmap_.isNull()) {
      watermark_pixmap_ =
          QPixmap(QStringLiteral(":/icons/gpgfrontend_logo.png"));
    }
    update();
  }

  void ClearWatermark() {
    watermark_active_ = false;
    update();
  }

 protected:
  void paintEvent(QPaintEvent* ev) override {
    QFrame::paintEvent(ev);
    if (!watermark_active_ || watermark_pixmap_.isNull()) return;

    constexpr int kWatermarkSize = 320;
    const QPixmap scaled =
        watermark_pixmap_.scaled(kWatermarkSize, kWatermarkSize,
                                 Qt::KeepAspectRatio, Qt::SmoothTransformation);

    QPainter p(this);
    p.setRenderHint(QPainter::SmoothPixmapTransform);
    p.setOpacity(0.06);
    p.drawPixmap((width() - scaled.width()) / 2,
                 (height() - scaled.height()) / 2, scaled);
  }

 private:
  bool watermark_active_ = false;
  QPixmap watermark_pixmap_;
};

}  // namespace

namespace GpgFrontend::UI {

InfoBoardWidget::InfoBoardWidget(QWidget* parent)
    : QWidget(parent),
      ui_(GpgFrontend::SecureCreateSharedObject<Ui_InfoBoard>()) {
  ui_->setupUi(this);
  ui_->externalLayout->addStretch();

  InitUI();

  connect(ui_->copyToolButton, &QToolButton::clicked, this,
          &InfoBoardWidget::slot_copy);
  connect(ui_->saveToolButton, &QToolButton::clicked, this,
          &InfoBoardWidget::slot_save);
  connect(ui_->clearToolButton, &QToolButton::clicked, this,
          &InfoBoardWidget::SlotReset);

  connect(UISignalStation::GetInstance(),
          &UISignalStation::SignalRefreshInfoBoard, this,
          &InfoBoardWidget::SlotRefresh);
  connect(UISignalStation::GetInstance(),
          &UISignalStation::SignalRefreshInfoBoardCards, this,
          &InfoBoardWidget::SlotRefreshWithCards);
}

void InfoBoardWidget::setup_tool_buttons() {
  ui_->copyToolButton->setToolTip(tr("Copy status text"));
  ui_->saveToolButton->setToolTip(tr("Save status text to file"));
  ui_->clearToolButton->setToolTip(tr("Clear status panel"));

  for (auto* button :
       {ui_->copyToolButton, ui_->saveToolButton, ui_->clearToolButton}) {
    button->setAutoRaise(false);
    button->setFocusPolicy(Qt::NoFocus);
  }
}

void InfoBoardWidget::setup_info_board() {
  ui_->infoBoard->setReadOnly(true);
  ui_->infoBoard->setAcceptRichText(false);
  ui_->infoBoard->setUndoRedoEnabled(false);
  ui_->infoBoard->setLineWrapMode(QTextEdit::WidgetWidth);
  ui_->infoBoard->setPlaceholderText(tr("Operation status will appear here."));

  AppearanceSO appearance(SettingsObject("general_settings_state"));

  QFont info_font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
  info_font.setPointSize(appearance.info_board_font_size);
  info_font.setStyleHint(QFont::Monospace);
  info_font.setFixedPitch(true);
  ui_->infoBoard->setFont(info_font);

  setStyleSheet(R"(
QWidget#InfoBoardWidget QTextEdit {
  border: 1px solid palette(mid);
  padding: 4px;
  background: palette(base);
  selection-background-color: palette(highlight);
  selection-color: palette(highlighted-text);
}

QWidget#InfoBoardWidget QTextEdit[status="ok"],
QWidget#InfoBoardWidget QTextEdit[status="warn"],
QWidget#InfoBoardWidget QTextEdit[status="critical"],
QWidget#InfoBoardWidget QTextEdit[status="neutral"] {
  border: 1px solid palette(mid);
}
)");
}

void InfoBoardWidget::InitUI() {
  setObjectName(QStringLiteral("InfoBoardWidget"));

  setup_tool_buttons();
  setup_info_board();

  ui_->statusIndicatorLabel->setFixedSize(StyleConstants::kIndicatorSize,
                                          StyleConstants::kIndicatorSize);

  init_status_page();
  ApplyStatusStyle(kINFO_ERROR_NEUTRAL);
  UpdateActionButtons();
}

void InfoBoardWidget::UpdateActionButtons() {
  const bool has_text = !ui_->infoBoard->toPlainText().trimmed().isEmpty();
  ui_->copyToolButton->setEnabled(has_text);
  ui_->saveToolButton->setEnabled(has_text);
  ui_->clearToolButton->setEnabled(has_text);
}

auto InfoBoardWidget::StatusColor(InfoBoardStatus status) const -> QColor {
  switch (status) {
    case kINFO_ERROR_OK:
      return {46, 125, 50};
    case kINFO_ERROR_WARN:
      return {239, 108, 0};
    case kINFO_ERROR_CRITICAL:
      return {198, 40, 40};
    case kINFO_ERROR_NEUTRAL:
    default:
      return palette().text().color();
  }
}

auto InfoBoardWidget::StatusIconPath(InfoBoardStatus status) const -> QString {
  switch (status) {
    case kINFO_ERROR_OK:
      return QStringLiteral(":/icons/stamp_success.png");
    case kINFO_ERROR_WARN:
      return QStringLiteral(":/icons/stamp_warning.png");
    case kINFO_ERROR_CRITICAL:
      return QStringLiteral(":/icons/stamp_failed.png");
    case kINFO_ERROR_NEUTRAL:
    default:
      return QStringLiteral(":/icons/stamp_info.png");
  }
}

auto InfoBoardWidget::StatusDescription(InfoBoardStatus status,
                                        const QString& operation) const
    -> QString {
  const QString op = operation.trimmed();
  switch (status) {
    case kINFO_ERROR_OK:
      return op.isEmpty() ? tr("The operation completed successfully.")
                          : tr("%1 completed successfully.").arg(op);
    case kINFO_ERROR_WARN:
      return op.isEmpty()
                 ? tr("Completed with warnings — please review the details.")
                 : tr("%1 completed with warnings — please review the details.")
                       .arg(op);
    case kINFO_ERROR_CRITICAL:
      return op.isEmpty()
                 ? tr("The operation failed. See the details for more "
                      "information.")
                 : tr("%1 failed. See the details for more information.")
                       .arg(op);
    default:
      return {};
  }
}

auto InfoBoardWidget::StatusTitle(InfoBoardStatus status) const -> QString {
  switch (status) {
    case kINFO_ERROR_OK:
      return tr("Success");
    case kINFO_ERROR_WARN:
      return tr("Warning");
    case kINFO_ERROR_CRITICAL:
      return tr("Error");
    case kINFO_ERROR_NEUTRAL:
    default:
      return tr("Information");
  }
}

QString InfoBoardWidget::build_status_symbol(InfoBoardStatus status) const {
  switch (status) {
    case kINFO_ERROR_OK:
      return QStringLiteral("✓  ") + StatusTitle(status).toUpper();
    case kINFO_ERROR_WARN:
      return QStringLiteral("⚠  ") + StatusTitle(status).toUpper();
    case kINFO_ERROR_CRITICAL:
      return QStringLiteral("✗  ") + StatusTitle(status).toUpper();
    default:
      return StatusTitle(status).toUpper();
  }
}

auto InfoBoardWidget::build_header_html(InfoBoardStatus status,
                                        const QString& subtitle,
                                        const QString& accent) const
    -> QString {
  QString html = QStringLiteral(
                     "<span style='font-size:7pt; color:gray; "
                     "letter-spacing:2px;'>%1</span>")
                     .arg(tr("GPGFRONTEND SECURITY REPORT"));
  if (!subtitle.isEmpty()) {
    html += QStringLiteral(
                "<br/><span style='font-size:8pt; color:gray;'>%1</span>")
                .arg(subtitle);
  }
  const QString symbol = build_status_symbol(status);
  html += QStringLiteral(
              "<br/><br/><span style='font-size:14pt; font-weight:bold; "
              "color:%1;'>%2</span>")
              .arg(accent, symbol);
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

void InfoBoardWidget::ApplyStatusStyle(InfoBoardStatus status) {
  const QColor text_color = StatusColor(status);

  const auto status_name = [&]() -> QString {
    switch (status) {
      case kINFO_ERROR_OK:
        return QStringLiteral("ok");
      case kINFO_ERROR_WARN:
        return QStringLiteral("warn");
      case kINFO_ERROR_CRITICAL:
        return QStringLiteral("critical");
      default:
        return QStringLiteral("neutral");
    }
  }();

  ui_->infoBoard->setProperty("status", status_name);
  ui_->infoBoard->style()->unpolish(ui_->infoBoard);
  ui_->infoBoard->style()->polish(ui_->infoBoard);

  auto pal = ui_->infoBoard->palette();
  pal.setColor(QPalette::Text, text_color);
  ui_->infoBoard->setPalette(pal);

  ui_->statusIndicatorLabel->setStyleSheet(
      QStringLiteral("background-color: %1; border-radius: 7px;")
          .arg(text_color.name()));
  ui_->statusIndicatorLabel->setToolTip(StatusTitle(status));
}

void InfoBoardWidget::SetInfoBoard(const QString& text, InfoBoardStatus status,
                                   const QString& content_hash) {
  ApplyStatusStyle(status);
  ui_->infoBoard->clear();

  const auto title = StatusTitle(status);
  const auto body = text.trimmed();

  const auto final_text = body.isEmpty()
                              ? tr("[%1] No details available.").arg(title)
                              : QString("[%1] %2").arg(title, body);

  ui_->infoBoard->setPlainText(final_text);
  ui_->infoBoard->moveCursor(QTextCursor::Start);

  if (status != kINFO_ERROR_NEUTRAL && doc_frame_ != nullptr) {
    update_status_page(text, status, content_hash);
    if (!user_selected_details_tab_) {
      ui_->tabWidget->setCurrentIndex(0);
    }
  }

  UpdateActionButtons();
}

void InfoBoardWidget::SlotRefresh(const QString& text, InfoBoardStatus status) {
  SetInfoBoard(text, status);
  ui_->infoBoard->verticalScrollBar()->setValue(0);
}

void InfoBoardWidget::SetInfoBoardCards(
    const QString& text, InfoBoardStatus status,
    const QContainer<InfoBoardCard>& cards, const QString& operation,
    const QString& description, const QString& details_title,
    const QStringList& details_items, const QString& content_hash) {
  ApplyStatusStyle(status);
  ui_->infoBoard->clear();

  const auto title = StatusTitle(status);
  const auto body = text.trimmed();

  const auto final_text = body.isEmpty()
                              ? tr("[%1] No details available.").arg(title)
                              : QString("[%1] %2").arg(title, body);

  ui_->infoBoard->setPlainText(final_text);
  ui_->infoBoard->moveCursor(QTextCursor::Start);

  if (status != kINFO_ERROR_NEUTRAL && doc_frame_ != nullptr) {
    update_status_page(text, status, content_hash, operation, description,
                       cards, details_title, details_items);
    if (!user_selected_details_tab_) {
      ui_->tabWidget->setCurrentIndex(0);
    }
  }

  UpdateActionButtons();
}

void InfoBoardWidget::SlotRefreshWithCards(
    const QString& text, InfoBoardStatus status,
    const QContainer<InfoBoardCard>& cards, const QString& operation,
    const QString& description, const QString& details_title,
    const QStringList& details_items) {
  SetInfoBoardCards(text, status, cards, operation, description, details_title,
                    details_items);
  ui_->infoBoard->verticalScrollBar()->setValue(0);
}

void InfoBoardWidget::AssociateTabWidget(QTabWidget* tab) {
  text_page_ = nullptr;
  tab_widget_ = tab;
  connect(tab, &QTabWidget::tabBarClicked, this, &InfoBoardWidget::SlotReset);
  connect(tab, &QTabWidget::tabCloseRequested, this,
          &InfoBoardWidget::SlotReset);
  connect(tab, &QTabWidget::currentChanged, this,
          &InfoBoardWidget::slot_tab_changed);
  SlotReset();
}

void InfoBoardWidget::ResetOptionActionsMenu() {
  const auto buttons = findChildren<QPushButton*>();
  for (auto* button : buttons) {
    if (button->property("optionalInfoBoardAction").toBool()) {
      button->deleteLater();
    }
  }
}

void InfoBoardWidget::clear_document_fields() {
  if (val_operation_ != nullptr) val_operation_->clear();
  if (val_status_ != nullptr) val_status_->clear();
  if (val_engine_ != nullptr) val_engine_->clear();
  if (time_label_ != nullptr) time_label_->clear();
  if (id_label_ != nullptr) id_label_->clear();
  if (hash_label_ != nullptr) hash_label_->clear();
  current_id_.clear();
  current_input_hash_.clear();
  current_copy_text_.clear();

  if (doc_frame_ != nullptr)
    static_cast<DocFrame*>(doc_frame_)->ClearWatermark();

  if (row_operation_ != nullptr) row_operation_->setVisible(false);
  if (row_engine_ != nullptr) row_engine_->setVisible(false);

  if (details_container_ != nullptr) {
    if (auto* l = qobject_cast<QVBoxLayout*>(details_container_->layout())) {
      delete_widgets_in_layout(l, 0);
    }
  }
  if (key_details_ != nullptr) key_details_->setText(tr("DETAILS"));
  if (row_details_ != nullptr) row_details_->setVisible(false);
  if (sep_top_ != nullptr) sep_top_->setVisible(false);
  if (extra_sep_ != nullptr) extra_sep_->setVisible(false);

  if (extra_widget_ != nullptr) {
    if (auto* l = qobject_cast<QVBoxLayout*>(extra_widget_->layout())) {
      delete_widgets_in_layout(l, 0);
    }
    extra_widget_->setVisible(false);
  }
}

void InfoBoardWidget::reset_document_view() {
  if (doc_scroll_ != nullptr) doc_scroll_->setVisible(false);
  if (placeholder_label_ != nullptr) placeholder_label_->setVisible(true);
}

void InfoBoardWidget::SlotReset() {
  ui_->infoBoard->clear();
  ui_->infoBoard->setPlaceholderText(tr("Operation status will appear here."));
  ApplyStatusStyle(kINFO_ERROR_NEUTRAL);
  ResetOptionActionsMenu();
  UpdateActionButtons();

  if (!user_selected_details_tab_) {
    ui_->tabWidget->setCurrentIndex(0);
  }

  reset_document_view();
  clear_document_fields();
}

void InfoBoardWidget::setup_status_page_layout(QVBoxLayout* page_layout) {
  page_layout->setContentsMargins(12, 12, 12, 12);
  page_layout->setSpacing(8);

  placeholder_label_ = new QLabel(
      tr("No operation yet.\nResults will appear here as a summary document."),
      ui_->page_1);
  placeholder_label_->setAlignment(Qt::AlignCenter);
  placeholder_label_->setWordWrap(true);
  placeholder_label_->setStyleSheet(
      QStringLiteral("color: palette(mid); font-style: italic;"));

  doc_frame_ = new DocFrame(ui_->page_1);
  doc_frame_->setObjectName(QStringLiteral("DocFrame"));
  doc_frame_->setVisible(false);
}

void InfoBoardWidget::create_field_rows(QWidget* parent,
                                        QVBoxLayout* fields_layout) {
  fields_layout->setContentsMargins(0, 4, 0, 4);
  fields_layout->setSpacing(5);

  auto make_key_label = [&](const QString& text, QWidget* p) -> QLabel* {
    auto* lbl = new QLabel(text, p);
    lbl->setFixedWidth(StyleConstants::kKeyColumnWidth);
    lbl->setAlignment(Qt::AlignRight | Qt::AlignTop);
    QFont kf = lbl->font();
    kf.setBold(true);
    kf.setPointSize(std::max(7, kf.pointSize() - 1));
    lbl->setFont(kf);
    lbl->setStyleSheet(QStringLiteral("color: palette(mid);"));
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
  const QString footer_style = QStringLiteral("color: palette(mid);");

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
  doc_layout->setContentsMargins(16, 14, 16, 12);
  doc_layout->setSpacing(8);

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
  doc_layout->addStretch(1);

  create_footer(doc_layout);

  doc_scroll_ = new QScrollArea(ui_->page_1);
  doc_scroll_->setWidgetResizable(true);
  doc_scroll_->setFrameShape(QFrame::NoFrame);
  doc_scroll_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  doc_scroll_->setWidget(doc_frame_);
  doc_scroll_->setVisible(false);

  page_layout->addWidget(placeholder_label_, 1, Qt::AlignCenter);
  page_layout->addWidget(doc_scroll_, 1);

  ui_->tabWidget->setCurrentIndex(0);
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

void InfoBoardWidget::update_doc_header(InfoBoardStatus status,
                                        const QString& operation,
                                        const QString& engine,
                                        const QString& count_summary) {
  if (header_label_ == nullptr) return;

  const QColor color = StatusColor(status);
  const QString accent = color.name();

  QStringList meta;
  if (!operation.isEmpty()) meta << operation.toHtmlEscaped();
  if (!count_summary.isEmpty()) meta << count_summary.toHtmlEscaped();
  if (!engine.isEmpty()) meta << engine.toHtmlEscaped();
  const QString subtitle = meta.join(QStringLiteral("  ·  "));

  header_label_->setText(build_header_html(status, subtitle, accent));

  const QString symbol = build_status_symbol(status);
  if (val_status_ != nullptr) {
    val_status_->setText(symbol);
    val_status_->setStyleSheet(
        QStringLiteral("color: %1; font-weight: bold;").arg(accent));
  }
  if (val_operation_ != nullptr) val_operation_->setText(operation);
  if (val_engine_ != nullptr) val_engine_->setText(engine);

  if (row_status_ != nullptr) row_status_->setVisible(false);
  if (row_operation_ != nullptr) row_operation_->setVisible(false);
  if (row_engine_ != nullptr) row_engine_->setVisible(false);
}

void InfoBoardWidget::update_status_page(
    const QString& text, InfoBoardStatus status, const QString& content_hash,
    const QString& operation, const QString& description,
    const QContainer<InfoBoardCard>& cards, const QString& details_title,
    const QStringList& details_items) {
  if (doc_frame_ == nullptr) return;

  const QColor color = StatusColor(status);
  const QString icon_path = StatusIconPath(status);

  stamp_label_->setPixmap(
      make_stamp_pixmap(color, icon_path, StyleConstants::kStampSize));

  const QString accent = color.name();
  const QString border_subtle = QStringLiteral("rgba(%1,%2,%3,50)")
                                    .arg(color.red())
                                    .arg(color.green())
                                    .arg(color.blue());
  doc_frame_->setStyleSheet(
      QStringLiteral(
          "#DocFrame { border: 1px solid %1; border-left: 4px solid %2;"
          " border-radius: 6px; }")
          .arg(border_subtle, accent));

  QString resolved_operation = operation;
  QString resolved_description = description;
  if (resolved_description.isEmpty() && !resolved_operation.isEmpty())
    resolved_description = StatusDescription(status, resolved_operation);
  QContainer<InfoBoardCard> resolved_cards = cards;
  QString resolved_details_title = details_title;
  QStringList resolved_details_items = details_items;

  if (resolved_cards.isEmpty() && resolved_details_items.isEmpty()) {
    const QString hash_title = tr("File Hash Information");
    if (text.contains(hash_title)) {
      resolved_operation = hash_title;
      struct HashField {
        QString key;
        QString value;
      };
      QList<HashField> fields;
      const QStringList lines = text.split(QLatin1Char('\n'));
      for (const QString& raw_line : lines) {
        const QString line = raw_line.trimmed();
        if (!line.startsWith(QLatin1String("- "))) continue;
        const QString body = line.mid(2);
        const int colon_pos = body.indexOf(QLatin1String(": "));
        if (colon_pos < 0) continue;
        fields.append({body.left(colon_pos), body.mid(colon_pos + 2)});
      }
      if (!fields.isEmpty()) {
        InfoBoardCard card;
        card.title = hash_title;
        card.status = status;
        for (const auto& f : fields) {
          card.fields.append({f.key, f.value});
        }
        resolved_cards.append(card);
      }
    }
  }

  update_doc_header(status, resolved_operation, {}, {});

  if (auto* l = qobject_cast<QVBoxLayout*>(details_container_->layout())) {
    delete_widgets_in_layout(l, 0);
  }
  key_details_->setText(tr("DETAILS"));
  row_details_->setVisible(false);

  if (!resolved_details_items.isEmpty() || !resolved_description.isEmpty()) {
    populate_details_section_generic(resolved_details_title,
                                     resolved_details_items, status,
                                     resolved_description);
  }

  const bool has_details = row_details_ != nullptr && row_details_->isVisible();
  if (sep_top_ != nullptr) sep_top_->setVisible(has_details);

  if (auto* l = qobject_cast<QVBoxLayout*>(extra_widget_->layout())) {
    delete_widgets_in_layout(l, 0);
  }

  bool has_extra = false;
  if (!resolved_cards.isEmpty()) {
    auto* extra_layout = qobject_cast<QVBoxLayout*>(extra_widget_->layout());
    if (extra_layout != nullptr) {
      populate_extra_section_generic(extra_layout, extra_widget_,
                                     resolved_cards);
      has_extra = true;
    }
  }

  if (has_extra) {
    if (extra_sep_ != nullptr) extra_sep_->setVisible(true);
    extra_widget_->setVisible(true);
  } else {
    extra_widget_->setVisible(false);
    if (extra_sep_ != nullptr) extra_sep_->setVisible(false);
  }

  const auto now = QDateTime::currentDateTime();
  time_label_->setText(
      tr("Issued:  %1").arg(QLocale().toString(now, QLocale::ShortFormat)));

  // Build content string for hashing (all visible content)
  QString content_for_hash;
  content_for_hash += text;
  content_for_hash += resolved_operation;
  for (const auto& card : resolved_cards) {
    content_for_hash += card.title;
    for (const auto& field : card.fields) {
      content_for_hash += field.first + field.second;
    }
  }
  content_for_hash += resolved_details_title;
  content_for_hash += resolved_details_items.join(QString());
  content_for_hash += now.toString(Qt::ISODate);

  // Compute SHA-256 hash of document content
  const QByteArray hash_bytes = QCryptographicHash::hash(
      content_for_hash.toUtf8(), QCryptographicHash::Sha256);
  const QString doc_hash = QString::fromLatin1(hash_bytes.toHex());

  const QString op_abbr = [&status]() -> QString {
    switch (status) {
      case kINFO_ERROR_OK:
        return QStringLiteral("OK");
      case kINFO_ERROR_WARN:
        return QStringLiteral("WN");
      case kINFO_ERROR_CRITICAL:
        return QStringLiteral("FL");
      default:
        return QStringLiteral("GP");
    }
  }();

  // Use first 8 hex chars of document hash for REF ID
  const QString content_id = doc_hash.left(8).toUpper();

  current_id_ = QStringLiteral("REF-%1-%2-%3")
                    .arg(op_abbr)
                    .arg(now.toString(QStringLiteral("yyyyMMdd")))
                    .arg(content_id);
  id_label_->setText(current_id_);

  // Display full hash in footer
  hash_label_->setText(tr("Hash: %1").arg(doc_hash.toUpper()));

  static_cast<DocFrame*>(doc_frame_)
      ->SetWatermark(StatusTitle(status).toUpper(), StatusColor(status));

  ui_->saveToolButton->setToolTip(tr("Export certificate as PNG image"));

  placeholder_label_->setVisible(false);
  doc_scroll_->setVisible(true);

  // Build plain-text copy of document content
  {
    QStringList lines;
    if (!current_id_.isEmpty()) {
      lines << current_id_;
      lines << QString(current_id_.length(), QLatin1Char('-'));
    }
    if (!resolved_operation.isEmpty())
      lines << tr("Operation: %1").arg(resolved_operation);
    lines << tr("Status:    %1").arg(build_status_symbol(status));
    if (!content_hash.isEmpty())
      lines << tr("SHA-256:   %1").arg(content_hash);
    if (!resolved_description.isEmpty()) lines << resolved_description;
    if (!resolved_details_items.isEmpty()) {
      const QString key = resolved_details_title.isEmpty()
                              ? tr("DETAILS")
                              : resolved_details_title;
      lines << QStringLiteral("%1:  %2").arg(
          key, resolved_details_items.join(QStringLiteral(", ")));
    }
    for (const auto& card : resolved_cards) {
      lines << QString();
      lines << card.title;
      for (const auto& f : card.fields)
        lines << QStringLiteral("  %1: %2").arg(f.first, f.second);
    }
    lines << QString() << time_label_->text();
    current_copy_text_ = lines.join(QLatin1Char('\n'));
  }
}

void InfoBoardWidget::populate_details_section(
    const GpgFrontend::GpgOpResultInfo& info, InfoBoardStatus status) {
  if (info.details.isEmpty()) return;

  if (info.operation.contains(tr("Decrypt"))) {
    key_details_->setText(tr("RECIPIENT"));
  } else if (info.operation.contains(tr("Sign")) ||
             info.operation.contains(tr("Verify"))) {
    key_details_->setText(tr("SIGNER"));
  } else {
    key_details_->setText(tr("DETAILS"));
  }

  auto* dc_layout = qobject_cast<QVBoxLayout*>(details_container_->layout());
  if (dc_layout == nullptr) return;

  delete_widgets_in_layout(dc_layout, 0);

  const QColor accent = StatusColor(status);
  const QString chip_style = build_chip_stylesheet(accent);

  for (const auto& detail : info.details) {
    auto* chip = new QLabel(detail, details_container_);
    chip->setWordWrap(true);
    chip->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    chip->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    chip->setStyleSheet(chip_style);
    dc_layout->addWidget(chip);
  }
  row_details_->setVisible(true);
}

void InfoBoardWidget::populate_extra_section(
    const GpgFrontend::GpgOpResultInfo& info, InfoBoardStatus status) {
  if (extra_widget_ == nullptr) return;

  auto* l = qobject_cast<QVBoxLayout*>(extra_widget_->layout());
  if (l == nullptr) return;

  delete_widgets_in_layout(l, 0);

  const QString& desc = info.description;
  if (!desc.isEmpty()) {
    auto* desc_label = new QLabel(desc, extra_widget_);
    desc_label->setWordWrap(true);
    desc_label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    desc_label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    desc_label->setStyleSheet(
        QStringLiteral("padding: 2px 0 8px 0;"
                       " border-bottom: 1px solid palette(mid);"));
    l->addWidget(desc_label);
  }

  bool has_cards = populate_extra_from_op_info(l, extra_widget_, info);
  const bool has_any = has_cards || !desc.isEmpty();

  if (extra_sep_ != nullptr) extra_sep_->setVisible(has_any);
  extra_widget_->setVisible(has_any);
}

void InfoBoardWidget::populate_details_section_generic(
    const QString& title, const QStringList& items, InfoBoardStatus status,
    const QString& description) {
  if (items.isEmpty() && description.isEmpty()) return;

  auto* dc_layout = qobject_cast<QVBoxLayout*>(details_container_->layout());
  if (dc_layout == nullptr) return;

  delete_widgets_in_layout(dc_layout, 0);

  key_details_->setText(title.isEmpty() ? tr("DETAILS") : title);

  // Add description at the top if provided
  if (!description.isEmpty()) {
    auto* desc_label = new QLabel(description, details_container_);
    desc_label->setWordWrap(true);
    desc_label->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    desc_label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    desc_label->setStyleSheet(
        QStringLiteral("color: palette(mid); font-style: italic; padding: 4px "
                       "0; border-bottom: 1px solid palette(mid);"));
    dc_layout->addWidget(desc_label);
  }

  const QColor accent = StatusColor(status);
  const QString chip_style = build_chip_stylesheet(accent);

  for (const auto& item : items) {
    auto* chip = new QLabel(item, details_container_);
    chip->setWordWrap(true);
    chip->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    chip->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    chip->setStyleSheet(chip_style);
    dc_layout->addWidget(chip);
  }
  row_details_->setVisible(true);
}

void InfoBoardWidget::populate_extra_section_generic(
    QVBoxLayout* layout, QWidget* parent,
    const QContainer<InfoBoardCard>& cards) {
  if (layout == nullptr || parent == nullptr) return;

  delete_widgets_in_layout(layout, 0);
  render_cards(layout, parent, cards);
}

void InfoBoardWidget::SetInfoBoardWithOpInfo(
    const QString& text, InfoBoardStatus status,
    const GpgFrontend::GpgOpResultInfo& info) {
  SetInfoBoard(text, status, info.inputHash);
  if (doc_frame_ == nullptr) return;

  update_doc_header(status, info.operation, info.engine, {});
  populate_details_section(info, status);

  const bool has_details = row_details_ != nullptr && row_details_->isVisible();
  if (sep_top_ != nullptr) sep_top_->setVisible(has_details);

  populate_extra_section(info, status);

  // Rebuild copy text now that full op_info content is populated
  {
    QStringList lines;
    if (!current_id_.isEmpty()) {
      lines << current_id_;
      lines << QString(current_id_.length(), QLatin1Char('-'));
    }
    if (!info.operation.isEmpty())
      lines << tr("Operation: %1").arg(info.operation);
    lines << tr("Status:    %1").arg(build_status_symbol(status));
    if (!info.engine.isEmpty()) lines << tr("Engine:    %1").arg(info.engine);
    if (!info.inputHash.isEmpty())
      lines << tr("SHA-256:   %1").arg(info.inputHash);
    if (!info.description.isEmpty()) lines << info.description;
    if (!info.details.isEmpty())
      lines << tr("Details:   %1").arg(
          info.details.join(QStringLiteral(", ")));
    const QStringList card_lines = build_op_info_copy_lines(info);
    if (!card_lines.isEmpty()) {
      lines << QString();
      lines.append(card_lines);
    }
    if (time_label_ != nullptr && !time_label_->text().isEmpty())
      lines << QString() << time_label_->text();
    current_copy_text_ = lines.join(QLatin1Char('\n'));
  }
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

void InfoBoardWidget::SetInfoBoardFromResults(
    const QString& text, InfoBoardStatus overall_status,
    const QContainer<GpgOperaResult>& results) {
  // Use the first result's input hash for content-dependent REF generation
  QString content_hash;
  if (!results.isEmpty()) {
    content_hash = results.first().op_info.inputHash;
  }
  SetInfoBoard(text, overall_status, content_hash);
  if (doc_frame_ != nullptr && !results.isEmpty()) {
    update_status_page_from_results(overall_status, results);
  }
}

void InfoBoardWidget::update_status_page_from_results(
    InfoBoardStatus status, const QContainer<GpgOperaResult>& results) {
  if (extra_widget_ == nullptr) return;

  auto* extra_layout = qobject_cast<QVBoxLayout*>(extra_widget_->layout());
  if (extra_layout == nullptr) return;

  delete_widgets_in_layout(extra_layout, 0);

  // Aggregate op_info from all results
  QString agg_operation;
  QString agg_engine;
  QString agg_description;
  QStringList agg_details;
  for (const auto& r : results) {
    if (!r.op_info.operation.isEmpty() && agg_operation.isEmpty()) {
      agg_operation = r.op_info.operation;
    }
    if (!r.op_info.engine.isEmpty() && agg_engine.isEmpty()) {
      agg_engine = r.op_info.engine;
    }
    if (!r.op_info.description.isEmpty() && agg_description.isEmpty()) {
      agg_description = r.op_info.description;
    }
    for (const auto& d : r.op_info.details) {
      if (!agg_details.contains(d)) agg_details << d;
    }
  }

  // Details row — chips only, no description
  if (!agg_details.isEmpty()) {
    if (agg_operation == tr("Decrypt")) {
      key_details_->setText(tr("RECIPIENT"));
    } else if (agg_operation == tr("Sign") || agg_operation == tr("Verify")) {
      key_details_->setText(tr("SIGNER"));
    } else {
      key_details_->setText(tr("DETAILS"));
    }

    if (auto* dc_layout =
            qobject_cast<QVBoxLayout*>(details_container_->layout())) {
      delete_widgets_in_layout(dc_layout, 0);

      const QString chip_bg = QStringLiteral("rgba(100,100,100,10)");
      const QString chip_border = QStringLiteral("rgba(100,100,100,30)");

      for (const auto& detail : agg_details) {
        auto* chip = new QLabel(detail, details_container_);
        chip->setWordWrap(true);
        chip->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        chip->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
        chip->setStyleSheet(
            QStringLiteral("padding: 3px 8px; border-radius: 4px;"
                           " background-color: %1; border: 1px solid %2;")
                .arg(chip_bg, chip_border));
        dc_layout->addWidget(chip);
      }
    }
    row_details_->setVisible(true);
  } else {
    row_details_->setVisible(false);
  }

  const bool has_details = row_details_ != nullptr && row_details_->isVisible();
  if (sep_top_ != nullptr) sep_top_->setVisible(has_details);

  // Build count summary
  const bool has_tags =
      std::any_of(results.begin(), results.end(),
                  [](const GpgOperaResult& r) { return !r.tag.isEmpty(); });
  QString count_summary;
  if (has_tags) {
    int ok_count = 0;
    int warn_count = 0;
    int fail_count = 0;
    for (const auto& r : results) {
      if (r.op_info.status > 0) {
        ok_count++;
      } else if (r.op_info.status == 0) {
        warn_count++;
      } else {
        fail_count++;
      }
    }
    QStringList parts;
    if (ok_count > 0) {
      parts << QStringLiteral("%1 %2").arg(ok_count).arg(tr("OK"));
    }
    if (warn_count > 0) {
      parts << QStringLiteral("%1 %2").arg(warn_count).arg(tr("Warning"));
    }
    if (fail_count > 0) {
      parts << QStringLiteral("%1 %2").arg(fail_count).arg(tr("Failed"));
    }
    count_summary = parts.join(QStringLiteral(", "));
  }

  update_doc_header(status, agg_operation, agg_engine, count_summary);

  const QString agg_desc = agg_description.isEmpty()
                               ? StatusDescription(status, agg_operation)
                               : agg_description;
  if (!agg_desc.isEmpty()) {
    auto* desc_label = new QLabel(agg_desc, extra_widget_);
    desc_label->setWordWrap(true);
    desc_label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    desc_label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    desc_label->setStyleSheet(
        QStringLiteral("padding: 2px 0 8px 0;"
                       " border-bottom: 1px solid palette(mid);"));
    extra_layout->addWidget(desc_label);
  }

  // Per-result rows
  bool has_cards = false;
  int shown = 0;
  for (const auto& r : results) {
    if (shown >= StyleConstants::kMaxResultCards) {
      auto* more = new QLabel(tr("… and %1 more").arg(results.size() - shown),
                              extra_widget_);
      more->setAlignment(Qt::AlignCenter);
      QFont f = more->font();
      f.setPointSize(std::max(7, f.pointSize() - 1));
      more->setFont(f);
      more->setStyleSheet(QStringLiteral("color: palette(mid);"));
      extra_layout->addWidget(more);
      break;
    }

    InfoBoardStatus item_status = kINFO_ERROR_CRITICAL;
    if (r.op_info.status > 0) {
      item_status = kINFO_ERROR_OK;
    } else if (r.op_info.status == 0) {
      item_status = kINFO_ERROR_WARN;
    }

    if (!r.tag.isEmpty()) {
      const QColor ic = StatusColor(item_status);
      const QString icon_path = StatusIconPath(item_status);

      auto* row = new QFrame(extra_widget_);
      row->setObjectName(QStringLiteral("DocResultRow"));
      row->setStyleSheet(build_card_stylesheet(ic));

      auto* rl = new QHBoxLayout(row);
      rl->setContentsMargins(10, 5, 10, 5);
      rl->setSpacing(8);

      auto* icon_lbl = new QLabel(row);
      icon_lbl->setFixedSize(18, 18);
      icon_lbl->setPixmap(QPixmap(icon_path).scaled(14, 14, Qt::KeepAspectRatio,
                                                    Qt::SmoothTransformation));
      rl->addWidget(icon_lbl, 0, Qt::AlignVCenter);

      auto* tag_lbl = new QLabel(r.tag, row);
      tag_lbl->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
      tag_lbl->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
      QFont tf = tag_lbl->font();
      tf.setPointSize(std::max(8, tf.pointSize() - 1));
      tag_lbl->setFont(tf);
      rl->addWidget(tag_lbl, 1);

      extra_layout->addWidget(row);
    }
    if (populate_extra_from_op_info(extra_layout, extra_widget_, r.op_info)) {
      has_cards = true;
    }
    shown++;
  }

  const bool has_any = has_cards || !agg_desc.isEmpty();
  extra_sep_->setVisible(has_any);
  extra_widget_->setVisible(has_any);

  const auto now =
      QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat);
  time_label_->setText(tr("Issued:  %1").arg(now));

  // Build plain-text copy of document content
  {
    QStringList lines;
    if (!current_id_.isEmpty()) {
      lines << current_id_;
      lines << QString(current_id_.length(), QLatin1Char('-'));
    }
    if (!agg_operation.isEmpty())
      lines << tr("Operation: %1").arg(agg_operation);
    lines << tr("Status:    %1").arg(build_status_symbol(status));
    if (!agg_engine.isEmpty()) lines << tr("Engine:    %1").arg(agg_engine);
    if (!count_summary.isEmpty()) lines << count_summary;
    if (!agg_desc.isEmpty()) lines << agg_desc;
    if (!agg_details.isEmpty())
      lines << tr("Details:   %1").arg(
          agg_details.join(QStringLiteral(", ")));
    for (const auto& r : results) {
      QStringList card_lines;
      if (!r.op_info.inputHash.isEmpty())
        card_lines << tr("  SHA-256: %1").arg(r.op_info.inputHash);
      card_lines.append(build_op_info_copy_lines(r.op_info));
      if (!card_lines.isEmpty()) {
        lines << QString();
        if (!r.tag.isEmpty()) lines << r.tag;
        lines.append(card_lines);
      }
    }
    lines << QString() << tr("Issued:  %1").arg(now);
    current_copy_text_ = lines.join(QLatin1Char('\n'));
  }
}

auto InfoBoardWidget::populate_extra_from_op_info(QVBoxLayout* extra_layout,
                                                  QWidget* parent,
                                                  const GpgOpResultInfo& info)
    -> bool {
  if (extra_layout == nullptr || parent == nullptr) return false;

  auto create_card = [&](InfoBoardStatus cs, QFrame*& out_card,
                         QVBoxLayout*& out_layout) {
    const QColor ic = StatusColor(cs);
    out_card = new QFrame(parent);
    out_card->setObjectName(QStringLiteral("DocResultRow"));
    out_card->setStyleSheet(build_card_stylesheet(ic));
    out_layout = new QVBoxLayout(out_card);
    out_layout->setContentsMargins(10, 6, 10, 6);
    out_layout->setSpacing(3);
  };

  auto add_header = [&](QVBoxLayout* cl, QWidget* p, InfoBoardStatus cs,
                        const QString& title) {
    const QColor ic = StatusColor(cs);
    auto* hdr = new QHBoxLayout();
    hdr->setContentsMargins(0, 0, 0, 2);
    hdr->setSpacing(6);
    auto* icon = new QLabel(p);
    icon->setFixedSize(16, 16);
    icon->setPixmap(
        QPixmap(StatusIconPath(cs))
            .scaled(14, 14, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    auto* lbl = new QLabel(title, p);
    lbl->setStyleSheet(
        QStringLiteral("color: %1; font-weight: bold;").arg(ic.name()));
    hdr->addWidget(icon, 0, Qt::AlignVCenter);
    hdr->addWidget(lbl, 1);
    cl->addLayout(hdr);
  };

  constexpr int kKeyW = StyleConstants::kCardKeyWidth;
  auto add_field = [&](QVBoxLayout* cl, QWidget* p, const QString& key,
                       const QString& val) {
    if (val.isEmpty()) return;
    auto* row = new QHBoxLayout();
    row->setContentsMargins(0, 1, 0, 1);
    row->setSpacing(6);
    auto* kl = new QLabel(key, p);
    QFont kf = kl->font();
    kf.setBold(true);
    kf.setPointSize(std::max(7, kf.pointSize() - 1));
    kl->setFont(kf);
    kl->setStyleSheet(QStringLiteral("color: palette(mid);"));
    kl->setFixedWidth(kKeyW);
    kl->setAlignment(Qt::AlignRight | Qt::AlignTop);
    auto* vl = new QLabel(val, p);
    vl->setWordWrap(true);
    vl->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    QFont vf = vl->font();
    vf.setPointSize(std::max(8, vf.pointSize() - 1));
    vl->setFont(vf);
    row->addWidget(kl);
    row->addWidget(vl, 1);
    cl->addLayout(row);
  };

  bool has_content = false;

  // Show input hash card for all operations
  if (!info.inputHash.isEmpty()) {
    current_input_hash_ = info.inputHash;
    QFrame* card = nullptr;
    QVBoxLayout* cl = nullptr;
    create_card(kINFO_ERROR_NEUTRAL, card, cl);
    add_header(cl, card, kINFO_ERROR_NEUTRAL, tr("Input Material Hash"));

    auto* hash_row = new QHBoxLayout();
    hash_row->setContentsMargins(0, 2, 0, 2);
    hash_row->setSpacing(6);

    auto* hash_key = new QLabel(tr("SHA-256"), card);
    QFont kf = hash_key->font();
    kf.setBold(true);
    kf.setPointSize(std::max(7, kf.pointSize() - 1));
    hash_key->setFont(kf);
    hash_key->setStyleSheet(QStringLiteral("color: palette(mid);"));
    hash_key->setFixedWidth(kKeyW);
    hash_key->setAlignment(Qt::AlignRight | Qt::AlignTop);

    auto* hash_val = new QLabel(info.inputHash, card);
    hash_val->setWordWrap(true);
    hash_val->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    QFont vf = hash_val->font();
    vf.setFamily(QStringLiteral("Monospace"));
    vf.setPointSize(std::max(7, vf.pointSize() - 1));
    hash_val->setFont(vf);
    hash_val->setStyleSheet(QStringLiteral("color: palette(text);"));

    hash_row->addWidget(hash_key);
    hash_row->addWidget(hash_val, 1);
    cl->addLayout(hash_row);

    extra_layout->addWidget(card);
    has_content = true;
  }

  // Verify: one card per signature
  for (const auto& sig : info.signatures) {
    const InfoBoardStatus cs = [&]() -> InfoBoardStatus {
      switch (sig.validity) {
        case GpgSigValidity::kFULLY_VALID:
          return kINFO_ERROR_OK;
        case GpgSigValidity::kVALID_NOT_FULLY_TRUSTED:
        case GpgSigValidity::kVALID_WITH_ISSUES:
        case GpgSigValidity::kSIG_EXPIRED:
        case GpgSigValidity::kKEY_EXPIRED:
          return kINFO_ERROR_WARN;
        default:
          return kINFO_ERROR_CRITICAL;
      }
    }();

    const QString vtext = [&]() -> QString {
      switch (sig.validity) {
        case GpgSigValidity::kFULLY_VALID:
          return tr("Fully Valid");
        case GpgSigValidity::kVALID_WITH_ISSUES:
          return tr("Valid (with Issues)");
        case GpgSigValidity::kVALID_NOT_FULLY_TRUSTED:
          return tr("Valid (Not Fully Trusted)");
        case GpgSigValidity::kINVALID:
          return tr("Invalid");
        case GpgSigValidity::kKEY_MISSING:
          return tr("Key Missing");
        case GpgSigValidity::kKEY_REVOKED:
          return tr("Key Revoked");
        case GpgSigValidity::kSIG_EXPIRED:
          return tr("Signature Expired");
        case GpgSigValidity::kKEY_EXPIRED:
          return tr("Key Expired");
        case GpgSigValidity::kERROR:
          return tr("Verification Error");
        default:
          return tr("Unknown");
      }
    }();

    QFrame* card = nullptr;
    QVBoxLayout* cl = nullptr;
    create_card(cs, card, cl);
    add_header(cl, card, cs, vtext);

    if (!sig.signer.uid.isEmpty()) {
      add_field(cl, card, tr("Signer"), sig.signer.uid);
    } else if (!sig.signer.fingerprint.isEmpty()) {
      add_field(cl, card, tr("Fingerprint"), sig.signer.fingerprint);
    }
    add_field(cl, card, tr("Key ID"), sig.signer.keyId);
    if (!sig.signer.pubkeyAlgo.isEmpty() || !sig.signer.hashAlgo.isEmpty()) {
      QString algo = sig.signer.pubkeyAlgo;
      if (!sig.signer.hashAlgo.isEmpty()) {
        algo += QStringLiteral(" / ") + sig.signer.hashAlgo;
      }
      add_field(cl, card, tr("Algorithm"), algo);
    }
    if (sig.signer.signTime.isValid()) {
      add_field(cl, card, tr("Signed"),
                QLocale().toString(sig.signer.signTime.toLocalTime(),
                                   QLocale::ShortFormat));
    }
    for (const auto& w : sig.warnings) {
      auto* wl = new QLabel(QStringLiteral("⚠  ") + w, card);
      wl->setWordWrap(true);
      QFont wf = wl->font();
      wf.setPointSize(std::max(7, wf.pointSize() - 1));
      wl->setFont(wf);
      wl->setStyleSheet(QStringLiteral("color: %1;")
                            .arg(StatusColor(kINFO_ERROR_WARN).name()));
      cl->addWidget(wl);
    }

    extra_layout->addWidget(card);
    has_content = true;
  }

  // Sign: one card per new signature
  for (const auto& ns : info.newSignatures) {
    QFrame* card = nullptr;
    QVBoxLayout* cl = nullptr;
    create_card(kINFO_ERROR_OK, card, cl);
    add_header(cl, card, kINFO_ERROR_OK, tr("Signature Created"));

    if (!ns.signer.uid.isEmpty()) {
      add_field(cl, card, tr("Signer"), ns.signer.uid);
    } else if (!ns.signer.fingerprint.isEmpty()) {
      add_field(cl, card, tr("Fingerprint"), ns.signer.fingerprint);
    }
    add_field(cl, card, tr("Key ID"), ns.signer.keyId);
    add_field(cl, card, tr("Mode"), ns.sigMode);
    if (!ns.signer.pubkeyAlgo.isEmpty() || !ns.signer.hashAlgo.isEmpty()) {
      QString algo = ns.signer.pubkeyAlgo;
      if (!ns.signer.hashAlgo.isEmpty()) {
        algo += QStringLiteral(" / ") + ns.signer.hashAlgo;
      }
      add_field(cl, card, tr("Algorithm"), algo);
    }
    if (ns.signer.signTime.isValid()) {
      add_field(cl, card, tr("Signed"),
                QLocale().toString(ns.signer.signTime.toLocalTime(),
                                   QLocale::ShortFormat));
    }

    extra_layout->addWidget(card);
    has_content = true;
  }

  // Sign: one card per invalid signer
  for (const auto& inv : info.invalidSigners) {
    QFrame* card = nullptr;
    QVBoxLayout* cl = nullptr;
    create_card(kINFO_ERROR_WARN, card, cl);
    add_header(cl, card, kINFO_ERROR_WARN, tr("Invalid Signer"));
    add_field(cl, card, tr("Fingerprint"), inv.first);
    add_field(cl, card, tr("Reason"), inv.second);
    extra_layout->addWidget(card);
    has_content = true;
  }

  // Decrypt: one metadata card
  const bool is_decrypt_with_data =
      info.newSignatures.isEmpty() && info.invalidSigners.isEmpty() &&
      (!info.recipients.isEmpty() || !info.details.isEmpty()) &&
      info.operation.contains(tr("Decrypt"));
  if (is_decrypt_with_data) {
    const InfoBoardStatus meta_cs =
        info.messageIntegrityProtected ? kINFO_ERROR_OK : kINFO_ERROR_WARN;
    QFrame* card = nullptr;
    QVBoxLayout* cl = nullptr;
    create_card(meta_cs, card, cl);
    add_header(cl, card, meta_cs, tr("Message Metadata"));

    add_field(cl, card, tr("File"), info.filename);
    add_field(cl, card, tr("Cipher"), info.symmetricAlgo);
    add_field(cl, card, tr("MIME"), info.mimeEncoded ? tr("Yes") : tr("No"));
    add_field(cl, card, tr("Integrity"),
              info.messageIntegrityProtected ? tr("Protected")
                                             : tr("Not Protected (unsafe)"));

    extra_layout->addWidget(card);
    has_content = true;
  }

  // Encrypt: one card per recipient key
  if (info.operation.contains(tr("Encrypt"))) {
    for (const auto& reci : info.recipients) {
      QFrame* card = nullptr;
      QVBoxLayout* cl = nullptr;
      create_card(kINFO_ERROR_OK, card, cl);
      add_header(cl, card, kINFO_ERROR_OK, tr("Encryption Recipient"));

      if (!reci.uid.isEmpty()) {
        add_field(cl, card, tr("Recipient"), reci.uid);
      } else if (!reci.fingerprint.isEmpty()) {
        add_field(cl, card, tr("Fingerprint"), reci.fingerprint);
      }
      add_field(cl, card, tr("Key ID"), reci.keyId);
      add_field(cl, card, tr("Algorithm"), reci.pubkeyAlgo);

      extra_layout->addWidget(card);
      has_content = true;
    }
  }

  return has_content;
}

void InfoBoardWidget::render_cards(QVBoxLayout* layout, QWidget* parent,
                                   const QContainer<InfoBoardCard>& cards) {
  if (layout == nullptr || parent == nullptr) return;

  for (const auto& card_data : cards) {
    const QColor card_color = StatusColor(card_data.status);
    const QString card_icon_path = StatusIconPath(card_data.status);

    auto* card = new QFrame(parent);
    card->setObjectName(QStringLiteral("DocResultRow"));
    card->setStyleSheet(build_card_stylesheet(card_color));
    auto* cl = new QVBoxLayout(card);
    cl->setContentsMargins(10, 6, 10, 6);
    cl->setSpacing(3);

    auto* hdr = new QHBoxLayout();
    hdr->setContentsMargins(0, 0, 0, 2);
    hdr->setSpacing(6);
    auto* icon = new QLabel(card);
    icon->setFixedSize(16, 16);
    icon->setPixmap(
        QPixmap(card_icon_path)
            .scaled(14, 14, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    auto* lbl = new QLabel(card_data.title, card);
    lbl->setStyleSheet(
        QStringLiteral("color: %1; font-weight: bold;").arg(card_color.name()));
    hdr->addWidget(icon, 0, Qt::AlignVCenter);
    hdr->addWidget(lbl, 1);
    cl->addLayout(hdr);

    constexpr int kKeyW = StyleConstants::kCardKeyWidth;
    for (const auto& field : card_data.fields) {
      auto* row = new QHBoxLayout();
      row->setContentsMargins(0, 1, 0, 1);
      row->setSpacing(6);

      auto* kl = new QLabel(field.first, card);
      QFont kf = kl->font();
      kf.setBold(true);
      kf.setPointSize(std::max(7, kf.pointSize() - 1));
      kl->setFont(kf);
      kl->setStyleSheet(QStringLiteral("color: palette(mid);"));
      kl->setFixedWidth(kKeyW);
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

void InfoBoardWidget::slot_tab_changed(int index) {
  user_selected_details_tab_ = (index > 0);
}

auto InfoBoardWidget::build_op_info_copy_lines(
    const GpgOpResultInfo& info) const -> QStringList {
  QStringList lines;
  for (const auto& sig : info.signatures) {
    const QString who =
        sig.signer.uid.isEmpty() ? sig.signer.fingerprint : sig.signer.uid;
    if (!who.isEmpty()) lines << tr("  Signer: %1").arg(who);
    if (!sig.signer.keyId.isEmpty())
      lines << tr("  Key ID: %1").arg(sig.signer.keyId);
  }
  for (const auto& ns : info.newSignatures) {
    const QString who =
        ns.signer.uid.isEmpty() ? ns.signer.fingerprint : ns.signer.uid;
    if (!who.isEmpty()) lines << tr("  Signed: %1").arg(who);
    if (!ns.signer.keyId.isEmpty())
      lines << tr("  Key ID: %1").arg(ns.signer.keyId);
  }
  for (const auto& inv : info.invalidSigners) {
    lines << tr("  Invalid signer: %1 — %2").arg(inv.first, inv.second);
  }
  if (info.operation.contains(tr("Encrypt"))) {
    for (const auto& reci : info.recipients) {
      const QString who =
          reci.uid.isEmpty() ? reci.fingerprint : reci.uid;
      if (!who.isEmpty()) lines << tr("  Recipient: %1").arg(who);
      if (!reci.keyId.isEmpty())
        lines << tr("  Key ID: %1").arg(reci.keyId);
    }
  }
  return lines;
}

void InfoBoardWidget::slot_copy() {
  QString text;
  if (doc_scroll_ != nullptr && doc_scroll_->isVisible()) {
    text = current_copy_text_;
  } else {
    text = ui_->infoBoard->toPlainText();
  }
  if (text.trimmed().isEmpty()) return;
  QGuiApplication::clipboard()->setText(text);
  ui_->copyToolButton->setToolTip(tr("Copied"));
  QTimer::singleShot(1200, this, [this]() {
    ui_->copyToolButton->setToolTip(tr("Copy status text"));
  });
}

void InfoBoardWidget::export_doc_as_png(const QString& file_path) {
  const qreal export_scale = 3.0;
  QPixmap pm(doc_frame_->size() * export_scale);
  pm.fill(Qt::transparent);

  QPainter painter(&pm);
  painter.setRenderHint(QPainter::Antialiasing, true);
  painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
  painter.scale(export_scale, export_scale);
  doc_frame_->render(&painter);
  painter.end();

  if (pm.isNull()) {
    QMessageBox::warning(this, tr("Unable to Save"),
                         tr("The content could not be captured."));
    return;
  }

  QImage image = pm.toImage();
  image.setText(QStringLiteral("Software"), QStringLiteral("GpgFrontend"));
  image.setText(QStringLiteral("Title"), tr("GpgFrontend Security Report"));
  image.setText(QStringLiteral("Description"),
                tr("Cryptographic operation result report"));
  image.setText(QStringLiteral("Creation Time"),
                QDateTime::currentDateTime().toString(Qt::ISODate));

  if (!current_id_.isEmpty())
    image.setText(QStringLiteral("Document ID"), current_id_);

  if (hash_label_ != nullptr && !hash_label_->text().isEmpty()) {
    const QString hash_text = hash_label_->text();
    const QString prefix = tr("Hash: ");
    image.setText(QStringLiteral("Document Hash"),
                  hash_text.startsWith(prefix)
                      ? hash_text.mid(static_cast<int>(prefix.length()))
                      : hash_text);
  }

  if (val_status_ != nullptr && !val_status_->text().isEmpty())
    image.setText(QStringLiteral("Status"), val_status_->text());

  if (val_operation_ != nullptr && !val_operation_->text().isEmpty())
    image.setText(QStringLiteral("Operation"), val_operation_->text());

  if (val_engine_ != nullptr && !val_engine_->text().isEmpty())
    image.setText(QStringLiteral("Engine"), val_engine_->text());

  if (!current_input_hash_.isEmpty())
    image.setText(QStringLiteral("Input Hash"), current_input_hash_);

  if (!current_copy_text_.isEmpty())
    image.setText(QStringLiteral("Comment"), current_copy_text_);

  if (!image.save(file_path))
    QMessageBox::warning(this, tr("Unable to Save"),
                         tr("The image could not be saved."));
}

void InfoBoardWidget::slot_save() {
  if (doc_scroll_ != nullptr && doc_scroll_->isVisible() &&
      doc_frame_ != nullptr) {
    const QString suggested =
        QStringLiteral("GpgFrontend_%1")
            .arg(current_id_.isEmpty() ? QStringLiteral("Document")
                                       : current_id_);
    QString file_path =
        QFileDialog::getSaveFileName(this, tr("Export Certificate"), suggested,
                                     tr("PNG Image (*.png);;All Files (*)"));
    if (file_path.isEmpty()) return;
    if (!file_path.endsWith(QStringLiteral(".png"), Qt::CaseInsensitive))
      file_path += QStringLiteral(".png");
    export_doc_as_png(file_path);
    return;
  }

  const auto text = ui_->infoBoard->toPlainText();
  if (text.trimmed().isEmpty()) return;

  auto file_path =
      QFileDialog::getSaveFileName(this, tr("Save Status Panel Content"), {},
                                   tr("Text Files (*.txt);;All Files (*)"));
  if (file_path.isEmpty()) return;

  QFile file(file_path);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate |
                 QIODevice::Text)) {
    QMessageBox::warning(this, tr("Unable to Save"),
                         tr("The file could not be saved. Please check the "
                            "path and permissions."));
    return;
  }
  file.write(text.toUtf8());
  file.close();
}

}  // namespace GpgFrontend::UI
