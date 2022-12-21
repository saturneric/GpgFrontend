/*
 * Copyright (c) 2022. Saturneric
 *
 *  This file is part of GpgFrontend.
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
 * Saturneric<eric@bktus.com> starting on May 12, 2021.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

//
// Created by eric on 2022/7/23.
//

#include "GnupgTab.h"

#include <vector>

#include "easylogging++.h"
#include "ui_GnuPGInfo.h"

GpgFrontend::UI::GnupgTab::GnupgTab(QWidget* parent)
    : QWidget(parent),
      ui_(std::make_shared<Ui_GnuPGInfo>()),
      gpgconf_process_(new QProcess(this)) {
  GpgContext& ctx = GpgContext::GetInstance();
  auto info = ctx.GetInfo();

  ui_->setupUi(this);

  QStringList column_titles;
  column_titles << _("Name") << _("Description") << _("Version") << _("Path");

  ui_->conponentDetailsTable->setColumnCount(column_titles.length());
  ui_->conponentDetailsTable->setHorizontalHeaderLabels(column_titles);
  ui_->conponentDetailsTable->horizontalHeader()->setStretchLastSection(false);
  ui_->conponentDetailsTable->setSelectionBehavior(
      QAbstractItemView::SelectRows);

  // tableitems not editable
  ui_->conponentDetailsTable->setEditTriggers(
      QAbstractItemView::NoEditTriggers);

  // no focus (rectangle around tableitems)
  // may be it should focus on whole row
  ui_->conponentDetailsTable->setFocusPolicy(Qt::NoFocus);
  ui_->conponentDetailsTable->setAlternatingRowColors(true);

  gpgconf_process_->start(QString::fromStdString(info.GpgConfPath),
                          QStringList() << "--list-components");

  connect(gpgconf_process_,
          qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this,
          &GnupgTab::process_components_info);
}

void GpgFrontend::UI::GnupgTab::process_components_info(
    int exit_code, QProcess::ExitStatus exit_status) {
  LOG(INFO) << "called";

  GpgContext& ctx = GpgContext::GetInstance();
  auto info = ctx.GetInfo();

  std::vector<std::vector<std::string>> components_info = {
      {"gpgme", "GPG Made Easy", info.GpgMEVersion, "/"},
      {"gpgconf", "GPG Configure", "/", info.GpgConfPath},

  };

  if (gpgconf_process_ != nullptr) {
    QString data = gpgconf_process_->readAllStandardOutput();

    std::vector<std::string> line_split_list;
    boost::split(line_split_list, data.toStdString(), boost::is_any_of("\n"));

    for (const auto& line : line_split_list) {
      std::vector<std::string> info_split_list;
      boost::split(info_split_list, line, boost::is_any_of(":"));
      LOG(INFO) << "gpgconf info line" << line << "info size"
                << info_split_list.size();

      if (info_split_list.size() != 3) continue;

      if (info_split_list[0] == "gpg") {
        components_info.push_back({info_split_list[0], info_split_list[1],
                                   info.GnupgVersion, info_split_list[2]});
      } else {
        components_info.push_back(
            {info_split_list[0], info_split_list[1], "/", info_split_list[2]});
      }
    }
  }

  ui_->conponentDetailsTable->setRowCount(components_info.size());

  int row = 0;
  for (const auto& info : components_info) {
    if (info.size() != 4) continue;

    auto* tmp0 = new QTableWidgetItem(QString::fromStdString(info[0]));
    tmp0->setTextAlignment(Qt::AlignCenter);
    ui_->conponentDetailsTable->setItem(row, 0, tmp0);

    auto* tmp1 = new QTableWidgetItem(QString::fromStdString(info[1]));
    tmp1->setTextAlignment(Qt::AlignCenter);
    ui_->conponentDetailsTable->setItem(row, 1, tmp1);

    auto* tmp2 = new QTableWidgetItem(QString::fromStdString(info[2]));
    tmp2->setTextAlignment(Qt::AlignCenter);
    ui_->conponentDetailsTable->setItem(row, 2, tmp2);

    auto* tmp3 = new QTableWidgetItem(QString::fromStdString(info[3]));
    tmp3->setTextAlignment(Qt::AlignLeft);
    ui_->conponentDetailsTable->setItem(row, 3, tmp3);

    row++;
  }
}
