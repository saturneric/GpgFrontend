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

#include "AboutDialog.h"

#include <openssl/opensslv.h>

#include "core/module/ModuleManager.h"
#include "core/utils/BuildInfoUtils.h"
#include "ui/dialog/help/GnupgTab.h"

namespace GpgFrontend::UI {

AboutDialog::AboutDialog(int defaultIndex, QWidget* parent)
    : GeneralDialog(typeid(AboutDialog).name(), parent) {
  this->setWindowTitle(tr("About") + " " + qApp->applicationName());

  auto* tab_widget = new QTabWidget;
  auto* info_tab = new InfoTab();
  auto* gnupg_tab = new GnupgTab();
  auto* translators_tab = new TranslatorsTab();
  update_tab_ = new UpdateTab();

  tab_widget->addTab(info_tab, tr("About GpgFrontend"));
  tab_widget->addTab(gnupg_tab, tr("GnuPG"));
  tab_widget->addTab(translators_tab, tr("Translators"));
  tab_widget->addTab(update_tab_, tr("Update"));

  connect(tab_widget, &QTabWidget::currentChanged, this,
          [&](int index) { GF_UI_LOG_DEBUG("current index: {}", index); });

  if (defaultIndex < tab_widget->count() && defaultIndex >= 0) {
    tab_widget->setCurrentIndex(defaultIndex);
  }

  auto* button_box = new QDialogButtonBox(QDialogButtonBox::Ok);
  connect(button_box, &QDialogButtonBox::accepted, this, &AboutDialog::close);

  auto* main_layout = new QVBoxLayout;
  main_layout->addWidget(tab_widget);
  main_layout->addWidget(button_box);
  setLayout(main_layout);

  this->resize(520, 620);
  this->setMinimumWidth(450);
  this->show();
}

void AboutDialog::showEvent(QShowEvent* ev) { QDialog::showEvent(ev); }

InfoTab::InfoTab(QWidget* parent) : QWidget(parent) {
  const auto gpgme_version = Module::RetrieveRTValueTypedOrDefault<>(
      "core", "gpgme.version", QString{"2.0.0"});
  GF_UI_LOG_DEBUG("got gpgme version from rt: {}", gpgme_version);

  auto pixmap = QPixmap(":/icons/gpgfrontend_logo.png");
  pixmap = pixmap.scaled(128, 128);

  auto text =
      "<center><h2>" + qApp->applicationName() + "</h2></center>" +
      "<center><b>" + qApp->applicationVersion() + "</b></center>" +
      "<center>" + GetProjectBuildGitVersion() + "</center>" + "<br><center>" +
      tr("GpgFrontend is an easy-to-use, compact, cross-platform, "
         "and installation-free GnuPG Frontend."
         "It visualizes most of the common operations of GnuPG."
         "GpgFrontend is licensed under the GPLv3") +
      "<br><br>"
      "<b>" +
      tr("Developer:") + "</b><br>" + "Saturneric" + "<br><br>" +
      tr("If you have any questions or suggestions, raise an issue at") +
      "<br/>"
      " <a href=\"https://github.com/saturneric/GpgFrontend\">GitHub</a> " +
      tr("or send a mail to my mailing list at") + " <a " +
      "href=\"mailto:eric@bktus.com\">eric@bktus.com</a>." + "<br><br> " +
      tr("Built with Qt") + " " + qVersion() + ", " + OPENSSL_VERSION_TEXT +
      " " + tr("and") + " " + "GPGME" + " " + gpgme_version + "<br>" +
      tr("Built at") + " " + QLocale().toString(GetProjectBuildTimestamp()) +
      "</center>";

  auto* layout = new QGridLayout();
  auto* pixmap_label = new QLabel();
  pixmap_label->setPixmap(pixmap);
  layout->addWidget(pixmap_label, 0, 0, 1, -1, Qt::AlignCenter);
  auto* about_label = new QLabel();
  about_label->setText(text);
  about_label->setWordWrap(true);
  about_label->setOpenExternalLinks(true);
  layout->addWidget(about_label, 1, 0, 1, -1);
  layout->addItem(
      new QSpacerItem(20, 10, QSizePolicy::Minimum, QSizePolicy::Fixed), 2, 1,
      1, 1);

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

UpdateTab::UpdateTab(QWidget* parent) : QWidget(parent) {
  auto* layout = new QGridLayout();

  current_version_ = GetProjectVersion();

  auto* tips_label = new QLabel();
  tips_label->setText(
      "<center>" +
      tr("It is recommended that you always check the version "
         "of GpgFrontend and upgrade to the latest version.") +
      "</center><center>" +
      tr("New versions not only represent new features, but "
         "also often represent functional and security fixes.") +
      "</center>");
  tips_label->setWordWrap(true);

  current_version_label_ = new QLabel();
  current_version_label_->setText("<center>" + tr("Current Version") +
                                  tr(": ") + "<b>" + current_version_ +
                                  "</b></center>");
  current_version_label_->setWordWrap(true);

  latest_version_label_ = new QLabel();
  latest_version_label_->setWordWrap(true);

  upgrade_label_ = new QLabel();
  upgrade_label_->setWordWrap(true);
  upgrade_label_->setOpenExternalLinks(true);
  upgrade_label_->setHidden(true);

  pb_ = new QProgressBar();
  pb_->setRange(0, 0);
  pb_->setTextVisible(false);

  layout->addWidget(tips_label, 1, 0, 1, -1);
  layout->addWidget(current_version_label_, 2, 0, 1, -1);
  layout->addWidget(latest_version_label_, 3, 0, 1, -1);
  layout->addWidget(upgrade_label_, 4, 0, 1, -1);
  layout->addWidget(pb_, 5, 0, 1, -1);
  layout->addItem(
      new QSpacerItem(20, 10, QSizePolicy::Minimum, QSizePolicy::Fixed), 2, 1,
      1, 1);

  setLayout(layout);
}

void UpdateTab::showEvent(QShowEvent* event) {
  QWidget::showEvent(event);
  GF_UI_LOG_DEBUG("loading version loading info from rt");

  auto is_loading_done = Module::RetrieveRTValueTypedOrDefault<>(
      "com.bktus.gpgfrontend.module.integrated.version-checking",
      "version.loading_done", false);

  if (!is_loading_done) {
    Module::ListenRTPublishEvent(
        this, "com.bktus.gpgfrontend.module.integrated.version-checking",
        "version.loading_done",
        [=](Module::Namespace, Module::Key, int, std::any) {
          GF_UI_LOG_DEBUG(
              "versionchecking version.loading_done changed, calling slot "
              "version upgrade");
          this->slot_show_version_status();
        });
    Module::TriggerEvent("CHECK_APPLICATION_VERSION");
  } else {
    slot_show_version_status();
  }
}

void UpdateTab::slot_show_version_status() {
  GF_UI_LOG_DEBUG("loading version info from rt");
  this->pb_->setHidden(true);

  auto is_loading_done = Module::RetrieveRTValueTypedOrDefault<>(
      "com.bktus.gpgfrontend.module.integrated.version-checking",
      "version.loading_done", false);

  if (!is_loading_done) {
    GF_UI_LOG_DEBUG("version info loading havn't been done yet.");
    return;
  }

  auto is_need_upgrade = Module::RetrieveRTValueTypedOrDefault<>(
      "com.bktus.gpgfrontend.module.integrated.version-checking",
      "version.need_upgrade", false);

  auto is_current_a_withdrawn_version = Module::RetrieveRTValueTypedOrDefault<>(
      "com.bktus.gpgfrontend.module.integrated.version-checking",
      "version.current_a_withdrawn_version", false);

  auto is_current_version_released = Module::RetrieveRTValueTypedOrDefault<>(
      "com.bktus.gpgfrontend.module.integrated.version-checking",
      "version.current_version_released", false);

  auto latest_version = Module::RetrieveRTValueTypedOrDefault<>(
      "com.bktus.gpgfrontend.module.integrated.version-checking",
      "version.latest_version", QString{});

  latest_version_label_->setText("<center><b>" +
                                 tr("Latest Version From Github") + ": " +
                                 latest_version + "</b></center>");

  if (is_need_upgrade) {
    upgrade_label_->setText(
        "<center>" +
        tr("The current version is less than the latest version on "
           "github.") +
        "</center><center>" + tr("Please click") +
        " <a "
        "href=\"https://www.gpgfrontend.bktus.com/#/downloads\">" +
        tr("Here") + "</a> " + tr("to download the latest stable version.") +
        "</center>");
    upgrade_label_->show();
  } else if (is_current_a_withdrawn_version) {
    upgrade_label_->setText(
        "<center>" +
        tr("This version has serious problems and has been withdrawn. "
           "Please stop using it immediately.") +
        "</center><center>" + tr("Please click") +
        " <a "
        "href=\"https://github.com/saturneric/GpgFrontend/releases\">" +
        tr("Here") + "</a> " + tr("to download the latest stable version.") +
        "</center>");
    upgrade_label_->show();
  } else if (!is_current_version_released) {
    upgrade_label_->setText(
        "<center>" +
        tr("This version has not been released yet, it may be a beta "
           "version. If you are not a tester and care about version "
           "stability, please do not use this version.") +
        "</center><center>" + tr("Please click") +
        " <a "
        "href=\"https://www.gpgfrontend.bktus.com/#/downloads\">" +
        tr("Here") + "</a> " + tr("to download the latest stable version.") +
        "</center>");
    upgrade_label_->show();
  }
}

}  // namespace GpgFrontend::UI
