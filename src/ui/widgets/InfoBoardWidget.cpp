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
#include "ui_InfoBoard.h"

namespace GpgFrontend::UI {

InfoBoardWidget::InfoBoardWidget(QWidget* parent)
    : QWidget(parent),
      ui_(GpgFrontend::SecureCreateSharedObject<Ui_InfoBoard>()) {
  ui_->setupUi(this);

  ui_->actionButtonLayout->addStretch();

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
}

void InfoBoardWidget::InitUI() {
  setObjectName(QStringLiteral("InfoBoardWidget"));

  ui_->copyToolButton->setToolTip(tr("Copy status text"));
  ui_->saveToolButton->setToolTip(tr("Save status text to file"));
  ui_->clearToolButton->setToolTip(tr("Clear status panel"));

  ui_->copyToolButton->setAutoRaise(false);
  ui_->saveToolButton->setAutoRaise(false);
  ui_->clearToolButton->setAutoRaise(false);

  ui_->copyToolButton->setFocusPolicy(Qt::NoFocus);
  ui_->saveToolButton->setFocusPolicy(Qt::NoFocus);
  ui_->clearToolButton->setFocusPolicy(Qt::NoFocus);

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
  border-radius: 6px;
  padding: 6px;
  background: palette(base);
  selection-background-color: palette(highlight);
  selection-color: palette(highlighted-text);
}

QWidget#InfoBoardWidget QTextEdit[status="ok"] {
  border: 1px solid #2e7d32;
}

QWidget#InfoBoardWidget QTextEdit[status="warn"] {
  border: 1px solid #ef6c00;
}

QWidget#InfoBoardWidget QTextEdit[status="critical"] {
  border: 1px solid #c62828;
}

QWidget#InfoBoardWidget QTextEdit[status="neutral"] {
  border: 1px solid palette(mid);
}

QWidget#InfoBoardWidget QToolButton {
  min-width: 28px;
  min-height: 26px;
  padding: 3px;
  border: 1px solid palette(mid);
  border-radius: 5px;
  background: palette(button);
}

QWidget#InfoBoardWidget QToolButton:hover {
  background: palette(light);
}

QWidget#InfoBoardWidget QPushButton {
  min-height: 26px;
  padding: 3px 10px;
  border: 1px solid palette(mid);
  border-radius: 5px;
  background: palette(button);
}

QWidget#InfoBoardWidget QPushButton:hover {
  background: palette(light);
}
)");

  ApplyStatusStyle(INFO_ERROR_NEUTRAL);
  UpdateActionButtons();
}

void InfoBoardWidget::UpdateActionButtons() {
  const bool has_text = !ui_->infoBoard->toPlainText().trimmed().isEmpty();

  ui_->copyToolButton->setEnabled(has_text);
  ui_->saveToolButton->setEnabled(has_text);
  ui_->clearToolButton->setEnabled(has_text);
}

auto InfoBoardWidget::StatusTitle(InfoBoardStatus status) const -> QString {
  switch (status) {
    case INFO_ERROR_OK:
      return tr("Success");
    case INFO_ERROR_WARN:
      return tr("Warning");
    case INFO_ERROR_CRITICAL:
      return tr("Error");
    case INFO_ERROR_NEUTRAL:
    default:
      return tr("Information");
  }
}

void InfoBoardWidget::ApplyStatusStyle(InfoBoardStatus status) {
  QString status_name;
  QColor text_color;

  switch (status) {
    case INFO_ERROR_OK:
      status_name = QStringLiteral("ok");
      text_color = QColor(46, 125, 50);
      break;
    case INFO_ERROR_WARN:
      status_name = QStringLiteral("warn");
      text_color = QColor(239, 108, 0);
      break;
    case INFO_ERROR_CRITICAL:
      status_name = QStringLiteral("critical");
      text_color = QColor(198, 40, 40);
      break;
    case INFO_ERROR_NEUTRAL:
    default:
      status_name = QStringLiteral("neutral");
      text_color = palette().text().color();
      break;
  }

  ui_->infoBoard->setProperty("status", status_name);
  ui_->infoBoard->style()->unpolish(ui_->infoBoard);
  ui_->infoBoard->style()->polish(ui_->infoBoard);

  auto pal = ui_->infoBoard->palette();
  pal.setColor(QPalette::Text, text_color);
  ui_->infoBoard->setPalette(pal);
}

void InfoBoardWidget::SetInfoBoard(const QString& text,
                                   InfoBoardStatus verify_label_status) {
  ApplyStatusStyle(verify_label_status);

  ui_->infoBoard->clear();

  const auto title = StatusTitle(verify_label_status);
  const auto body = text.trimmed();

  const auto final_text = body.isEmpty()
                              ? tr("[%1] No details available.").arg(title)
                              : tr("[%1] %2").arg(title, body);

  ui_->infoBoard->setPlainText(final_text);
  ui_->infoBoard->moveCursor(QTextCursor::Start);

  UpdateActionButtons();
}

void InfoBoardWidget::SlotRefresh(const QString& text, InfoBoardStatus status) {
  ui_->infoBoard->clear();
  SetInfoBoard(text, status);
  ui_->infoBoard->verticalScrollBar()->setValue(0);
}

void InfoBoardWidget::AssociateTabWidget(QTabWidget* tab) {
  m_text_page_ = nullptr;
  m_tab_widget_ = tab;
  connect(tab, &QTabWidget::tabBarClicked, this, &InfoBoardWidget::SlotReset);
  connect(tab, &QTabWidget::tabCloseRequested, this,
          &InfoBoardWidget::SlotReset);
  // reset
  this->SlotReset();
}

void InfoBoardWidget::AddOptionalAction(const QString& name,
                                        const std::function<void()>& action) {
  auto* action_button = new QPushButton(name, this);
  action_button->setFocusPolicy(Qt::NoFocus);
  action_button->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
  action_button->setProperty("optionalInfoBoardAction", true);

  ui_->actionButtonLayout->insertWidget(
      std::max(0, ui_->actionButtonLayout->count() - 1), action_button);

  connect(action_button, &QPushButton::clicked, this, [action]() {
    if (action) action();
  });
}

void InfoBoardWidget::ResetOptionActionsMenu() {
  const auto buttons = findChildren<QPushButton*>();
  for (auto* button : buttons) {
    if (button->property("optionalInfoBoardAction").toBool()) {
      button->deleteLater();
    }
  }
}
void InfoBoardWidget::SlotReset() {
  ui_->infoBoard->clear();
  ui_->infoBoard->setPlaceholderText(tr("Operation status will appear here."));
  ApplyStatusStyle(INFO_ERROR_NEUTRAL);
  ResetOptionActionsMenu();
  UpdateActionButtons();
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

void InfoBoardWidget::slot_copy() {
  const auto text = ui_->infoBoard->toPlainText();
  if (text.trimmed().isEmpty()) return;

  auto* clipboard = QGuiApplication::clipboard();
  clipboard->setText(text);

  ui_->copyToolButton->setToolTip(tr("Copied"));
  QTimer::singleShot(1200, this, [this]() {
    ui_->copyToolButton->setToolTip(tr("Copy status text"));
  });
}

void InfoBoardWidget::slot_save() {
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
