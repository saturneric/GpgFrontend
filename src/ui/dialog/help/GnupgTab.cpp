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

#include "core/module/ModuleManager.h"
#include "ui_GnuPGInfo.h"

GpgFrontend::UI::GnupgTab::GnupgTab(QWidget* parent)
    : QWidget(parent),
      ui_(GpgFrontend::SecureCreateSharedObject<Ui_GnuPGInfo>()) {
  ui_->setupUi(this);

  QStringList components_column_titles;
  components_column_titles << tr("Name") << tr("Description") << tr("Version")
                           << tr("Checksum") << tr("Binary Path");

  ui_->tabWidget->setTabText(0, tr("Components"));
  ui_->tabWidget->setTabText(1, tr("Directories"));
  ui_->tabWidget->setTabText(2, tr("Options"));

  ui_->componentDetailsTable->setColumnCount(components_column_titles.length());
  ui_->componentDetailsTable->setHorizontalHeaderLabels(
      components_column_titles);
  ui_->componentDetailsTable->horizontalHeader()->setStretchLastSection(false);
  ui_->componentDetailsTable->setSelectionBehavior(
      QAbstractItemView::SelectRows);

  // tableitems not editable
  ui_->componentDetailsTable->setEditTriggers(
      QAbstractItemView::NoEditTriggers);

  // no focus (rectangle around tableitems)
  // may be it should focus on whole row
  ui_->componentDetailsTable->setFocusPolicy(Qt::NoFocus);
  ui_->componentDetailsTable->setAlternatingRowColors(true);

  QStringList directories_column_titles;
  directories_column_titles << tr("Directory Type") << tr("Path");

  ui_->directoriesDetailsTable->setColumnCount(
      directories_column_titles.length());
  ui_->directoriesDetailsTable->setHorizontalHeaderLabels(
      directories_column_titles);
  ui_->directoriesDetailsTable->horizontalHeader()->setStretchLastSection(
      false);
  ui_->directoriesDetailsTable->setSelectionBehavior(
      QAbstractItemView::SelectRows);

  // tableitems not editable
  ui_->directoriesDetailsTable->setEditTriggers(
      QAbstractItemView::NoEditTriggers);

  // no focus (rectangle around tableitems)
  // may be it should focus on whole row
  ui_->directoriesDetailsTable->setFocusPolicy(Qt::NoFocus);
  ui_->directoriesDetailsTable->setAlternatingRowColors(true);

  QStringList options_column_titles;
  options_column_titles << tr("Component") << tr("Group") << tr("Key")
                        << tr("Description") << tr("Default Value")
                        << tr("Value");

  ui_->optionDetailsTable->setColumnCount(options_column_titles.length());
  ui_->optionDetailsTable->setHorizontalHeaderLabels(options_column_titles);
  ui_->optionDetailsTable->horizontalHeader()->setStretchLastSection(false);
  ui_->optionDetailsTable->setSelectionBehavior(QAbstractItemView::SelectRows);

  // tableitems not editable
  ui_->optionDetailsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

  // no focus (rectangle around tableitems)
  // may be it should focus on whole row
  ui_->optionDetailsTable->setFocusPolicy(Qt::NoFocus);
  ui_->optionDetailsTable->setAlternatingRowColors(true);

  process_software_info();
}

