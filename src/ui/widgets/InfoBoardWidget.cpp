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

#include "core/model/SettingsObject.h"
#include "ui/UISignalStation.h"
#include "ui/struct/settings_object/AppearanceSO.h"
#include "ui/widgets/InfoBoardDocFrame.h"
#include "ui_InfoBoard.h"

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
  connect(ui_->magnifierToolButton, &QToolButton::clicked, this,
          &InfoBoardWidget::slot_open_magnifier);
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
  // Set the captions and tooltips here (rather than relying on the values baked
  // into the .ui file) so they are extracted under this widget's own
  // translation context; strings left only in the .ui are not picked up by
  // lupdate in this project.
  ui_->copyToolButton->setText(tr("Copy"));
  ui_->saveToolButton->setText(tr("Save"));
  ui_->magnifierToolButton->setText(tr("Magnify"));
  ui_->clearToolButton->setText(tr("Clear"));

  ui_->copyToolButton->setToolTip(tr("Copy status text"));
  ui_->saveToolButton->setToolTip(tr("Save status text to file"));
  ui_->magnifierToolButton->setToolTip(tr("Magnify the generated document"));
  ui_->clearToolButton->setToolTip(tr("Clear status panel"));

  for (auto* button : {ui_->copyToolButton, ui_->saveToolButton,
                       ui_->magnifierToolButton, ui_->clearToolButton}) {
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
  setup_view_switcher();
  ApplyStatusStyle(kINFO_ERROR_NEUTRAL);
  UpdateActionButtons();
}

void InfoBoardWidget::setup_view_switcher() {
  // Keep these as native checkable tool buttons rather than a
  // stylesheet-painted pill: an exclusive group renders the active segment with
  // the platform's own "checked" look, which stays correct across themes and
  // platforms.
  // Set the captions here (rather than relying on the values baked into the
  // .ui file) so they are extracted under this widget's own translation
  // context alongside every other string the widget owns.
  ui_->segStatusButton->setText(tr("Status"));
  ui_->segDetailsButton->setText(tr("Details"));
  ui_->segStatusButton->setToolTip(tr("Show the summary report"));
  ui_->segDetailsButton->setToolTip(tr("Show the raw status text"));
  for (auto* button : {ui_->segStatusButton, ui_->segDetailsButton}) {
    button->setToolButtonStyle(Qt::ToolButtonTextOnly);
    button->setFocusPolicy(Qt::NoFocus);
  }

  view_group_ = new QButtonGroup(this);
  view_group_->setExclusive(true);
  view_group_->addButton(ui_->segStatusButton, 0);
  view_group_->addButton(ui_->segDetailsButton, 1);

  connect(view_group_, &QButtonGroup::idClicked, this,
          [this](int id) { set_active_view(id); });
  connect(ui_->stackedWidget, &QStackedWidget::currentChanged, this,
          [this]() { UpdateActionButtons(); });

  // Restore the view the user last looked at; the choice is remembered across
  // operations and sessions rather than being reset on every result.
  set_active_view(load_persisted_view(), false);
}

void InfoBoardWidget::set_active_view(int index, bool persist) {
  index = index == 1 ? 1 : 0;
  ui_->stackedWidget->setCurrentIndex(index);
  if (auto* button = view_group_->button(index);
      button != nullptr && !button->isChecked()) {
    button->setChecked(true);
  }
  if (persist) persist_view(index);
  UpdateActionButtons();
}

auto InfoBoardWidget::load_persisted_view() const -> int {
  SettingsObject so("info_board_state");
  if (const auto v = so["view_index"]; v.isDouble()) {
    return v.toInt() == 1 ? 1 : 0;
  }
  return 0;
}

void InfoBoardWidget::persist_view(int index) const {
  SettingsObject so("info_board_state");
  so["view_index"] = index;
  so.Store(so);
}

void InfoBoardWidget::UpdateActionButtons() {
  const bool has_text = !ui_->infoBoard->toPlainText().trimmed().isEmpty();
  ui_->copyToolButton->setEnabled(has_text);
  ui_->saveToolButton->setEnabled(has_text);
  ui_->clearToolButton->setEnabled(has_text);

  // The magnifier inspects the rendered report, so it is only meaningful while
  // a generated document is on screen and the Status view (index 0) is active.
  const bool on_status_view = ui_->stackedWidget->currentIndex() == 0;
  const bool has_document = doc_scroll_ != nullptr && doc_scroll_->isVisible();
  ui_->magnifierToolButton->setEnabled(has_document && on_status_view);
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

auto InfoBoardWidget::build_status_symbol(InfoBoardStatus status) const
    -> QString {
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

void InfoBoardWidget::set_info_board_text(const QString& text,
                                          InfoBoardStatus status) {
  ApplyStatusStyle(status);
  ui_->infoBoard->clear();

  const auto title = StatusTitle(status);
  const auto body = text.trimmed();

  const auto final_text = body.isEmpty()
                              ? tr("[%1] No details available.").arg(title)
                              : QString("[%1] %2").arg(title, body);

  ui_->infoBoard->setPlainText(final_text);
  ui_->infoBoard->moveCursor(QTextCursor::Start);
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
  set_info_board_text(text, status);

  if (status != kINFO_ERROR_NEUTRAL && doc_frame_ != nullptr) {
    update_status_page(text, status, content_hash);
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
  set_info_board_text(text, status);

  if (status != kINFO_ERROR_NEUTRAL && doc_frame_ != nullptr) {
    update_status_page(text, status, content_hash, operation, description,
                       cards, details_title, details_items);
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

  reset_document_view();
  clear_document_fields();
}

}  // namespace GpgFrontend::UI
