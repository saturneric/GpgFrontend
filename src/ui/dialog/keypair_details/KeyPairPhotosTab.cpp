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

#include "KeyPairPhotosTab.h"

#include "core/utils/FilesystemUtils.h"
#include "ui_KeyPairPhotosTab.h"

namespace GpgFrontend::UI {

KeyPairPhotosTab::KeyPairPhotosTab(int channel, GpgKeyPtr key,
                                   const QContainer<GpgAttrInfo>& infos,
                                   QWidget* parent)
    : QWidget(parent),
      ui_(GpgFrontend::SecureCreateSharedObject<Ui_KeyPairPhotosTab>()),
      current_gpg_context_channel_(channel),
      key_(std::move(key)),
      infos_(infos) {
  ui_->setupUi(this);

  auto* table_widget = ui_->tableWidget;
  table_widget->setColumnCount(5);
  table_widget->setHorizontalHeaderLabels(
      {tr("Type"), tr("Flags"), tr("Date"), tr("Size"), tr("Thumbnail")});

  int row_count = 0;
  for (const auto& info : infos_) {
    if (info.ext != "jpg") continue;
    jpg_infos_.push_back(info);
    row_count++;
  }

  table_widget->setRowCount(row_count);

  int current_index = 0;
  for (const auto& info : jpg_infos_) {
    auto* item = new QTableWidgetItem(QString::number(info.type));
    item->setTextAlignment(Qt::AlignCenter);
    table_widget->setItem(current_index, 0, item);

    item = new QTableWidgetItem(QString::number(info.flags));
    item->setTextAlignment(Qt::AlignCenter);
    table_widget->setItem(current_index, 1, item);

    item = new QTableWidgetItem(QLocale().toString(
        QDateTime::fromSecsSinceEpoch(info.ts), QLocale::ShortFormat));
    item->setTextAlignment(Qt::AlignCenter);
    table_widget->setItem(current_index, 2, item);

    item = new QTableWidgetItem(
        GpgFrontend::GetHumanFriendlyFileSize(info.payload.size()));
    item->setTextAlignment(Qt::AlignCenter);
    table_widget->setItem(current_index, 3, item);

    // Thumbnail
    item = new QTableWidgetItem();
    QPixmap pixmap;
    pixmap.loadFromData(info.payload, "JPG");
    auto* label = new QLabel();
    label->setAlignment(Qt::AlignCenter);
    label->setPixmap(
        pixmap.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    table_widget->setCellWidget(current_index, 4, label);

    current_index++;
  }

  table_widget->resizeColumnsToContents();

  connect(table_widget, &QTableWidget::itemSelectionChanged, this,
          &KeyPairPhotosTab::slot_view_photo);

  ui_->listGroupBox->setTitle(
      tr("List of Photo IDs (%1)").arg(QString::number(jpg_infos_.size())));
  ui_->viewerGroupBox->setTitle(tr("Photo Viewer"));

  setAttribute(Qt::WA_DeleteOnClose, true);
}

KeyPairPhotosTab::~KeyPairPhotosTab() = default;

void KeyPairPhotosTab::slot_view_photo() {
  auto* photo_label = ui_->photoLabel;
  auto* table_widget = ui_->tableWidget;
  const auto selected_items = table_widget->selectedItems();

  if (selected_items.isEmpty()) {
    photo_label->clear();
    return;
  }

  const int selected_row = table_widget->row(selected_items.first());
  if (selected_row < 0 || selected_row >= jpg_infos_.size()) {
    photo_label->clear();
    return;
  }

  auto info = jpg_infos_.at(selected_row);

  QPixmap pixmap;
  pixmap.loadFromData(info.payload, "JPG");

  photo_label->setPixmap(pixmap.scaled(photo_label->size(), Qt::KeepAspectRatio,
                                       Qt::SmoothTransformation));
  photo_label->setAlignment(Qt::AlignCenter);
}
}  // namespace GpgFrontend::UI