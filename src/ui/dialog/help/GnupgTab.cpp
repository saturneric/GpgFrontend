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

#include <nlohmann/json.hpp>

#include "core/module/ModuleManager.h"
#include "ui_GnuPGInfo.h"

GpgFrontend::UI::GnupgTab::GnupgTab(QWidget* parent)
    : QWidget(parent),
      ui_(GpgFrontend::SecureCreateSharedObject<Ui_GnuPGInfo>()) {
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
  configurations_column_titles << _("Component") << _("Group") << _("Key")
                               << _("Description") << _("Default Value")
                               << _("Value");

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
    auto component_info_json = Module::RetrieveRTValueTypedOrDefault(
        "com.bktus.gpgfrontend.module.integrated.gnupg-info-gathering",
        QString("gnupg.components.%1").arg(component), QString{});
    GF_UI_LOG_DEBUG("got gnupg component {} info from rt, info: {}", component,
                    component_info_json);

    auto component_info =
        nlohmann::json::parse(component_info_json.toStdString());
    if (!component_info.contains("name")) {
      GF_UI_LOG_WARN("illegal gnupg component info, json: {}",
                     component_info_json);
      continue;
    }

    auto* tmp0 = new QTableWidgetItem(
        QString::fromStdString(component_info.value("name", "")));
    tmp0->setTextAlignment(Qt::AlignCenter);
    ui_->componentDetailsTable->setItem(row, 0, tmp0);

    auto* tmp1 = new QTableWidgetItem(
        QString::fromStdString(component_info.value("desc", "")));
    tmp1->setTextAlignment(Qt::AlignCenter);
    ui_->componentDetailsTable->setItem(row, 1, tmp1);

    auto* tmp2 = new QTableWidgetItem(
        QString::fromStdString(component_info.value("version", "")));
    tmp2->setTextAlignment(Qt::AlignCenter);
    ui_->componentDetailsTable->setItem(row, 2, tmp2);

    auto* tmp3 = new QTableWidgetItem(
        QString::fromStdString(component_info.value("binary_checksum", "")));
    tmp3->setTextAlignment(Qt::AlignCenter);
    ui_->componentDetailsTable->setItem(row, 3, tmp3);

    auto* tmp4 = new QTableWidgetItem(
        QString::fromStdString(component_info.value("path", "")));
    tmp4->setTextAlignment(Qt::AlignLeft);
    ui_->componentDetailsTable->setItem(row, 4, tmp4);

    row++;
  }

  ui_->componentDetailsTable->resizeColumnsToContents();

  // calcualte the total row number of configuration table
  row = 0;
  for (auto& component : components) {
    auto options = Module::ListRTChildKeys(
        "com.bktus.gpgfrontend.module.integrated.gnupg-info-gathering",
        QString("gnupg.components.%1.options").arg(component));
    for (auto& option : options) {
      const auto option_info = nlohmann::json::parse(
          Module::RetrieveRTValueTypedOrDefault(
              "com.bktus.gpgfrontend.module.integrated.gnupg-info-gathering",
              QString("gnupg.components.%1.options.%2")
                  .arg(component)
                  .arg(option),
              QString{})
              .toStdString());
      if (!option_info.contains("name") ||
          option_info.value("flags", "1") == "1") {
        continue;
      }
      row++;
    }
  }
  ui_->configurationDetailsTable->setRowCount(row);

  row = 0;
  QString configuration_group;
  for (auto& component : components) {
    auto options = Module::ListRTChildKeys(
        "com.bktus.gpgfrontend.module.integrated.gnupg-info-gathering",
        QString("gnupg.components.%1.options").arg(component));

    for (auto& option : options) {
      auto option_info_json = Module::RetrieveRTValueTypedOrDefault(
          "com.bktus.gpgfrontend.module.integrated.gnupg-info-gathering",
          QString("gnupg.components.%1.options.%2")
              .arg(component)
              .arg(option),
          QString{});
      GF_UI_LOG_DEBUG("got gnupg component's option {} info from rt, info: {}",
                      component, option_info_json);

      auto option_info = nlohmann::json::parse(option_info_json.toStdString());

      if (!option_info.contains("name")) {
        GF_UI_LOG_WARN("illegal gnupg configuation info, json: {}",
                       option_info_json);
        continue;
      }

      if (option_info.value("flags", "1") == "1") {
        configuration_group =
            QString::fromStdString(option_info.value("name", ""));
        continue;
      }

      auto* tmp0 = new QTableWidgetItem(component);
      tmp0->setTextAlignment(Qt::AlignCenter);
      ui_->configurationDetailsTable->setItem(row, 0, tmp0);

      auto* tmp1 = new QTableWidgetItem(configuration_group);
      tmp1->setTextAlignment(Qt::AlignCenter);
      ui_->configurationDetailsTable->setItem(row, 1, tmp1);

      auto* tmp2 = new QTableWidgetItem(
          QString::fromStdString(option_info.value("name", "")));
      tmp2->setTextAlignment(Qt::AlignCenter);
      ui_->configurationDetailsTable->setItem(row, 2, tmp2);

      auto* tmp3 = new QTableWidgetItem(
          QString::fromStdString(option_info.value("description", "")));

      tmp3->setTextAlignment(Qt::AlignLeft);
      ui_->configurationDetailsTable->setItem(row, 3, tmp3);

      auto* tmp4 = new QTableWidgetItem(
          QString::fromStdString(option_info.value("default_value", "")));
      tmp4->setTextAlignment(Qt::AlignLeft);
      ui_->configurationDetailsTable->setItem(row, 4, tmp4);

      auto* tmp5 = new QTableWidgetItem(
          QString::fromStdString(option_info.value("value", "")));
      tmp5->setTextAlignment(Qt::AlignLeft);
      ui_->configurationDetailsTable->setItem(row, 5, tmp5);

      row++;
    }
  }
  // ui_->configurationDetailsTable->resizeColumnsToContents();
}
