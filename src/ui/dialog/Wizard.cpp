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

#include "Wizard.h"

#include "core/GpgModel.h"
#include "core/function/GlobalSettingStation.h"

namespace GpgFrontend::UI {

Wizard::Wizard(QWidget* parent) : QWizard(parent) {
  setPage(Page_Intro, new IntroPage(this));
  setPage(Page_Choose, new ChoosePage(this));
  setPage(Page_GenKey, new KeyGenPage(this));
  setPage(Page_Conclusion, new ConclusionPage(this));
#ifndef Q_WS_MAC
  setWizardStyle(ModernStyle);
#endif
  setWindowTitle(tr("First Start Wizard"));

  // http://www.flickr.com/photos/laureenp/6141822934/
  setPixmap(QWizard::WatermarkPixmap, QPixmap(":/icons/keys2.jpg"));
  setPixmap(QWizard::LogoPixmap, QPixmap(":/icons/logo_small.png"));
  setPixmap(QWizard::BannerPixmap, QPixmap(":/icons/banner.png"));

  int next_page_id = GlobalSettingStation::GetInstance()
                         .GetSettings()
                         .value("wizard.next_page", -1)
                         .toInt();
  setStartId(next_page_id);

  connect(this, &Wizard::accepted, this, &Wizard::slot_wizard_accepted);
}

void Wizard::slot_wizard_accepted() {
  // Don't show is mapped to show -> negation
  try {
    auto settings = GlobalSettingStation::GetInstance().GetSettings();
    settings.setValue("wizard/show_wizard", false);
  } catch (...) {
    GF_UI_LOG_ERROR("setting operation error");
  }
  if (field("openHelp").toBool()) {
    emit SignalOpenHelp("docu.html#content");
  }
}

IntroPage::IntroPage(QWidget* parent) : QWizardPage(parent) {
  setTitle(tr("Getting Started..."));
  setSubTitle(tr("... with GpgFrontend"));

  auto* topLabel = new QLabel(
      QString(
          tr("Welcome to GpgFrontend for decrypting and signing text or "
             "files!")) +
      " <br><br><a href='https://gpgfrontend.bktus.com'>GpgFrontend</a> " +
      tr("is a Powerful, Easy-to-Use, Compact, Cross-Platform, and "
         "Installation-Free OpenPGP Crypto Tool. ") +
      tr("To get started, be sure to check out the") +
      " <a href='https://gpgfrontend.bktus.com/overview/glance'>" +
      tr("Overview") + "</a> (" +
      tr("by clicking the link, the page will open in your web browser") +
      "). <br>");
  topLabel->setTextFormat(Qt::RichText);
  topLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
  topLabel->setOpenExternalLinks(true);
  topLabel->setWordWrap(true);

  // QComboBox for language selection
  auto* lang_label =
      new QLabel(tr("If it supports the language currently being used in your "
                    "system, GpgFrontend will automatically set it."));
  lang_label->setWordWrap(true);

  // set layout and add widgets
  auto* layout = new QVBoxLayout;
  layout->addWidget(topLabel);
  layout->addStretch();
  layout->addWidget(lang_label);

  setLayout(layout);
}

int IntroPage::nextId() const { return Wizard::Page_Choose; }

ChoosePage::ChoosePage(QWidget* parent) : QWizardPage(parent) {
  setTitle(tr("Choose your action..."));
  setSubTitle(tr("...by clicking on the appropriate link."));

  auto* keygen_label = new QLabel(
      tr("If you have never used GpgFrontend before and also don't own a gpg "
         "key yet you may possibly want to read how to") +
      " <a href=\"https://gpgfrontend.bktus.com/guides/generate-key\">" +
      tr("Generate Key") + "</a><hr>");
  keygen_label->setTextFormat(Qt::RichText);
  keygen_label->setTextInteractionFlags(Qt::TextBrowserInteraction);
  keygen_label->setOpenExternalLinks(true);
  keygen_label->setWordWrap(true);

 auto* encr_decy_text_label = new QLabel(
      tr("If you want to learn how to encrypt, decrypt, sign and verify text, "
         "you can read ") +
      "<a href=\"https://gpgfrontend.bktus.com/guides/encrypt-decrypt-text\">" +
      tr("Encrypt & Decrypt Text") + "</a> " + tr("or") +
      " <a href=\"https://gpgfrontend.bktus.com/guides/sign-verify-text\">" +
      tr("Sign & Verify Text") + "</a><hr>");

  encr_decy_text_label->setTextFormat(Qt::RichText);
  encr_decy_text_label->setTextInteractionFlags(Qt::TextBrowserInteraction);
  encr_decy_text_label->setOpenExternalLinks(true);
  encr_decy_text_label->setWordWrap(true);

  auto* sign_verify_text_label =
      new QLabel(tr("If you want to operate file, you can read ") +
                 "<a href=\"https://gpgfrontend.bktus.com/guides/encrypt-decrypt-file\">" +
                 tr("Encrypt & Sign File") + "</a> " + tr("or") +
                 " <a href=\"https://gpgfrontend.bktus.com/guides/sign-verify-file\">" +
                 tr("Sign & Verify File") + "</a><hr>");
  sign_verify_text_label->setTextFormat(Qt::RichText);
  sign_verify_text_label->setTextInteractionFlags(Qt::TextBrowserInteraction);
  sign_verify_text_label->setOpenExternalLinks(true);
  sign_verify_text_label->setWordWrap(true);

  auto* layout = new QVBoxLayout();
  layout->addWidget(keygen_label);
  layout->addWidget(encr_decy_text_label);
  layout->addWidget(sign_verify_text_label);
  setLayout(layout);
  next_page_ = Wizard::Page_Conclusion;
}

int ChoosePage::nextId() const { return next_page_; }

void ChoosePage::slot_jump_page(const QString& page) {
  QMetaObject qmo = Wizard::staticMetaObject;
  int index = qmo.indexOfEnumerator("WizardPages");
  QMetaEnum m = qmo.enumerator(index);

  next_page_ = m.keyToValue(page.toUtf8().data());
  wizard()->next();
}

KeyGenPage::KeyGenPage(QWidget* parent) : QWizardPage(parent) {
  setTitle(tr("Create a keypair..."));
  setSubTitle(tr("...for decrypting and signing messages"));
  auto* top_label = new QLabel(
      tr("You should create a new keypair."
         "The pair consists of a public and a private key.<br>"
         "Other users can use the public key to encrypt messages for you "
         "and verify messages signed by you."
         "You can use the private key to decrypt and sign messages.<br>"
         "For more information have a look at the offline tutorial (which then "
         "is shown in the main window):"));
  top_label->setWordWrap(true);
  auto* link_label = new QLabel(
      "<a href="
      "docu_keygen.html#content"
      ">" +
      tr("Offline tutorial") + "</a>");

  auto* create_key_button_box = new QWidget(this);
  auto* create_key_button_box_layout = new QHBoxLayout(create_key_button_box);
  auto* create_key_button = new QPushButton(tr("Create New Key"));
  create_key_button_box_layout->addWidget(create_key_button);
  create_key_button_box_layout->addStretch(1);
  auto* layout = new QVBoxLayout();
  layout->addWidget(top_label);
  layout->addWidget(link_label);
  layout->addWidget(create_key_button_box);
  connect(create_key_button, &QPushButton::clicked, this,
          &KeyGenPage::slot_generate_key_dialog);

  setLayout(layout);
}

int KeyGenPage::nextId() const { return Wizard::Page_Conclusion; }

void KeyGenPage::slot_generate_key_dialog() {
  (new KeyGenDialog(this))->show();
  wizard()->next();
}

ConclusionPage::ConclusionPage(QWidget* parent) : QWizardPage(parent) {
  setTitle(tr("Ready."));
  setSubTitle(tr("Have fun with GpgFrontend!"));

  auto* bottom_label = new QLabel(
      tr("You are ready to use GpgFrontend now.<br><br>") +
      "<a href=\"https://gpgfrontend.bktus.com/guides/understand-interface\">" +
      tr("The Online Document") + "</a>" +
      tr(" will get you started with GpgFrontend. Anytime you encounter "
         "problems, please try to find help from the documentation") +
      "<br>");

  bottom_label->setTextFormat(Qt::RichText);
  bottom_label->setTextInteractionFlags(Qt::TextBrowserInteraction);
  bottom_label->setOpenExternalLinks(true);
  bottom_label->setWordWrap(true);

  open_help_check_box_ = new QCheckBox(tr("Open offline help."));
  open_help_check_box_->setChecked(true);

  dont_show_wizard_checkbox_ = new QCheckBox(tr("Don't show the wizard again."));
  dont_show_wizard_checkbox_->setChecked(true);

  registerField("showWizard", dont_show_wizard_checkbox_);
  // registerField("openHelp", openHelpCheckBox);

  auto* layout = new QVBoxLayout;
  layout->addWidget(bottom_label);
  // layout->addWidget(openHelpCheckBox);
  layout->addWidget(dont_show_wizard_checkbox_);
  setLayout(layout);
  setVisible(true);
}

int ConclusionPage::nextId() const { return -1; }

}  // namespace GpgFrontend::UI
