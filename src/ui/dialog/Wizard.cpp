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
#include <QComboBox>
#include <QFrame>
#include <QLabel>
#include <QMetaEnum>
#include <QPixmap>
#include <QVBoxLayout>
#include <algorithm>

#include "core/function/GlobalSettingStation.h"
#include "core/utils/BuildInfoUtils.h"
#include "core/utils/CommonUtils.h"
#include "ui/GpgFrontendUIInit.h"
#include "ui/dialog/settings/SettingsDialog.h"

namespace GpgFrontend::UI {
namespace {

// Swaps the interface language of the running process. The wizard applies the
// picked language immediately so the user sees what they chose, instead of
// having to finish the wizard first and hope for the best.
void ApplyUILanguage(const QString& lang) {
  QLocale::setDefault(lang.trimmed().isEmpty() ? QLocale::system()
                                               : QLocale(lang));
  InitUITranslations();
  InitModulesTranslations();
}

auto CreateBodyLabel() -> QLabel* {
  auto* label = new QLabel;
  label->setWordWrap(true);
  label->setTextFormat(Qt::RichText);
  label->setTextInteractionFlags(Qt::TextBrowserInteraction);
  label->setOpenExternalLinks(true);
  return label;
}

auto CreateMutedLabel() -> QLabel* {
  auto* label = CreateBodyLabel();
  return label;
}

// The wordmark plus its tagline. Only the tagline is translated, so it is the
// single thing handed back to the page for retranslation.
struct BrandHeader {
  QWidget* widget{nullptr};
  QLabel* tagline_label{nullptr};
};

// A card built by CreateLinkCard/CreateStarCard. It carries its url and anchor
// markup so a page can re-text it after a language change without repeating
// the markup at every call site.
struct LinkCard {
  QFrame* frame{nullptr};
  QLabel* title_label{nullptr};
  QLabel* desc_label{nullptr};
  QString url;
  QString title_markup;

