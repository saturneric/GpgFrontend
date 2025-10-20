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

#include "core/utils/BuildInfoUtils.h"
#include "ui/UIModuleManager.h"

namespace GpgFrontend::UI {

AboutDialog::AboutDialog(const QString& default_tab_name, QWidget* parent)
    : GeneralDialog(typeid(AboutDialog).name(), parent) {
  this->setWindowTitle(tr("About") + " " + qApp->applicationName());

  auto* tab_widget = new QTabWidget;
  auto* info_tab = new InfoTab();
  auto* translators_tab = new TranslatorsTab();
  auto* status_tab = new StatusTab();

  tab_widget->setDocumentMode(true);

  tab_widget->addTab(info_tab, tr("About GpgFrontend"));
  tab_widget->addTab(translators_tab, tr("Translators"));
  tab_widget->addTab(status_tab, tr("Status"));

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

  this->show();
  this->raise();
  this->activateWindow();
}

void AboutDialog::showEvent(QShowEvent* ev) { QDialog::showEvent(ev); }

InfoTab::InfoTab(QWidget* parent) : QWidget(parent) {
  auto pixmap =
      QPixmap(":/icons/gpgfrontend_logo.png")
          .scaled(128, 128, Qt::KeepAspectRatio, Qt::SmoothTransformation);
  auto* pixmap_label = new QLabel;
  pixmap_label->setPixmap(pixmap);
  pixmap_label->setMinimumSize({150, 150});
  pixmap_label->setAlignment(Qt::AlignCenter);

  QString app_info = QString(R"(
    <div align="center">
      <span style="font-size:20px; font-weight:bold;">%1</span>
      <br/> <br/>
      <span style="font-size:16px;">%2</span>
    </div>
)")
                         .arg(qApp->applicationDisplayName())
                         .arg(GetProjectVersion());

  auto* app_info_label = new QLabel(app_info);
  app_info_label->setTextFormat(Qt::RichText);
  app_info_label->setAlignment(Qt::AlignCenter);
  app_info_label->setWordWrap(true);

  auto* sep = new QFrame;
  sep->setFrameShape(QFrame::HLine);
  sep->setFrameShadow(QFrame::Sunken);

  auto developer_info =
      QString(R"(
      <b>%1</b>Saturneric<br><br>
      %2
      <a href="https://github.com/saturneric/GpgFrontend/issues">GitHub</a>
      %3
      <a href="mailto:eric@bktus.com">eric@bktus.com</a>
  )")
          .arg(tr("Developer:") + " ")
          .arg(
              tr("If you have any questions or suggestions, raise an issue at"))
          .arg(tr("or send a mail to my private email at"));

  auto* dev_group = new QGroupBox(tr("Developer"));
  auto* dev_layout = new QVBoxLayout;
  auto* dev_label = new QLabel(developer_info);
  dev_label->setTextFormat(Qt::RichText);
  dev_label->setWordWrap(true);
  dev_label->setOpenExternalLinks(true);
  dev_layout->addWidget(dev_label);
  dev_group->setLayout(dev_layout);

  auto* build_group = new QGroupBox(tr("Build Information"));
  auto* build_form = new QFormLayout();

  build_form->addRow(tr("Qt"), new QLabel(GetProjectQtVersion()));
  build_form->addRow(tr("GPGME"), new QLabel(GetProjectGpgMEVersion()));
  build_form->addRow(tr("Assuan"), new QLabel(GetProjectAssuanVersion()));
  build_form->addRow(tr("Libarchive"),
                     new QLabel(GetProjectLibarchiveVersion()));
  build_form->addRow(tr("OpenSSL"), new QLabel(GetProjectOpenSSLVersion()));
  build_form->addRow(tr("Git Branch:"),
                     new QLabel(GetProjectBuildGitBranchName()));
  build_form->addRow(tr("Git Commit:"),
                     new QLabel(GetProjectBuildGitCommitHash()));
  build_form->addRow(
      tr("Built at:"),
      new QLabel(QLocale().toString(GetProjectBuildTimestamp())));

  build_form->setRowWrapPolicy(QFormLayout::DontWrapRows);
  build_form->setFieldGrowthPolicy(QFormLayout::FieldsStayAtSizeHint);
  build_form->setFormAlignment(Qt::AlignHCenter | Qt::AlignTop);
  build_form->setLabelAlignment(Qt::AlignLeft);
  build_group->setLayout(build_form);

  auto* main_layout = new QVBoxLayout(this);
  main_layout->addWidget(pixmap_label);
  main_layout->addSpacing(15);
  main_layout->addWidget(app_info_label);
  main_layout->addWidget(sep);
  main_layout->addWidget(dev_group);
  main_layout->addWidget(build_group, 1);
  main_layout->addStretch();

  setObjectName("InfoTab");
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

StatusTab::StatusTab(QWidget* parent) : QWidget(parent) {
  const int secure_level = qApp->property("GFSecureLevel").toInt();
  const bool portable_mode = qApp->property("GFPortableMode").toBool();
  const bool self_check = qApp->property("GFSelfCheck").toBool();
  const bool gnupg_offline_mode = qApp->property("GFGnuPGOfflineMode").toBool();
  const QString pinentry_program_path =
      qApp->property("GFPinentryProgramPath").toString();

  auto* main_layout = new QVBoxLayout(this);
  auto* status_form = new QFormLayout();

  status_form->setRowWrapPolicy(QFormLayout::DontWrapRows);
  status_form->setFieldGrowthPolicy(QFormLayout::FieldsStayAtSizeHint);
  status_form->setFormAlignment(Qt::AlignHCenter | Qt::AlignTop);
  status_form->setLabelAlignment(Qt::AlignLeft);

  const QString secure_level_str = [secure_level]() {
    switch (secure_level) {
      case 0:
        return tr("Default");
      case 1:
        return tr("Standard");
      case 2:
        return tr("Enhanced");
      case 3:
        return tr("High");
      default:
        return tr("Unknown");
    }
  }();

  // Running mode string
  const QString portable_mode_str =
      portable_mode ? tr("Portable Mode") : tr("Installed Mode");

  // Self-check string
  const QString self_check_str =
      self_check ? tr("Self-Check Active") : tr("Self-Check Disabled");

  // GnuPG Offline Mode string
  const QString gnupg_offline_mode_str =
      gnupg_offline_mode ? tr("Active") : tr("Disabled");

  // Pinentry Program Path string
  const QString pinentry_program_path_str = pinentry_program_path.isEmpty()
                                                ? tr("Default Pinentry Program")
                                                : pinentry_program_path;

  // Add rows to form
  status_form->addRow(tr("Security Level:"), new QLabel(secure_level_str));
  status_form->addRow(tr("Running Mode:"), new QLabel(portable_mode_str));
  status_form->addRow(tr("Self-Check Status:"), new QLabel(self_check_str));
  status_form->addRow(tr("GnuPG Offline Mode:"),
                      new QLabel(gnupg_offline_mode_str));
  status_form->addRow(tr("Pinentry Program Path:"),
                      new QLabel(pinentry_program_path_str));

  auto* tip_label = new QLabel(tr(
      "Tips: The above parameters reflect how the application was started. "));
  tip_label->setWordWrap(true);

  main_layout->addLayout(status_form);
  main_layout->addStretch();
  main_layout->addWidget(tip_label);
  setLayout(main_layout);
}

}  // namespace GpgFrontend::UI
