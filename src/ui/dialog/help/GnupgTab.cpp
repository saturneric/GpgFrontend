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

//
// Created by eric on 2022/7/23.
//

#include "GnupgTab.h"

#include <shared_mutex>

#include "ui/UserInterfaceUtils.h"
#include "ui_GnuPGInfo.h"

GpgFrontend::UI::GnupgTab::GnupgTab(QWidget* parent)
    : QWidget(parent), ui_(std::make_shared<Ui_GnuPGInfo>()) {
  ui_->setupUi(this);

  QStringList components_column_titles;
  components_column_titles << _("Name") << _("Description") << _("Version")
                           << _("Checksum") << _("Binary Path");

  ui_->tabWidget->setTabText(0, _("Components"));
  ui_->tabWidget->setTabText(1, _("Configurations"));

  ui_->componentDetailsTable->setColumnCount(components_column_titles.length());
  ui_->componentDetailsTable->setHorizontalHeaderLabels(
      components_column_titles);
  ui_->componentDetailsTable->horizontalHeader()->setStretchLastSection(false);
  ui_->componentDetailsTable->setSelectionBehavior(
      QAbstractItemView::SelectRows);

  QStringList configurations_column_titles;
  configurations_column_titles << _("Key") << _("Value");

  ui_->configurationDetailsTable->setColumnCount(
      configurations_column_titles.length());
  ui_->configurationDetailsTable->setHorizontalHeaderLabels(
      configurations_column_titles);
  ui_->configurationDetailsTable->horizontalHeader()->setStretchLastSection(
      false);
  ui_->configurationDetailsTable->setSelectionBehavior(
      QAbstractItemView::SelectRows);

  // tableitems not editable
  ui_->componentDetailsTable->setEditTriggers(
      QAbstractItemView::NoEditTriggers);

  // no focus (rectangle around tableitems)
  // may be it should focus on whole row
  ui_->componentDetailsTable->setFocusPolicy(Qt::NoFocus);
  ui_->componentDetailsTable->setAlternatingRowColors(true);

  process_software_info();
}

void GpgFrontend::UI::GnupgTab::process_software_info() {
  auto& ctx_info = GpgContext::GetInstance().GetInfo();

  ui_->gnupgVersionLabel->setText(QString::fromStdString(
      fmt::format("Version: {}", ctx_info.GnupgVersion)));

  ui_->componentDetailsTable->setRowCount(ctx_info.ComponentsInfo.size());

  int row = 0;
  for (const auto& info : ctx_info.ComponentsInfo) {
    if (info.second.size() != 4) continue;

    auto* tmp0 = new QTableWidgetItem(QString::fromStdString(info.first));
    tmp0->setTextAlignment(Qt::AlignCenter);
    ui_->componentDetailsTable->setItem(row, 0, tmp0);

    auto* tmp1 = new QTableWidgetItem(QString::fromStdString(info.second[0]));
    tmp1->setTextAlignment(Qt::AlignCenter);
    ui_->componentDetailsTable->setItem(row, 1, tmp1);

    auto* tmp2 = new QTableWidgetItem(QString::fromStdString(info.second[1]));
    tmp2->setTextAlignment(Qt::AlignCenter);
    ui_->componentDetailsTable->setItem(row, 2, tmp2);

    auto* tmp3 = new QTableWidgetItem(QString::fromStdString(info.second[3]));
    tmp3->setTextAlignment(Qt::AlignCenter);
    ui_->componentDetailsTable->setItem(row, 3, tmp3);

    auto* tmp4 = new QTableWidgetItem(QString::fromStdString(info.second[2]));
    tmp4->setTextAlignment(Qt::AlignLeft);
    ui_->componentDetailsTable->setItem(row, 4, tmp4);

    row++;
  }

  ui_->componentDetailsTable->resizeColumnsToContents();

  ui_->configurationDetailsTable->setRowCount(
      ctx_info.ConfigurationsInfo.size());

  row = 0;
  for (const auto& info : ctx_info.ConfigurationsInfo) {
    if (info.second.size() != 1) continue;

    auto* tmp0 = new QTableWidgetItem(QString::fromStdString(info.first));
    tmp0->setTextAlignment(Qt::AlignCenter);
    ui_->configurationDetailsTable->setItem(row, 0, tmp0);

    auto* tmp1 = new QTableWidgetItem(QString::fromStdString(info.second[0]));
    tmp1->setTextAlignment(Qt::AlignCenter);
    ui_->configurationDetailsTable->setItem(row, 1, tmp1);

    row++;
  }

  ui_->configurationDetailsTable->resizeColumnsToContents();
}
