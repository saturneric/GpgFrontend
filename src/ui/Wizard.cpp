/**
 * Copyright (C) 2021 Saturneric
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
 * Saturneric<eric@bktus.com> starting on May 12, 2021.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include "ui/Wizard.h"

#include "ui/settings/GlobalSettingStation.h"

namespace GpgFrontend::UI {

Wizard::Wizard(QWidget* parent) : QWizard(parent) {
  setPage(Page_Intro, new IntroPage(this));
  setPage(Page_Choose, new ChoosePage(this));
  setPage(Page_GenKey, new KeyGenPage(this));
  setPage(Page_Conclusion, new ConclusionPage(this));
#ifndef Q_WS_MAC
  setWizardStyle(ModernStyle);
#endif
  setWindowTitle(_("First Start Wizard"));

  // http://www.flickr.com/photos/laureenp/6141822934/
  setPixmap(QWizard::WatermarkPixmap, QPixmap(":/keys2.jpg"));
  setPixmap(QWizard::LogoPixmap, QPixmap(":/logo_small.png"));
  setPixmap(QWizard::BannerPixmap, QPixmap(":/banner.png"));

  auto& settings = GlobalSettingStation::GetInstance().GetUISettings();
  int next_page_id = -1;
  try {
    next_page_id = settings.lookup("wizard.next_page");
  } catch (...) {
    LOG(ERROR) << _("Setting Operation Error");
  }
  setStartId(next_page_id);

  connect(this, SIGNAL(accepted()), this, SLOT(slot_wizard_accepted()));
}

void Wizard::slot_wizard_accepted() {
  LOG(INFO) << _("Called");
  // Don't show is mapped to show -> negation
  try {
    auto& settings = GlobalSettingStation::GetInstance().GetUISettings();
    if (!settings.exists("wizard")) {
      settings.add("wizard", libconfig::Setting::TypeGroup);
    }
    auto& wizard = settings["wizard"];
    if (!wizard.exists("show_wizard")) {
      wizard.add("show_wizard", libconfig::Setting::TypeBoolean) = false;
    } else {
      wizard["show_wizard"] = false;
    }
    GlobalSettingStation::GetInstance().SyncSettings();
  } catch (...) {
    LOG(ERROR) << _("Setting Operation Error");
  }
  if (field("openHelp").toBool()) {
    emit SignalOpenHelp("docu.html#content");
  }
}

IntroPage::IntroPage(QWidget* parent) : QWizardPage(parent) {
  setTitle(_("Getting Started..."));
  setSubTitle(_("... with GpgFrontend"));

  auto* topLabel = new QLabel(
      QString(_("Welcome to use GpgFrontend for decrypting and signing text or "
                "file!")) +
      " <br><br><a href='https://gpgfrontend.pub'>GpgFrontend</a> " +
      _("is a Powerful, Easy-to-Use, Compact, Cross-Platform, and "
        "Installation-Free OpenPGP Crypto Tool.") +
      _("For brief information have a look at the") +
      " <a href='https://gpgfrontend.pub/index.html#/overview'>" +
      _("Overview") + "</a> (" +
      _("by clicking the link, the page will open in the web browser") +
      "). <br>");
  topLabel->setTextFormat(Qt::RichText);
  topLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
  topLabel->setOpenExternalLinks(true);
  topLabel->setWordWrap(true);

  // QComboBox for language selection
  auto* langLabel =
      new QLabel(_("If it supports the language currently being used in your "
                   "system, GpgFrontend will automatically set it."));
  langLabel->setWordWrap(true);

  // set layout and add widgets
  auto* layout = new QVBoxLayout;
  layout->addWidget(topLabel);
  layout->addStretch();
#ifdef MULTI_LANG_SUPPORT
  layout->addWidget(langLabel);
#endif

  setLayout(layout);
}

int IntroPage::nextId() const { return Wizard::Page_Choose; }

ChoosePage::ChoosePage(QWidget* parent) : QWizardPage(parent) {
  setTitle(_("Choose your action..."));
  setSubTitle(_("...by clicking on the appropriate link."));

  auto* keygenLabel = new QLabel(
      QString(_(
          "If you have never used GpgFrontend before and also don't own a gpg "
          "key yet you "
          "may possibly want to read how to")) +
      " <a href=\"https://gpgfrontend.pub/index.html#/manual/generate-key\">" +
      _("Generate Key") + "</a><hr>");
  keygenLabel->setTextFormat(Qt::RichText);
  keygenLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
  keygenLabel->setOpenExternalLinks(true);
  keygenLabel->setWordWrap(true);

  auto* encrDecyTextLabel = new QLabel(
      QString(_(
          "If you want to learn how to encrypt, decrypt, sign and verify text, "
          "you can read ")) +
      "<a "
      "href=\"https://gpgfrontend.pub/index.html#/manual/"
      "encrypt-decrypt-text\">" +
      _("Encrypt & Decrypt Text") + "</a> " + _("or") +
      " <a "
      "href=\"https://gpgfrontend.pub/index.html#/manual/sign-verify-text\">" +
      _("Sign & Verify Text") + "</a><hr>");

  encrDecyTextLabel->setTextFormat(Qt::RichText);
  encrDecyTextLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
  encrDecyTextLabel->setOpenExternalLinks(true);
  encrDecyTextLabel->setWordWrap(true);

  auto* signVerifyTextLabel = new QLabel(
      QString(_("If you want to operate file, you can read ")) +
      "<a "
      "href=\"https://gpgfrontend.pub/index.html#/manual/"
      "encrypt-decrypt-file\">" +
      _("Encrypt & Sign File") + "</a> " + _("or") +
      " <a "
      "href=\"https://gpgfrontend.pub/index.html#/manual/sign-verify-file\">" +
      _("Sign & Verify File") + "</a><hr>");
  signVerifyTextLabel->setTextFormat(Qt::RichText);
  signVerifyTextLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
  signVerifyTextLabel->setOpenExternalLinks(true);
  signVerifyTextLabel->setWordWrap(true);

  auto* layout = new QVBoxLayout();
  layout->addWidget(keygenLabel);
  layout->addWidget(encrDecyTextLabel);
  layout->addWidget(signVerifyTextLabel);
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
  setTitle(_("Create a keypair..."));
  setSubTitle(_("...for decrypting and signing messages"));
  auto* topLabel = new QLabel(
      _("You should create a new keypair."
        "The pair consists of a public and a private key.<br>"
        "Other users can use the public key to encrypt messages for you "
        "and verify messages signed by you."
        "You can use the private key to decrypt and sign messages.<br>"
        "For more information have a look at the offline tutorial (which then "
        "is shown in the main window):"));
  topLabel->setWordWrap(true);
  auto* linkLabel = new QLabel(
      "<a href="
      "docu_keygen.html#content"
      ">" +
      QString(_("Offline tutorial")) + "</a>");
  // linkLabel->setOpenExternalLinks(true);

  // connect(linkLabel, SIGNAL(linkActivated(QString)),
  // parentWidget()->parentWidget(), SLOT(openHelp(QString)));

  auto* createKeyButtonBox = new QWidget(this);
  auto* createKeyButtonBoxLayout = new QHBoxLayout(createKeyButtonBox);
  auto* createKeyButton = new QPushButton(_("Create New Key"));
  createKeyButtonBoxLayout->addWidget(createKeyButton);
  createKeyButtonBoxLayout->addStretch(1);
  auto* layout = new QVBoxLayout();
  layout->addWidget(topLabel);
  layout->addWidget(linkLabel);
  layout->addWidget(createKeyButtonBox);
  connect(createKeyButton, SIGNAL(clicked(bool)), this,
          SLOT(slot_generate_key_dialog()));

  setLayout(layout);
}

int KeyGenPage::nextId() const { return Wizard::Page_Conclusion; }

void KeyGenPage::slot_generate_key_dialog() {
  LOG(INFO) << "Try Opening KeyGenDialog";
  (new KeyGenDialog(this))->show();
  wizard()->next();
}

ConclusionPage::ConclusionPage(QWidget* parent) : QWizardPage(parent) {
  setTitle(_("Ready."));
  setSubTitle(_("Have fun with GpgFrontend!"));

  auto* bottomLabel = new QLabel(
      QString(_("You are ready to use GpgFrontend now.<br><br>")) +
      "<a "
      "href=\"https://saturneric.github.io/GpgFrontend/index.html#/"
      "overview\">" +
      _("The Online Document") + "</a>" +
      _(" will get you started with GpgFrontend. Anytime you encounter "
        "problems, please try to find help from the documentation") +
      "<br>");

  bottomLabel->setTextFormat(Qt::RichText);
  bottomLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
  bottomLabel->setOpenExternalLinks(true);
  bottomLabel->setWordWrap(true);

  open_help_check_box_ = new QCheckBox(_("Open offline help."));
  open_help_check_box_->setChecked(true);

  dont_show_wizard_checkbox_ = new QCheckBox(_("Dont show the wizard again."));
  dont_show_wizard_checkbox_->setChecked(true);

  registerField("showWizard", dont_show_wizard_checkbox_);
  // registerField("openHelp", openHelpCheckBox);

  auto* layout = new QVBoxLayout;
  layout->addWidget(bottomLabel);
  // layout->addWidget(openHelpCheckBox);
  layout->addWidget(dont_show_wizard_checkbox_);
  setLayout(layout);
  setVisible(true);
}

int ConclusionPage::nextId() const { return -1; }

}  // namespace GpgFrontend::UI
