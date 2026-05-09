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

#include <QCheckBox>
#include <QFrame>
#include <QLabel>
#include <QMetaEnum>
#include <QPixmap>
#include <QVBoxLayout>

#include "core/function/GlobalSettingStation.h"
#include "core/utils/CommonUtils.h"

namespace GpgFrontend::UI {
namespace {

auto CreateBodyLabel(const QString& text) -> QLabel* {
  auto* label = new QLabel(text);
  label->setWordWrap(true);
  label->setTextFormat(Qt::RichText);
  label->setTextInteractionFlags(Qt::TextBrowserInteraction);
  label->setOpenExternalLinks(true);
  return label;
}

auto CreateMutedLabel(const QString& text) -> QLabel* {
  auto* label = CreateBodyLabel(text);
  return label;
}

auto CreateLinkCard(const QString& title, const QString& description,
                    const QString& url) -> QFrame* {
  auto* frame = new QFrame;
  frame->setObjectName(QStringLiteral("WizardLinkCard"));
  frame->setFrameShape(QFrame::StyledPanel);
  frame->setCursor(Qt::PointingHandCursor);

  auto* title_label = CreateBodyLabel(
      QStringLiteral("<a href=\"%1\"><b>%2</b></a>").arg(url, title));

  auto* desc_label = CreateMutedLabel(description);

  auto* layout = new QVBoxLayout(frame);
  layout->setContentsMargins(14, 10, 14, 10);
  layout->setSpacing(4);
  layout->addWidget(title_label);
  layout->addWidget(desc_label);

  return frame;
}
}  // namespace

Wizard::Wizard(QWidget* parent) : QWizard(parent) {
  setPage(kPAGE_INTRO, new IntroPage(this));
  setPage(kPAGE_CHOOSE, new ChoosePage(this));
  setPage(kPAGE_CONCLUSION, new ConclusionPage(this));

#if !defined(Q_OS_MACOS)
  setWizardStyle(QWizard::ModernStyle);
#endif

  setWindowTitle(tr("Welcome to GpgFrontend"));
  setOption(QWizard::NoBackButtonOnStartPage);
  setOption(QWizard::HaveHelpButton, false);

  resize(680, 500);

  const auto logo =
      QPixmap(QStringLiteral(":/icons/gpgfrontend_logo.png"))
          .scaled(72, 72, Qt::KeepAspectRatio, Qt::SmoothTransformation);

  setPixmap(QWizard::LogoPixmap, logo);

  const int next_page_id =
      GetSettings()
          .value(QStringLiteral("wizard.next_page"), kPAGE_INTRO)
          .toInt();

  if (next_page_id >= kPAGE_INTRO && next_page_id <= kPAGE_CONCLUSION) {
    setStartId(next_page_id);
  } else {
    setStartId(kPAGE_INTRO);
  }

  connect(this, &Wizard::accepted, this, &Wizard::slot_wizard_accepted);
}

void Wizard::slot_wizard_accepted() {
  auto settings = GetSettings();

  settings.setValue(QStringLiteral("wizard/show_wizard"),
                    !field(QStringLiteral("hideWizard")).toBool());

  settings.setValue(QStringLiteral("network/prohibit_update_check"),
                    !field(QStringLiteral("checkUpdate")).toBool());
}

IntroPage::IntroPage(QWidget* parent) : QWizardPage(parent) {
  setTitle(tr("Welcome to GpgFrontend"));
  setSubTitle(
      tr("A simple and privacy-focused OpenPGP tool for text and files."));

  auto* intro_label = CreateBodyLabel(
      tr("<b>GpgFrontend</b> helps you encrypt, decrypt, sign, and verify "
         "messages and files with OpenPGP. This short wizard will point you to "
         "the most useful places to start."));

  auto* privacy_label = CreateMutedLabel(
      tr("You can change language, update checking, key database, and "
         "appearance settings later from the application settings."));

  auto* overview_card = CreateLinkCard(
      tr("Open the overview page"),
      tr("Get a quick tour of the main features and common workflows."),
      QStringLiteral("https://gpgfrontend.bktus.com/overview/glance"));

  auto* concepts_card = CreateLinkCard(
      tr("Fundamental concepts"),
      tr("Understand public keys, private keys, encryption, signing, and "
         "trust."),
      QStringLiteral(
          "https://gpgfrontend.bktus.com/guides/fundamental-concepts/"));

  auto* layout = new QVBoxLayout;
  layout->setContentsMargins(8, 8, 8, 8);
  layout->setSpacing(14);
  layout->addWidget(intro_label);
  layout->addWidget(privacy_label);
  layout->addSpacing(8);
  layout->addWidget(overview_card);
  layout->addWidget(concepts_card);
  layout->addStretch();

  setLayout(layout);
}

auto IntroPage::nextId() const -> int { return Wizard::kPAGE_CHOOSE; }

ChoosePage::ChoosePage(QWidget* parent) : QWizardPage(parent) {
  setTitle(tr("Choose a guide"));
  setSubTitle(tr("Pick a topic if you want to learn the basics first."));

  auto* layout = new QVBoxLayout;
  layout->setContentsMargins(8, 8, 8, 8);
  layout->setSpacing(10);

  layout->addWidget(CreateLinkCard(
      tr("Generate a new Key Pair"),
      tr("Learn how to create your own key pairs."),
      QStringLiteral("https://gpgfrontend.bktus.com/guides/generate-key/")));

  layout->addWidget(CreateLinkCard(
      tr("Text operations"),
      tr("Learn how to encrypt, decrypt, sign, and verify text messages."),
      QStringLiteral("https://gpgfrontend.bktus.com/guides/text-operations/")));

  layout->addWidget(CreateLinkCard(
      tr("File operations"),
      tr("Learn how to encrypt, decrypt, sign, and verify files securely."),
      QStringLiteral("https://gpgfrontend.bktus.com/guides/file-operations/")));

  layout->addWidget(CreateLinkCard(
      tr("View key pair information"),
      tr("Learn how to inspect key details, user IDs, fingerprints, and key "
         "capabilities."),
      QStringLiteral(
          "https://gpgfrontend.bktus.com/guides/view-keypair-info/")));

  auto* hint_label = CreateMutedLabel(
      tr("You can also skip these guides and start using GpgFrontend "
         "directly."));

  layout->addSpacing(2);
  layout->addWidget(hint_label);
  layout->addStretch();

  setLayout(layout);

  next_page_ = Wizard::kPAGE_CONCLUSION;
}

auto ChoosePage::nextId() const -> int { return next_page_; }

void ChoosePage::slot_jump_page(const QString& page) {
  const QMetaObject qmo = Wizard::staticMetaObject;
  const int index = qmo.indexOfEnumerator("WizardPages");

  if (index < 0) {
    next_page_ = Wizard::kPAGE_CONCLUSION;
    wizard()->next();
    return;
  }

  const QMetaEnum meta_enum = qmo.enumerator(index);
  const int page_id = meta_enum.keyToValue(page.toUtf8().constData());

  if (page_id >= Wizard::kPAGE_INTRO && page_id <= Wizard::kPAGE_CONCLUSION) {
    next_page_ = page_id;
  } else {
    next_page_ = Wizard::kPAGE_CONCLUSION;
  }

  wizard()->next();
}

ConclusionPage::ConclusionPage(QWidget* parent) : QWizardPage(parent) {
  setTitle(tr("Ready to use"));
  setSubTitle(
      tr("GpgFrontend is ready. You can adjust these options before "
         "finishing."));

  auto* layout = new QVBoxLayout;
  layout->setContentsMargins(8, 8, 8, 8);
  layout->setSpacing(10);

  layout->addWidget(CreateLinkCard(
      tr("Contact and feedback"),
      tr("Report issues, ask questions, or send feedback to help improve "
         "GpgFrontend."),
      QStringLiteral("https://gpgfrontend.bktus.com/overview/contact/")));

  layout->addWidget(CreateLinkCard(
      tr("Submit an issue on GitHub"),
      tr("Use GitHub issues if you want to report a bug or track a technical "
         "problem."),
      QStringLiteral("https://github.com/saturneric/GpgFrontend/issues")));

  check_updates_checkbox_ = new QCheckBox(tr("Check for updates on startup"));
  check_updates_checkbox_->setChecked(false);

  if (IsRunningInSandBox()) {
    check_updates_checkbox_->setHidden(true);
    check_updates_checkbox_->setChecked(false);
  }

  dont_show_wizard_checkbox_ =
      new QCheckBox(tr("Don't show this setup wizard again"));
  dont_show_wizard_checkbox_->setChecked(true);

  registerField(QStringLiteral("hideWizard"), dont_show_wizard_checkbox_);
  registerField(QStringLiteral("checkUpdate"), check_updates_checkbox_);

  layout->addSpacing(8);
  layout->addWidget(check_updates_checkbox_);
  layout->addWidget(dont_show_wizard_checkbox_);
  layout->addStretch();

  setLayout(layout);
}

auto ConclusionPage::nextId() const -> int { return -1; }

}  // namespace GpgFrontend::UI