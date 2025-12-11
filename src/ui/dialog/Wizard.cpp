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

#include "Wizard.h"

#include "core/function/GlobalSettingStation.h"

namespace GpgFrontend::UI {

Wizard::Wizard(QWidget* parent) : QWizard(parent) {
  setPage(kPAGE_INTRO, new IntroPage(this));
  setPage(kPAGE_CHOOSE, new ChoosePage(this));
  setPage(kPAGE_CONCLUSION, new ConclusionPage(this));
#ifndef Q_WS_MAC
  setWizardStyle(ModernStyle);
#endif
  setWindowTitle(tr("First Start Wizard"));

  setPixmap(QWizard::LogoPixmap,
            QPixmap(":/icons/gpgfrontend_logo.png").scaled(64, 64));

  int next_page_id = GetSettings().value("wizard.next_page", -1).toInt();
  setStartId(next_page_id);

  connect(this, &Wizard::accepted, this, &Wizard::slot_wizard_accepted);
}

void Wizard::slot_wizard_accepted() {
  auto settings = GetSettings();
  settings.setValue("wizard/show_wizard", !field("showWizard").toBool());
  settings.setValue("network/prohibit_update_checking",
                    !field("checkUpdate").toBool());
}

IntroPage::IntroPage(QWidget* parent) : QWizardPage(parent) {
  setTitle(tr("Welcome to GpgFrontend"));
  setSubTitle(tr("Your OpenPGP encryption companion."));

  // Intro text
  auto* intro_label = new QLabel(
      tr("<b>GpgFrontend</b> is a free, user-friendly, and cross-platform tool "
         "for "
         "OpenPGP encryption, decryption, and signing of text and files.") +
      "<br><br>" + tr("To get started, take a quick look at the") +
      " <a href='https://gpgfrontend.bktus.com/overview/glance'>" +
      tr("Overview Page") + "</a>." + " " +
      tr("It will open in your default browser."));
  intro_label->setTextFormat(Qt::RichText);
  intro_label->setTextInteractionFlags(Qt::TextBrowserInteraction);
  intro_label->setOpenExternalLinks(true);
  intro_label->setWordWrap(true);

  // Language info
  auto* lang_label =
      new QLabel(tr("GpgFrontend will automatically use the language of your "
                    "system if supported. "
                    "You can change the language later in settings."));
  lang_label->setWordWrap(true);

  // Layout
  auto* layout = new QVBoxLayout;
  layout->addWidget(intro_label);
  layout->addSpacing(20);
  layout->addWidget(lang_label);
  layout->addStretch();
  setLayout(layout);
}

auto IntroPage::nextId() const -> int { return Wizard::kPAGE_CHOOSE; }

ChoosePage::ChoosePage(QWidget* parent) : QWizardPage(parent) {
  setTitle(tr("Welcome to GpgFrontend"));
  setSubTitle(tr("These quick guides will help you get started."));

  auto* layout = new QVBoxLayout;

  // Section 1: Fundamental Concepts
  auto* concepts_title = new QLabel(tr("<b>New to encryption?</b>"));
  auto* concepts_desc = new QLabel(
      tr("Understand the basics of OpenPGP and how GpgFrontend works."));
  auto* concepts_link = new QLabel(
      "<a "
      "href=\"https://gpgfrontend.bktus.com/guides/fundamental-concepts/\">" +
      tr("Fundamental Concepts") + "</a>");

  // Section 2: Text operations
  auto* textops_title = new QLabel(tr("<b>Working with text?</b>"));
  auto* textops_desc = new QLabel(
      tr("Learn how to encrypt, decrypt, sign, and verify text messages."));
  auto* textops_link = new QLabel(
      "<a href=\"https://gpgfrontend.bktus.com/guides/text-opetations/\">" +
      tr("Text Operations Guide") + "</a>");

  // Section 3: File operations
  auto* fileops_title = new QLabel(tr("<b>Working with files?</b>"));
  auto* fileops_desc =
      new QLabel(tr("Learn how to encrypt, sign, and verify files securely."));
  auto* fileops_link = new QLabel(
      "<a href=\"https://gpgfrontend.bktus.com/guides/file-operations/\">" +
      tr("File Operations Guide") + "</a>");

  // All labels: uniform setup
  const QList<QLabel*> labels = {concepts_title, concepts_desc, concepts_link,
                                 textops_title,  textops_desc,  textops_link,
                                 fileops_title,  fileops_desc,  fileops_link};
  for (auto* l : labels) {
    l->setTextFormat(Qt::RichText);
    l->setTextInteractionFlags(Qt::TextBrowserInteraction);
    l->setOpenExternalLinks(true);
    l->setWordWrap(true);
    layout->addWidget(l);
  }

  layout->addStretch();  // push content upward
  setLayout(layout);

  next_page_ = Wizard::kPAGE_CONCLUSION;
}

auto ChoosePage::nextId() const -> int { return next_page_; }

void ChoosePage::slot_jump_page(const QString& page) {
  QMetaObject const qmo = Wizard::staticMetaObject;
  int const index = qmo.indexOfEnumerator("WizardPages");
  QMetaEnum const m = qmo.enumerator(index);

  next_page_ = m.keyToValue(page.toUtf8().data());
  wizard()->next();
}

ConclusionPage::ConclusionPage(QWidget* parent) : QWizardPage(parent) {
  setTitle(tr("You're all set!"));
  setSubTitle(tr("GpgFrontend is ready to use."));

  auto* bottom_label = new QLabel(
      tr("GpgFrontend is now fully set up and ready to go.") + "<br><br>" +
      tr("If you encounter any issues or have questions, feel free to ") +
      "<a href=\"https://github.com/saturneric/GpgFrontend/issues\">" +
      tr("submit an issue on GitHub") + "</a> " + tr("or ") +
      "<a href=\"https://gpgfrontend.bktus.com/overview/contact/\">" +
      tr("contact the developer") + "</a>." + "<br>" +
      tr("Any feedback is welcome and valuable!"));
  bottom_label->setTextFormat(Qt::RichText);
  bottom_label->setTextInteractionFlags(Qt::TextBrowserInteraction);
  bottom_label->setOpenExternalLinks(true);
  bottom_label->setWordWrap(true);

  // Option checkboxes
  open_help_check_box_ = new QCheckBox(tr("Open offline help now"));
  open_help_check_box_->setChecked(true);

  dont_show_wizard_checkbox_ =
      new QCheckBox(tr("Don't show this setup wizard again"));
  dont_show_wizard_checkbox_->setChecked(true);

  // I think update checking should be off by default for privacy reasons
  check_updates_checkbox_ = new QCheckBox(tr("Check for updates on startup"));
  check_updates_checkbox_->setChecked(false);

  // Register fields
  registerField("showWizard", dont_show_wizard_checkbox_);
  registerField("checkUpdate", check_updates_checkbox_);

  // Layout
  auto* layout = new QVBoxLayout;
  layout->addWidget(bottom_label);
  layout->addSpacing(15);
  // layout->addWidget(open_help_check_box_);
  layout->addWidget(check_updates_checkbox_);
  layout->addWidget(dont_show_wizard_checkbox_);
  layout->addStretch();
  setLayout(layout);
}

auto ConclusionPage::nextId() const -> int { return -1; }

}  // namespace GpgFrontend::UI
