/**
 * Copyright (C) 2021 Saturneric <eric@bktus.com>
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

#include "core/GpgModel.h"
#include "core/function/GlobalSettingStation.h"
#include "ui/UISignalStation.h"
#include "ui/struct/SettingsObject.h"
#include "ui_InfoBoard.h"

namespace GpgFrontend::UI {

InfoBoardWidget::InfoBoardWidget(QWidget* parent)
    : QWidget(parent),
      ui_(GpgFrontend::SecureCreateSharedObject<Ui_InfoBoard>()) {
  ui_->setupUi(this);

  ui_->actionButtonLayout->addStretch();
  ui_->copyButton->setText(_("Copy"));
  ui_->saveButton->setText(_("Save File"));
  ui_->clearButton->setText(_("Clear"));

  connect(ui_->copyButton, &QPushButton::clicked, this,
          &InfoBoardWidget::slot_copy);
  connect(ui_->saveButton, &QPushButton::clicked, this,
          &InfoBoardWidget::slot_save);
  connect(ui_->clearButton, &QPushButton::clicked, this,
          &InfoBoardWidget::SlotReset);

  connect(UISignalStation::GetInstance(),
          &UISignalStation::SignalRefreshInfoBoard, this,
          &InfoBoardWidget::SlotRefresh);
}

void InfoBoardWidget::SetInfoBoard(const QString& text,
                                   InfoBoardStatus verify_label_status) {
  QString color;
  ui_->infoBoard->clear();
  switch (verify_label_status) {
    case INFO_ERROR_OK:
      color = "#008000";
      break;
    case INFO_ERROR_WARN:
      color = "#FF8C00";
      break;
    case INFO_ERROR_CRITICAL:
      color = "#DC143C";
      break;
    default:
      break;
  }
  ui_->infoBoard->append(text);

  ui_->infoBoard->setAutoFillBackground(true);
  QPalette status = ui_->infoBoard->palette();
  status.setColor(QPalette::Text, color);
  ui_->infoBoard->setPalette(status);

  SettingsObject general_settings_state("general_settings_state");

  // info board font size
  auto info_font_size =
      general_settings_state.Check("text_editor").Check("font_size", 10);
  ui_->infoBoard->setFont(QFont("Times", info_font_size));
}

void InfoBoardWidget::SlotRefresh(const QString& text, InfoBoardStatus status) {
  ui_->infoBoard->clear();
  SetInfoBoard(text, status);
  ui_->infoBoard->verticalScrollBar()->setValue(0);
}

void InfoBoardWidget::AssociateTextEdit(QTextEdit* edit) {
  if (m_text_page_ != nullptr)
    disconnect(m_text_page_, &QTextEdit::textChanged, this,
               &InfoBoardWidget::SlotReset);
  this->m_text_page_ = edit;
  connect(edit, &QTextEdit::textChanged, this, &InfoBoardWidget::SlotReset);
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
  SPDLOG_DEBUG("add option: {}", name.toStdString());
  auto actionButton = new QPushButton(name);
  auto layout = new QHBoxLayout();
  layout->setContentsMargins(5, 0, 5, 0);
  ui_->infoBoard->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
  // set margin from surroundings
  layout->addWidget(actionButton);
  ui_->actionButtonLayout->addLayout(layout);
  connect(actionButton, &QPushButton::clicked, this, [=]() { action(); });
}

/**
 * Delete All item in actionButtonLayout
 */
void InfoBoardWidget::ResetOptionActionsMenu() {
  // skip stretch
  delete_widgets_in_layout(ui_->actionButtonLayout, 1);
}

void InfoBoardWidget::SlotReset() {
  ui_->infoBoard->clear();
  ResetOptionActionsMenu();
}

/**
 * Try Delete all widget from target layout
 * @param layout target layout
 */
void InfoBoardWidget::delete_widgets_in_layout(QLayout* layout,
                                               int start_index) {
  QLayoutItem* item;
  while ((item = layout->layout()->takeAt(start_index)) != nullptr) {
    layout->removeItem(item);
    if (item->layout() != nullptr)
      delete_widgets_in_layout(item->layout());
    else if (item->widget() != nullptr)
      delete item->widget();
    delete item;
  }
}

void InfoBoardWidget::slot_copy() {
  auto* clipboard = QGuiApplication::clipboard();
  clipboard->setText(ui_->infoBoard->toPlainText());
}

void InfoBoardWidget::slot_save() {
  auto file_path = QFileDialog::getSaveFileName(
      this, _("Save Information Board's Content"), {}, tr("Text (*.txt)"));
  SPDLOG_DEBUG("file path: {}", file_path.toStdString());
  if (file_path.isEmpty()) return;

  QFile file(file_path);
  if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    file.write(ui_->infoBoard->toPlainText().toUtf8());
  } else {
    QMessageBox::critical(
        this, _("Error"),
        _("The file path is not exists, unprivileged or unreachable."));
  }
  file.close();
}

}  // namespace GpgFrontend::UI
