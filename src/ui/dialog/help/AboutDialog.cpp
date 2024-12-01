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

#include "AboutDialog.h"

#include <openssl/opensslv.h>

#include "core/module/ModuleManager.h"
#include "core/utils/BuildInfoUtils.h"
#include "ui/UIModuleManager.h"

namespace GpgFrontend::UI {

AboutDialog::AboutDialog(const QString& default_tab_name, QWidget* parent)
    : GeneralDialog(typeid(AboutDialog).name(), parent) {
  this->setWindowTitle(tr("About") + " " + qApp->applicationName());

  auto* tab_widget = new QTabWidget;
  auto* info_tab = new InfoTab();
  auto* translators_tab = new TranslatorsTab();

  tab_widget->setDocumentMode(true);

  tab_widget->addTab(info_tab, tr("About GpgFrontend"));
  tab_widget->addTab(translators_tab, tr("Translators"));

  auto entries =
      UIModuleManager::GetInstance().QueryMountedEntries("AboutDialogTabs");

  for (const auto& entry : entries) {
    auto* widget = entry.GetWidget();
    if (widget != nullptr) {
      tab_widget->addTab(widget,
                         entry.GetMetaDataByDefault("TabTitle", tr("Unnamed")));
    }
  }

  int default_index = 0;
  for (int i = 0; i < tab_widget->count(); i++) {
    if (tab_widget->tabText(i) == default_tab_name) {
      default_index = i;
    }
  }

  if (default_index < tab_widget->count() && default_index >= 0) {
    tab_widget->setCurrentIndex(default_index);
  }

  auto* main_layout = new QVBoxLayout;
  main_layout->addWidget(tab_widget);
  main_layout->setContentsMargins(QMargins{5, 0, 5, 0});
  setLayout(main_layout);

  this->resize(520, 620);
  this->setMinimumWidth(450);
  this->show();
}

void AboutDialog::showEvent(QShowEvent* ev) { QDialog::showEvent(ev); }

InfoTab::InfoTab(QWidget* parent) : QWidget(parent) {
  const auto gpgme_version = Module::RetrieveRTValueTypedOrDefault<>(
      "core", "gpgme.version", QString{"2.0.0"});

  auto pixmap = QPixmap(":/icons/gpgfrontend_logo.png");
  pixmap =
      pixmap.scaled(128, 128, Qt::KeepAspectRatio, Qt::SmoothTransformation);

  QString app_info = "<h2 style='text-align:center;'>" +
                     qApp->applicationDisplayName() + "</h2>" + "<center><b>" +
                     GetProjectVersion() + "</b></center>" + "<center>" +
                     GetProjectBuildGitVersion() + "</center>";

  QString developer_info =
      "<b>" + tr("Developer:") + "</b> Saturneric" + "<br /><br />" +
      tr("If you have any questions or suggestions, raise an issue at") +
      " <a href=\"https://github.com/saturneric/GpgFrontend\">GitHub</a> " +
      tr("or send a mail to my mailing list at") +
      " <a href=\"mailto:eric@bktus.com\">eric@bktus.com</a>.";

  QString build_info = tr("Built with Qt") + " " + qVersion() + ", " +
                       OPENSSL_VERSION_TEXT + " " + tr("and") + " GPGME " +
                       gpgme_version + "<br>" + tr("Built at") + " " +
                       QLocale().toString(GetProjectBuildTimestamp());

  auto* layout = new QVBoxLayout();

  auto* pixmap_label = new QLabel();
  pixmap_label->setPixmap(pixmap);
  pixmap_label->setAlignment(Qt::AlignCenter);
  layout->addWidget(pixmap_label);

  auto* app_info_label = new QLabel(app_info);
  app_info_label->setWordWrap(true);
  app_info_label->setAlignment(Qt::AlignCenter);
  layout->addWidget(app_info_label);

  auto* dev_info_group = new QGroupBox(tr("Developer Information"));
  auto* dev_layout = new QVBoxLayout();
  auto* dev_info_label = new QLabel(developer_info);
  dev_info_label->setWordWrap(true);
  dev_info_label->setOpenExternalLinks(true);
  dev_layout->addWidget(dev_info_label);
  dev_info_group->setLayout(dev_layout);
  layout->addWidget(dev_info_group);

  auto* build_info_group = new QGroupBox(tr("Build Information"));
  auto* build_layout = new QVBoxLayout();
  auto* build_info_label = new QLabel(build_info);
  build_info_label->setWordWrap(true);
  build_layout->addWidget(build_info_label);
  build_info_group->setLayout(build_layout);
  layout->addWidget(build_info_group);

  setLayout(layout);
}

TranslatorsTab::TranslatorsTab(QWidget* parent) : QWidget(parent) {
  QFile translators_file(":/TRANSLATORS");

  translators_file.open(QIODevice::ReadOnly);
  auto* label = new QLabel(translators_file.readAll());
  auto* main_layout = new QVBoxLayout(this);
  main_layout->addWidget(label);
  main_layout->addStretch();

  auto* notice_label = new QLabel(
      tr("If you think there are any problems with the translation, why not "
         "participate in the translation work? If you want to participate, "
         "please read the document or contact me via email."),
      this);
  notice_label->setWordWrap(true);
  main_layout->addWidget(notice_label);

  setLayout(main_layout);
}

}  // namespace GpgFrontend::UI