void GpgFrontend::UI::GnupgTab::process_software_info() {
  const auto gnupg_version = Module::RetrieveRTValueTypedOrDefault<>(
      "core", "gpgme.ctx.gnupg_version", QString{"2.0.0"});
  GF_UI_LOG_DEBUG("got gnupg version from rt: {}", gnupg_version);

  ui_->gnupgVersionLabel->setText(
      QString::fromStdString(fmt::format("Version: {}", gnupg_version)));

  auto components = Module::ListRTChildKeys(
      "com.bktus.gpgfrontend.module.integrated.gnupg-info-gathering",
      "gnupg.components");
  GF_UI_LOG_DEBUG("got gnupg components from rt, size: {}", components.size());

  ui_->componentDetailsTable->setRowCount(components.size());

  int row = 0;
  for (auto& component : components) {
    auto component_info_json_bytes = Module::RetrieveRTValueTypedOrDefault(
        "com.bktus.gpgfrontend.module.integrated.gnupg-info-gathering",
        QString("gnupg.components.%1").arg(component), QByteArray{});
    GF_UI_LOG_DEBUG("got gnupg component {} info from rt", component);

    auto component_info_json =
        QJsonDocument::fromJson(component_info_json_bytes);
    if (!component_info_json.isObject()) {
      GF_UI_LOG_WARN("illegal gnupg component info, json: {}",
                     QString(component_info_json_bytes));
      continue;
    }

    auto component_info = component_info_json.object();
    if (!component_info.contains("name")) {
      GF_UI_LOG_WARN(
          "illegal gnupg component info. it doesn't have a name, json: {}",
          QString(component_info_json_bytes));
      continue;
    }

    auto* tmp0 = new QTableWidgetItem(component_info["name"].toString());
    tmp0->setTextAlignment(Qt::AlignCenter);
    ui_->componentDetailsTable->setItem(row, 0, tmp0);

    auto* tmp1 = new QTableWidgetItem(component_info["desc"].toString());
    tmp1->setTextAlignment(Qt::AlignCenter);
    ui_->componentDetailsTable->setItem(row, 1, tmp1);

    auto* tmp2 = new QTableWidgetItem(component_info["version"].toString());
    tmp2->setTextAlignment(Qt::AlignCenter);
    ui_->componentDetailsTable->setItem(row, 2, tmp2);

    auto* tmp3 =
        new QTableWidgetItem(component_info["binary_checksum"].toString());
    tmp3->setTextAlignment(Qt::AlignCenter);
    ui_->componentDetailsTable->setItem(row, 3, tmp3);

    auto* tmp4 = new QTableWidgetItem(component_info["path"].toString());
    tmp4->setTextAlignment(Qt::AlignLeft);
    ui_->componentDetailsTable->setItem(row, 4, tmp4);

    row++;
  }

  ui_->componentDetailsTable->resizeColumnsToContents();

  auto directories = Module::ListRTChildKeys(
      "com.bktus.gpgfrontend.module.integrated.gnupg-info-gathering",
      QString("gnupg.dirs"));

  ui_->directoriesDetailsTable->setRowCount(directories.size());

  row = 0;
  for (auto& dir : directories) {
    const auto dir_path = Module::RetrieveRTValueTypedOrDefault(
        "com.bktus.gpgfrontend.module.integrated.gnupg-info-gathering",
        QString("gnupg.dirs.%1").arg(dir), QString{});

    if (dir_path.isEmpty()) continue;

    auto* tmp0 = new QTableWidgetItem(dir);
    tmp0->setTextAlignment(Qt::AlignCenter);
    ui_->directoriesDetailsTable->setItem(row, 0, tmp0);

    auto* tmp1 = new QTableWidgetItem(dir_path);
    tmp1->setTextAlignment(Qt::AlignCenter);
    ui_->directoriesDetailsTable->setItem(row, 1, tmp1);

    row++;
  }

  ui_->directoriesDetailsTable->resizeColumnsToContents();

  // calcualte the total row number of configuration table
  row = 0;
  for (auto& component : components) {
    auto options = Module::ListRTChildKeys(
        "com.bktus.gpgfrontend.module.integrated.gnupg-info-gathering",
        QString("gnupg.components.%1.options").arg(component));
    for (auto& option : options) {
      const auto option_info_json =
          QJsonDocument::fromJson(Module::RetrieveRTValueTypedOrDefault(
              "com.bktus.gpgfrontend.module.integrated.gnupg-info-gathering",
              QString("gnupg.components.%1.options.%2")
                  .arg(component)
                  .arg(option),
              QByteArray{}));

      if (!option_info_json.isObject()) continue;

      auto option_info = option_info_json.object();
      if (!option_info.contains("name") || option_info["flags"] == "1") {
        continue;
      }
      row++;
    }
  }

  ui_->optionDetailsTable->setRowCount(row);

  row = 0;
  QString configuration_group;
  for (auto& component : components) {
    auto options = Module::ListRTChildKeys(
        "com.bktus.gpgfrontend.module.integrated.gnupg-info-gathering",
        QString("gnupg.components.%1.options").arg(component));

    for (auto& option : options) {
      auto option_info_json_bytes = Module::RetrieveRTValueTypedOrDefault(
          "com.bktus.gpgfrontend.module.integrated.gnupg-info-gathering",
          QString("gnupg.components.%1.options.%2").arg(component).arg(option),
          QByteArray{});
      GF_UI_LOG_DEBUG("got gnupg component's option {} info from rt, info: {}",
                      component, option_info_json_bytes);

      auto option_info_json = QJsonDocument::fromJson(option_info_json_bytes);

      if (!option_info_json.isObject()) {
        GF_UI_LOG_WARN("illegal gnupg option info, json: {}",
                       QString(option_info_json_bytes));
        continue;
      }

      auto option_info = option_info_json.object();
      if (!option_info.contains("name")) {
        GF_UI_LOG_WARN(
            "illegal gnupg configuation info. it doesn't have a name, json: {}",
            QString(option_info_json_bytes));
        continue;
      }

      if (option_info["flags"] == "1") {
        configuration_group = option_info["name"].toString();
        continue;
      }

      auto* tmp0 = new QTableWidgetItem(component);
      tmp0->setTextAlignment(Qt::AlignCenter);
      ui_->optionDetailsTable->setItem(row, 0, tmp0);

      auto* tmp1 = new QTableWidgetItem(configuration_group);
      tmp1->setTextAlignment(Qt::AlignCenter);
      ui_->optionDetailsTable->setItem(row, 1, tmp1);

      auto* tmp2 = new QTableWidgetItem(option_info["name"].toString());
      tmp2->setTextAlignment(Qt::AlignCenter);
      ui_->optionDetailsTable->setItem(row, 2, tmp2);

      auto* tmp3 = new QTableWidgetItem(option_info["description"].toString());

      tmp3->setTextAlignment(Qt::AlignLeft);
      ui_->optionDetailsTable->setItem(row, 3, tmp3);

      auto* tmp4 =
          new QTableWidgetItem(option_info["default_value"].toString());
      tmp4->setTextAlignment(Qt::AlignLeft);
      ui_->optionDetailsTable->setItem(row, 4, tmp4);

      auto* tmp5 = new QTableWidgetItem(option_info["value"].toString());
      tmp5->setTextAlignment(Qt::AlignLeft);
      ui_->optionDetailsTable->setItem(row, 5, tmp5);

      row++;
    }
  }
  // ui_->configurationDetailsTable->resizeColumnsToContents();
}
