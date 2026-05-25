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

#include "core/function/GlobalSettingStation.h"
#include "core/module/ModuleManager.h"
#include "core/utils/BuildInfoUtils.h"
#include "ui/UIModuleManager.h"

namespace GpgFrontend::UI {
namespace {

auto CreateBodyLabel(const QString& text, QWidget* parent = nullptr)
    -> QLabel* {
  auto* label = new QLabel(text, parent);
  label->setWordWrap(true);
  label->setTextFormat(Qt::RichText);
  label->setTextInteractionFlags(Qt::TextBrowserInteraction |
                                 Qt::TextSelectableByMouse);
  label->setOpenExternalLinks(true);
  return label;
}

auto CreateValueLabel(const QString& text, QWidget* parent = nullptr)
    -> QLabel* {
  auto* label = new QLabel(text, parent);
  label->setWordWrap(true);
  label->setTextInteractionFlags(Qt::TextSelectableByMouse);
  return label;
}

auto CreateCard(const QString& title, QWidget* content,
                QWidget* parent = nullptr) -> QFrame* {
  auto* frame = new QFrame(parent);
  frame->setObjectName(QStringLiteral("AboutCard"));
  frame->setFrameShape(QFrame::StyledPanel);

  auto* title_label = new QLabel(QStringLiteral("<b>%1</b>").arg(title), frame);
  title_label->setTextFormat(Qt::RichText);
  title_label->setWordWrap(true);

  auto* layout = new QVBoxLayout(frame);
  layout->setContentsMargins(14, 12, 14, 12);
  layout->setSpacing(8);
  layout->addWidget(title_label);
  layout->addWidget(content);

  return frame;
}

auto CreateInfoForm(QWidget* parent = nullptr) -> QFormLayout* {
  auto* form = new QFormLayout(parent);
  form->setRowWrapPolicy(QFormLayout::WrapLongRows);
  form->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
  form->setFormAlignment(Qt::AlignTop);
  form->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
  form->setHorizontalSpacing(18);
  form->setVerticalSpacing(8);
  return form;
}

auto CreateBuildInfoText() -> QString {
  return QStringLiteral(
             "GpgFrontend: %1\n"
             "Qt: %2\n"
             "GPGME: %3\n"
             "Assuan: %4\n"
             "Libarchive: %5\n"
             "OpenSSL: %6\n"
             "Git Branch: %7\n"
             "Git Commit: %8\n"
             "Built at: %9")
      .arg(GetProjectVersion(), GetProjectQtVersion(), GetProjectGpgMEVersion(),
           GetProjectAssuanVersion(), GetProjectLibarchiveVersion(),
           GetProjectOpenSSLVersion(), GetProjectBuildGitBranchName(),
           GetProjectBuildGitCommitHash(),
           QLocale().toString(GetProjectBuildTimestamp()));
}

auto CreateCopyButton(const QString& text, const QString& content,
                      QWidget* parent = nullptr) -> QPushButton* {
  auto* button = new QPushButton(text, parent);

  QObject::connect(button, &QPushButton::clicked, button, [content]() {
    QApplication::clipboard()->setText(content);
  });

  return button;
}

auto CreateScrollArea(QWidget* content, QWidget* parent = nullptr)
    -> QScrollArea* {
  auto* scroll_area = new QScrollArea(parent);
  scroll_area->setWidget(content);
  scroll_area->setWidgetResizable(true);
  scroll_area->setFrameShape(QFrame::NoFrame);
  return scroll_area;
}

}  // namespace

AboutDialog::AboutDialog(const QString& default_tab_name, QWidget* parent)
    : GeneralDialog(typeid(AboutDialog).name(), parent) {
  setWindowTitle(tr("About") + " " + qApp->applicationDisplayName());

  auto* tab_widget = new QTabWidget(this);
  auto* info_tab = new InfoTab(tab_widget);
  auto* translators_tab = new TranslatorsTab(tab_widget);
  auto* status_tab = new StatusTab(tab_widget);

  tab_widget->setDocumentMode(true);

  tab_widget->addTab(info_tab, tr("About"));
  tab_widget->addTab(translators_tab, tr("Translators"));
  tab_widget->addTab(status_tab, tr("Status"));

  Module::TriggerEvent(
      "ABOUT_DIALOG_TABS_MOUNTED",
      {
          {"tab_widget", GFBuffer(RegisterQObject(tab_widget))},
      });

  int default_index = 0;
  for (int i = 0; i < tab_widget->count(); ++i) {
    if (tab_widget->tabText(i) == default_tab_name) {
      default_index = i;
      break;
    }
  }

  if (default_index < tab_widget->count() && default_index >= 0) {
    tab_widget->setCurrentIndex(default_index);
  }

  auto* main_layout = new QVBoxLayout(this);
  main_layout->setContentsMargins(8, 8, 8, 8);
  main_layout->addWidget(tab_widget);
  setLayout(main_layout);
}

void AboutDialog::showEvent(QShowEvent* ev) { QDialog::showEvent(ev); }

InfoTab::InfoTab(QWidget* parent) : QWidget(parent) {
  auto* content = new QWidget(this);
  auto* main_layout = new QVBoxLayout(content);
  main_layout->setContentsMargins(18, 18, 18, 18);
  main_layout->setSpacing(14);

  auto pixmap =
      QPixmap(QStringLiteral(":/icons/gpgfrontend_logo.png"))
          .scaled(112, 112, Qt::KeepAspectRatio, Qt::SmoothTransformation);

  auto* pixmap_label = new QLabel(content);
  pixmap_label->setPixmap(pixmap);
  pixmap_label->setAlignment(Qt::AlignCenter);

  auto* title_label = new QLabel(
      QStringLiteral(
          "<div align=\"center\">"
          "<span style=\"font-size:22px; font-weight:600;\">%1</span>"
          "<br/>"
          "<span style=\"font-size:14px;\">%2</span>"
          "</div>")
          .arg(qApp->applicationDisplayName(), GetProjectVersion()),
      content);
  title_label->setTextFormat(Qt::RichText);
  title_label->setAlignment(Qt::AlignCenter);
  title_label->setTextInteractionFlags(Qt::TextSelectableByMouse);

  auto* tagline_label = CreateBodyLabel(
      QStringLiteral("<div align=\"center\">%1</div>")
          .arg(tr("A user-friendly OpenPGP tool for encryption, signing, and "
                  "key management.")),
      content);
  tagline_label->setAlignment(Qt::AlignCenter);

  main_layout->addWidget(pixmap_label);
  main_layout->addWidget(title_label);
  main_layout->addWidget(tagline_label);

  auto* developer_label = CreateBodyLabel(
      QStringLiteral(
          "%1<br/><br/>"
          "<a href=\"https://github.com/saturneric/GpgFrontend/issues\">%2</a>"
          "<br/>"
          "<a href=\"https://gpgfrontend.bktus.com/overview/contact/\">%3</a>"
          "<br/>"
          "<a href=\"mailto:eric@bktus.com\">eric@bktus.com</a>")
          .arg(tr("Developed and maintained by Saturneric."),
               tr("Report an issue on GitHub"), tr("Contact the developer")),
      content);

  main_layout->addWidget(CreateCard(tr("Developer"), developer_label, content));

  auto* build_widget = new QWidget(content);
  auto* build_layout = new QVBoxLayout(build_widget);
  build_layout->setContentsMargins(0, 0, 0, 0);
  build_layout->setSpacing(10);

  auto* build_form_widget = new QWidget(build_widget);
  auto* build_form = CreateInfoForm(build_form_widget);
  build_form_widget->setLayout(build_form);

  build_form->addRow(
      tr("Qt:"), CreateValueLabel(GetProjectQtVersion(), build_form_widget));
  build_form->addRow(tr("GPGME:"), CreateValueLabel(GetProjectGpgMEVersion(),
                                                    build_form_widget));
  build_form->addRow(tr("Assuan:"), CreateValueLabel(GetProjectAssuanVersion(),
                                                     build_form_widget));
  build_form->addRow(
      tr("Libarchive:"),
      CreateValueLabel(GetProjectLibarchiveVersion(), build_form_widget));
  build_form->addRow(
      tr("OpenSSL:"),
      CreateValueLabel(GetProjectOpenSSLVersion(), build_form_widget));
  build_form->addRow(
      tr("Git Branch:"),
      CreateValueLabel(GetProjectBuildGitBranchName(), build_form_widget));
  build_form->addRow(
      tr("Git Commit:"),
      CreateValueLabel(GetProjectBuildGitCommitHash(), build_form_widget));
  build_form->addRow(
      tr("Built at:"),
      CreateValueLabel(QLocale().toString(GetProjectBuildTimestamp()),
                       build_form_widget));

  auto* copy_button = CreateCopyButton(tr("Copy Build Information"),
                                       CreateBuildInfoText(), build_widget);

  build_layout->addWidget(build_form_widget);
  build_layout->addWidget(copy_button, 0, Qt::AlignRight);

  main_layout->addWidget(
      CreateCard(tr("Build Information"), build_widget, content));

  main_layout->addStretch();

  auto* outer_layout = new QVBoxLayout(this);
  outer_layout->setContentsMargins(0, 0, 0, 0);
  outer_layout->addWidget(content);
  setLayout(outer_layout);

  setObjectName(QStringLiteral("InfoTab"));
}

TranslatorsTab::TranslatorsTab(QWidget* parent) : QWidget(parent) {
  auto* main_layout = new QVBoxLayout(this);
  main_layout->setContentsMargins(18, 18, 18, 18);
  main_layout->setSpacing(12);

  auto* title_label = CreateBodyLabel(
      QStringLiteral("<b>%1</b>").arg(tr("Thanks to all translators")), this);

  auto* browser = new QTextBrowser(this);
  browser->setOpenExternalLinks(true);
  browser->setReadOnly(true);

  QFile translators_file(QStringLiteral(":/TRANSLATORS"));
  if (translators_file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    browser->setPlainText(QString::fromUtf8(translators_file.readAll()));
  } else {
    browser->setPlainText(tr("Translator information is not available."));
  }

  auto* notice_label = CreateBodyLabel(
      tr("If you find a translation issue or want to help improve "
         "localization, please contact the developer or submit a "
         "contribution."),
      this);

  main_layout->addWidget(title_label);
  main_layout->addWidget(browser, 1);
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

  auto* content = new QWidget(this);
  auto* main_layout = new QVBoxLayout(content);
  main_layout->setContentsMargins(18, 18, 18, 18);
  main_layout->setSpacing(14);

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

  const QString portable_mode_str =
      portable_mode ? tr("Portable Mode") : tr("Installed Mode");

  const QString self_check_str =
      self_check ? tr("Self-Check Active") : tr("Self-Check Disabled");

  const QString gnupg_offline_mode_str =
      gnupg_offline_mode ? tr("Active") : tr("Disabled");

  const QString pinentry_program_path_str = pinentry_program_path.isEmpty()
                                                ? tr("Default Pinentry Program")
                                                : pinentry_program_path;

  auto* status_widget = new QWidget(content);
  auto* status_form = CreateInfoForm(status_widget);
  status_widget->setLayout(status_form);

  status_form->addRow(tr("Security Level:"),
                      CreateValueLabel(secure_level_str, status_widget));
  status_form->addRow(tr("Running Mode:"),
                      CreateValueLabel(portable_mode_str, status_widget));
  status_form->addRow(tr("Self-Check Status:"),
                      CreateValueLabel(self_check_str, status_widget));
  status_form->addRow(tr("GnuPG Offline Mode:"),
                      CreateValueLabel(gnupg_offline_mode_str, status_widget));
  status_form->addRow(
      tr("Pinentry Program Path:"),
      CreateValueLabel(pinentry_program_path_str, status_widget));
  main_layout->addWidget(
      CreateCard(tr("Application Status"), status_widget, content));

  auto* tip_label = CreateBodyLabel(
      tr("Tip: These values reflect the current startup environment and may "
         "help when reporting issues."),
      content);

  main_layout->addStretch();
  main_layout->addWidget(tip_label);

  auto* outer_layout = new QVBoxLayout(this);
  outer_layout->setContentsMargins(0, 0, 0, 0);
  outer_layout->addWidget(CreateScrollArea(content, this));
  setLayout(outer_layout);
}

}  // namespace GpgFrontend::UI