  void SetText(const QString& title, const QString& description) const {
    title_label->setText(title_markup.arg(url, title));
    desc_label->setText(description);
  }
};

// A branded hero header for the very first wizard page: the GpgFrontend
// wordmark, its version, and a short tagline. The logo itself already lives in
// the wizard header band on every page, so the hero stays a text wordmark to
// avoid showing the logo twice while still making the first thing a new user
// sees unmistakably GpgFrontend rather than a wall of text.
auto CreateBrandHeader() -> BrandHeader {
  auto* widget = new QWidget;

  auto* name_label = new QLabel(
      QStringLiteral(
          "<span style=\"font-size:24px; font-weight:600;\">GpgFrontend</span>"
          "&nbsp;&nbsp;"
          "<span style=\"font-size:13px; color:gray;\">%1</span>")
          .arg(GetProjectVersion()));
  name_label->setTextFormat(Qt::RichText);
  name_label->setTextInteractionFlags(Qt::TextSelectableByMouse);

  auto* tagline_label = CreateMutedLabel();

  auto* layout = new QVBoxLayout(widget);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(2);
  layout->addWidget(name_label);
  layout->addWidget(tagline_label);

  return {widget, tagline_label};
}

auto CreateLinkCard(const QString& url) -> LinkCard {
  auto* frame = new QFrame;
  frame->setObjectName(QStringLiteral("WizardLinkCard"));
  frame->setFrameShape(QFrame::StyledPanel);
  frame->setCursor(Qt::PointingHandCursor);

  auto* title_label = CreateBodyLabel();
  auto* desc_label = CreateMutedLabel();

  auto* layout = new QVBoxLayout(frame);
  layout->setContentsMargins(14, 10, 14, 10);
  layout->setSpacing(4);
  layout->addWidget(title_label);
  layout->addWidget(desc_label);

  return {frame, title_label, desc_label, url,
          QStringLiteral("<a href=\"%1\"><b>%2</b></a>")};
}

// A highlighted variant of the link card used to invite the user to star the
// project on GitHub. It mirrors the plain link card but carries a GitHub-star
// amber accent and a star glyph so it stands out as a call to action instead of
// reading as just another documentation link.
auto CreateStarCard(const QString& url) -> LinkCard {
  auto* frame = new QFrame;
  frame->setObjectName(QStringLiteral("WizardStarCard"));
  frame->setFrameShape(QFrame::StyledPanel);
  frame->setCursor(Qt::PointingHandCursor);
  frame->setStyleSheet(
      QStringLiteral("QFrame#WizardStarCard {"
                     "  border: 1px solid #e3b341;"
                     "  border-radius: 8px;"
                     "}"));

  auto* title_label = CreateBodyLabel();
  auto* desc_label = CreateMutedLabel();

  auto* layout = new QVBoxLayout(frame);
  layout->setContentsMargins(14, 10, 14, 10);
  layout->setSpacing(4);
  layout->addWidget(title_label);
  layout->addWidget(desc_label);

  return {
      frame, title_label, desc_label, url,
      QStringLiteral("<a href=\"%1\" style=\"text-decoration:none;\">&#9733; "
                     "<b>%2</b></a>")};
}
}  // namespace

WizardPage::WizardPage(QWidget* parent) : QWizardPage(parent) {}

void WizardPage::add_retranslator(std::function<void()> retranslator) {
  retranslator();
  retranslators_.append(std::move(retranslator));
}

void WizardPage::retranslate_ui() {
  for (const auto& retranslator : retranslators_) retranslator();
}

void WizardPage::changeEvent(QEvent* event) {
  if (event->type() == QEvent::LanguageChange) retranslate_ui();
  QWizardPage::changeEvent(event);
}

Wizard::Wizard(QWidget* parent) : QWizard(parent) {
  initial_lang_ = GetSettings().value(QStringLiteral("basic/lang")).toString();

  intro_page_ = new IntroPage(this);
  setPage(kPAGE_INTRO, intro_page_);
  setPage(kPAGE_CHOOSE, new ChoosePage(this));
  setPage(kPAGE_CONCLUSION, new ConclusionPage(this));

#if !defined(Q_OS_MACOS)
  setWizardStyle(QWizard::ModernStyle);
#endif

  setOption(QWizard::NoBackButtonOnStartPage);
  setOption(QWizard::HaveHelpButton, false);

  // Tall enough that the language row on the first page does not squeeze the
  // cards below it into clipped single lines.
  resize(680, 580);

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

  setWindowTitle(tr("Welcome to GpgFrontend"));

  connect(this, &Wizard::accepted, this, &Wizard::slot_wizard_accepted);
  connect(this, &Wizard::rejected, this, &Wizard::slot_wizard_rejected);
}

void Wizard::retranslate_ui() {
  setWindowTitle(tr("Welcome to GpgFrontend"));

  // QWizard takes the standard button texts from Qt's own translation once,
  // when the style is applied, and never listens for QEvent::LanguageChange.
  // Bouncing the style is the only public way to make it read them again, so
  // Back/Next/Finish follow the language the user just picked.
  const auto style = wizardStyle();
  setWizardStyle(style == QWizard::ClassicStyle ? QWizard::ModernStyle
                                                : QWizard::ClassicStyle);
  setWizardStyle(style);
}

void Wizard::changeEvent(QEvent* event) {
  if (event->type() == QEvent::LanguageChange) retranslate_ui();
  QWizard::changeEvent(event);
}

void Wizard::slot_wizard_accepted() {
  auto settings = GetSettings();

  settings.setValue(QStringLiteral("wizard/show_wizard"),
                    !field(QStringLiteral("hideWizard")).toBool());

  settings.setValue(QStringLiteral("network/prohibit_update_check"),
                    !field(QStringLiteral("checkUpdate")).toBool());

  const auto lang = intro_page_->SelectedLanguage();
  settings.setValue(QStringLiteral("basic/lang"), lang);

  // Everything built before the switch, the main window above all, keeps the
  // translations it was built with, so it only follows after a reload.
  if (lang != initial_lang_) emit SignalRestartNeeded(kRestartCode);
}

void Wizard::slot_wizard_rejected() {
  // Nothing was written to the settings, so the language preview the user
  // walked away from must not outlive the wizard either.
  if (intro_page_->SelectedLanguage() == initial_lang_) return;

  ApplyUILanguage(initial_lang_);
}

IntroPage::IntroPage(QWidget* parent) : WizardPage(parent) {
  const auto brand = CreateBrandHeader();

  lang_label_ = new QLabel;
  lang_select_box_ = new QComboBox;
  lang_select_box_->setSizeAdjustPolicy(QComboBox::AdjustToContents);
  populate_languages();

  auto* lang_layout = new QHBoxLayout;
  lang_layout->setContentsMargins(0, 0, 0, 0);
  lang_layout->setSpacing(8);
  lang_layout->addWidget(lang_label_);
  lang_layout->addWidget(lang_select_box_);
  lang_layout->addStretch();

  auto* intro_label = CreateBodyLabel();
  auto* privacy_label = CreateMutedLabel();

  const auto star_card = CreateStarCard(
      QStringLiteral("https://github.com/saturneric/GpgFrontend"));

  const auto overview_card = CreateLinkCard(
      QStringLiteral("https://gpgfrontend.bktus.com/overview/glance"));

  const auto concepts_card = CreateLinkCard(QStringLiteral(
      "https://gpgfrontend.bktus.com/guides/fundamental-concepts/"));

  auto* layout = new QVBoxLayout;
  layout->setContentsMargins(8, 8, 8, 8);
  layout->setSpacing(14);
  layout->addWidget(brand.widget);
  layout->addLayout(lang_layout);
  layout->addWidget(intro_label);
  layout->addSpacing(4);
  layout->addWidget(star_card.frame);
  layout->addWidget(overview_card.frame);
  layout->addWidget(concepts_card.frame);
  layout->addWidget(privacy_label);
  layout->addStretch();

  setLayout(layout);

  add_retranslator([this, brand, intro_label, privacy_label, star_card,
                    overview_card, concepts_card]() {
    setTitle(tr("Welcome to GpgFrontend"));
    setSubTitle(tr("Let's get you started in just a moment."));

    brand.tagline_label->setText(
        tr("A simple, privacy-focused OpenPGP tool for text, files, and "
           "keys."));

    lang_label_->setText(tr("Language:"));
    lang_select_box_->setItemText(0, tr("System Default"));

    intro_label->setText(
        tr("<b>GpgFrontend</b> helps you encrypt, decrypt, sign, and verify "
           "messages and files with OpenPGP. This short wizard will point you "
           "to the most useful places to start."));

    star_card.SetText(
        tr("Star GpgFrontend on GitHub"),
        tr("GpgFrontend is free and open source. A star helps more people "
           "discover it and keeps the project moving forward."));

    overview_card.SetText(
        tr("Open the overview page"),
        tr("Get a quick tour of the main features and common workflows."));

    concepts_card.SetText(
        tr("Fundamental concepts"),
        tr("Understand public keys, private keys, encryption, signing, and "
           "trust."));

    privacy_label->setText(
        tr("You can change update checking, key database, and appearance "
           "settings later from the application settings."));
  });

  // Connected last: filling the box above already moved the current index, and
  // that must not count as the user picking a language.
  connect(lang_select_box_, &QComboBox::currentIndexChanged, this,
          &IntroPage::slot_language_changed);
}

void IntroPage::populate_languages() {
  const auto languages = SettingsDialog::ListLanguages();

  // ListLanguages() hands back a QHash, whose iteration order is not stable.
  // Sort by the native language name and pin the system default to the top, so
  // the box reads the same way every time it is opened.
  QContainer<QPair<QString, QString>> entries;
  for (auto it = languages.constBegin(); it != languages.constEnd(); ++it) {
    if (it.key().isEmpty()) continue;
    entries.append({it.key(), it.value()});
  }

  std::sort(
      entries.begin(), entries.end(),
      [](const QPair<QString, QString>& a, const QPair<QString, QString>& b) {
        return a.second.localeAwareCompare(b.second) < 0;
      });

  lang_select_box_->addItem(tr("System Default"), QString());
  for (const auto& entry : entries) {
    lang_select_box_->addItem(entry.second, entry.first);
  }

  // Matched on the stored locale key rather than on the display text, so a
  // stale or unknown basic/lang falls back to the system default instead of
  // leaving the box with nothing selected at all.
  const auto lang =
      GetSettings().value(QStringLiteral("basic/lang")).toString();
  const auto index = lang_select_box_->findData(lang);
  lang_select_box_->setCurrentIndex(index >= 0 ? index : 0);

  applied_lang_ = lang_select_box_->currentData().toString();
}

void IntroPage::slot_language_changed(int index) {
  const auto lang = lang_select_box_->itemData(index).toString();
  if (lang == applied_lang_) return;

  applied_lang_ = lang;
  ApplyUILanguage(lang);
}

auto IntroPage::SelectedLanguage() const -> QString {
  return lang_select_box_->currentData().toString();
}

auto IntroPage::nextId() const -> int { return Wizard::kPAGE_CHOOSE; }

ChoosePage::ChoosePage(QWidget* parent) : WizardPage(parent) {
  auto* layout = new QVBoxLayout;
  layout->setContentsMargins(8, 8, 8, 8);
  layout->setSpacing(10);

  const auto generate_card = CreateLinkCard(
      QStringLiteral("https://gpgfrontend.bktus.com/guides/generate-key/"));

  const auto text_card = CreateLinkCard(
      QStringLiteral("https://gpgfrontend.bktus.com/guides/text-operations/"));

  const auto file_card = CreateLinkCard(
      QStringLiteral("https://gpgfrontend.bktus.com/guides/file-operations/"));

  const auto keypair_card = CreateLinkCard(QStringLiteral(
      "https://gpgfrontend.bktus.com/guides/view-keypair-info/"));

  auto* hint_label = CreateMutedLabel();

  layout->addWidget(generate_card.frame);
  layout->addWidget(text_card.frame);
  layout->addWidget(file_card.frame);
  layout->addWidget(keypair_card.frame);
  layout->addSpacing(2);
  layout->addWidget(hint_label);
  layout->addStretch();

  setLayout(layout);

  add_retranslator([this, generate_card, text_card, file_card, keypair_card,
                    hint_label]() {
    setTitle(tr("Choose a guide"));
    setSubTitle(tr("Pick a topic if you want to learn the basics first."));

    generate_card.SetText(tr("Generate a new Key Pair"),
                          tr("Learn how to create your own key pairs."));

    text_card.SetText(
        tr("Text operations"),
        tr("Learn how to encrypt, decrypt, sign, and verify text messages."));

    file_card.SetText(
        tr("File operations"),
        tr("Learn how to encrypt, decrypt, sign, and verify files securely."));

    keypair_card.SetText(
        tr("View key pair information"),
        tr("Learn how to inspect key details, user IDs, fingerprints, and key "
           "capabilities."));

    hint_label->setText(
        tr("You can also skip these guides and start using GpgFrontend "
           "directly."));
  });

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

ConclusionPage::ConclusionPage(QWidget* parent) : WizardPage(parent) {
  auto* layout = new QVBoxLayout;
  layout->setContentsMargins(8, 8, 8, 8);
  layout->setSpacing(10);

  const auto contact_card = CreateLinkCard(
      QStringLiteral("https://gpgfrontend.bktus.com/overview/contact/"));

  const auto issue_card = CreateLinkCard(
      QStringLiteral("https://github.com/saturneric/GpgFrontend/issues"));

  check_updates_checkbox_ = new QCheckBox;
  check_updates_checkbox_->setChecked(false);

  if (IsRunningInSandBox()) {
    check_updates_checkbox_->setHidden(true);
    check_updates_checkbox_->setChecked(false);
  }

  dont_show_wizard_checkbox_ = new QCheckBox;
  dont_show_wizard_checkbox_->setChecked(true);

  registerField(QStringLiteral("hideWizard"), dont_show_wizard_checkbox_);
  registerField(QStringLiteral("checkUpdate"), check_updates_checkbox_);

  layout->addWidget(contact_card.frame);
  layout->addWidget(issue_card.frame);
  layout->addSpacing(8);
  layout->addWidget(check_updates_checkbox_);
  layout->addWidget(dont_show_wizard_checkbox_);
  layout->addStretch();

  setLayout(layout);

  add_retranslator([this, contact_card, issue_card]() {
    setTitle(tr("Ready to use"));
    setSubTitle(
        tr("GpgFrontend is ready. You can adjust these options before "
           "finishing."));

    contact_card.SetText(
        tr("Contact and feedback"),
        tr("Report issues, ask questions, or send feedback to help improve "
           "GpgFrontend."));

    issue_card.SetText(
        tr("Submit an issue on GitHub"),
        tr("Use GitHub issues if you want to report a bug or track a technical "
           "problem."));

    check_updates_checkbox_->setText(tr("Check for updates on startup"));
    dont_show_wizard_checkbox_->setText(
        tr("Don't show this setup wizard again"));
  });
}

auto ConclusionPage::nextId() const -> int { return -1; }

}  // namespace GpgFrontend::UI
