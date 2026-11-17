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

#include <QDesktopServices>
#include <functional>

#include "core/function/GlobalSettingStation.h"
#include "core/module/ModuleManager.h"
#include "core/utils/BuildInfoUtils.h"
#include "core/utils/GpgUtils.h"
#include "core/utils/RustUtils.h"
#include "shell/present/ProfileStatus.h"
#include "ui/UIModuleManager.h"
#include "ui/function/UIStyle.h"
#include "ui/widgets/MetaListPanel.h"

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

// A QFrame that invokes a callback when clicked. It needs no signals/slots, so
// it deliberately omits Q_OBJECT and just overrides the mouse handler.
class ClickableFrame : public QFrame {
 public:
  explicit ClickableFrame(std::function<void()> on_click,
                          QWidget* parent = nullptr)
      : QFrame(parent), on_click_(std::move(on_click)) {}

 protected:
  void mouseReleaseEvent(QMouseEvent* event) override {
    if (event->button() == Qt::LeftButton &&
        rect().contains(event->position().toPoint()) && on_click_) {
      on_click_();
    }
    QFrame::mouseReleaseEvent(event);
  }

 private:
  std::function<void()> on_click_;
};

// Open an external URL, degrading gracefully when the system has no browser or
// URL handler (plausible on a hardened, offline machine): the URL is copied to
// the clipboard and the user is told, so the link is never a dead end. Nothing
// is opened unless the user explicitly clicks.
void OpenExternalUrlWithFallback(QWidget* parent, const QString& url) {
  if (QDesktopServices::openUrl(QUrl(url))) return;

  QApplication::clipboard()->setText(url);
  QMessageBox::information(
      parent, QObject::tr("Open Link"),
      QObject::tr("Could not open a web browser on this system.\n\n"
                  "The link has been copied to your clipboard:\n%1")
          .arg(url));
}

// A highlighted call-to-action card inviting the user to star the project on
// GitHub. It carries a GitHub-star amber accent and a star glyph so it stands
// out from the plain information cards, mirroring the same card in the setup
// wizard. The whole card is clickable; the URL is opened only on that click,
// with a clipboard fallback, and nothing is fetched.
auto CreateStarCard(QWidget* parent = nullptr) -> QFrame* {
  const auto url = u"https://github.com/saturneric/GpgFrontend"_s;

  auto* frame = new ClickableFrame(
      [parent, url]() { OpenExternalUrlWithFallback(parent, url); }, parent);
  frame->setObjectName(u"AboutStarCard"_s);
  frame->setFrameShape(QFrame::StyledPanel);
  frame->setCursor(Qt::PointingHandCursor);
  frame->setStyleSheet(
      u"QFrame#AboutStarCard {"
      "  border: 1px solid #e3b341;"
      "  border-radius: 8px;"
      "}"_s);

  // The title looks like a link but isn't interactive itself: the parent frame
  // handles the click so the whole card is the hit target.
  auto* title_label = new QLabel(
      u"<span style=\"color:#d29922;\">&#9733;</span> <b>%1</b>"_s.arg(
          QObject::tr("Star GpgFrontend on GitHub")),
      frame);
  title_label->setTextFormat(Qt::RichText);
  title_label->setWordWrap(true);
  title_label->setTextInteractionFlags(Qt::NoTextInteraction);

  auto* desc_label = new QLabel(
      QObject::tr("GpgFrontend is free and open source. A star helps more "
                  "people discover it and keeps the project moving forward."),
      frame);
  desc_label->setWordWrap(true);
  desc_label->setTextInteractionFlags(Qt::NoTextInteraction);

  auto* layout = new QVBoxLayout(frame);
  layout->setContentsMargins(14, 10, 14, 10);
  layout->setSpacing(4);
  layout->addWidget(title_label);
  layout->addWidget(desc_label);

  return frame;
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
  auto* build_info_tab = new BuildInfoTab(tab_widget);
  auto* translators_tab = new TranslatorsTab(tab_widget);
  auto* status_tab = new StatusTab(tab_widget);

  tab_widget->setDocumentMode(true);

  tab_widget->addTab(info_tab, tr("About"));
  tab_widget->addTab(build_info_tab, tr("Build Information"));
  tab_widget->addTab(translators_tab, tr("Translators"));
  tab_widget->addTab(status_tab, tr("Status"));

  if (GetGSS().IsEngineSupported(OpenPGPEngine::kRPGP)) {
    tab_widget->addTab(new RpgpEngineTab(tab_widget), tr("Rust Engine"));
  }

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
      QPixmap(u":/icons/gpgfrontend_logo.png"_s)
          .scaled(112, 112, Qt::KeepAspectRatio, Qt::SmoothTransformation);

  auto* pixmap_label = new QLabel(content);
  pixmap_label->setPixmap(pixmap);
  pixmap_label->setAlignment(Qt::AlignCenter);

  auto* title_label = new QLabel(
      u"<div align=\"center\">"
      "<span style=\"font-size:22px; font-weight:600;\">%1</span>"
      "<br/>"
      "<span style=\"font-size:14px;\">%2</span>"
      "</div>"_s.arg(qApp->applicationDisplayName(), GetProjectVersion()),
      content);
  title_label->setTextFormat(Qt::RichText);
  title_label->setAlignment(Qt::AlignCenter);
  title_label->setTextInteractionFlags(Qt::TextSelectableByMouse);

  auto* tagline_label = CreateBodyLabel(
      u"<div align=\"center\">%1</div>"_s.arg(
          tr("A user-friendly OpenPGP tool for encryption, signing, and "
             "key management.")),
      content);
  tagline_label->setAlignment(Qt::AlignCenter);

  main_layout->addWidget(pixmap_label);
  main_layout->addWidget(title_label);
  main_layout->addWidget(tagline_label);

  main_layout->addWidget(CreateStarCard(content));

  auto* developer_label = CreateBodyLabel(
      u"%1<br/><br/>"
      "<a href=\"https://github.com/saturneric/GpgFrontend/issues\">%2</a>"
      "<br/>"
      "<a href=\"https://gpgfrontend.bktus.com/overview/contact/\">%3</a>"
      "<br/>"
      "<a href=\"mailto:eric@bktus.com\">eric@bktus.com</a>"_s.arg(
          tr("Developed and maintained by Saturneric."),
          tr("Report an issue on GitHub"), tr("About and contact information")),
      content);

  main_layout->addWidget(CreateCard(tr("Developer"), developer_label, content));

  // Resources card: static links to the project's main destinations. Each is
  // opened only when the user clicks it; nothing is fetched here.
  auto* resources = new MetaListPanel(content);
  resources->SetRows({
      {.caption = tr("Website:"),
       .value = u"gpgfrontend.bktus.com"_s,
       .link = u"https://gpgfrontend.bktus.com"_s},
      {.caption = tr("Documentation:"),
       .value = tr("User guides and overview"),
       .link = u"https://gpgfrontend.bktus.com/overview/glance"_s},
      {.caption = tr("Source code:"),
       .value = u"github.com/saturneric/GpgFrontend"_s,
       .link = u"https://github.com/saturneric/GpgFrontend"_s},
      {.caption = tr("Release notes:"),
       .value = tr("Changelog and downloads"),
       .link = u"https://github.com/saturneric/GpgFrontend/releases"_s},
  });

  main_layout->addWidget(CreateCard(tr("Resources"), resources, content));

  main_layout->addStretch();

  // Footer: license notice and copyright, kept small and muted.
  auto* license_label = CreateBodyLabel(
      u"<div align=\"center\" style=\"color:gray;\">%1</div>"_s.arg(
          tr("GpgFrontend is free software, licensed under "
             "<a href=\"https://www.gnu.org/licenses/gpl-3.0.html\">"
             "GPL-3.0-or-later</a>.")),
      content);
  license_label->setAlignment(Qt::AlignCenter);

  auto* copyright_label = new QLabel(
      u"<div align=\"center\" style=\"color:gray; font-size:12px;\">"
      "&copy; 2021-2026 Saturneric &lt;eric@bktus.com&gt;</div>"_s,
      content);
  copyright_label->setTextFormat(Qt::RichText);
  copyright_label->setAlignment(Qt::AlignCenter);
  copyright_label->setTextInteractionFlags(Qt::TextSelectableByMouse);

  main_layout->addWidget(license_label);
  main_layout->addWidget(copyright_label);

  // The About tab is intentionally not wrapped in a scroll area: its natural
  // size hint flows up through the tab widget so the dialog adapts to fit this
  // tab's content. The taller tabs (Build Information, Status, Rust Engine)
  // keep their own scroll areas instead.
  auto* outer_layout = new QVBoxLayout(this);
  outer_layout->setContentsMargins(0, 0, 0, 0);
  outer_layout->addWidget(content);
  setLayout(outer_layout);

  setObjectName(u"InfoTab"_s);
}

BuildInfoTab::BuildInfoTab(QWidget* parent) : QWidget(parent) {
  auto* content = new QWidget(this);
  auto* main_layout = new QVBoxLayout(content);
  main_layout->setContentsMargins(18, 18, 18, 18);
  main_layout->setSpacing(14);

  auto* build_widget = new QWidget(content);
  auto* build_layout = new QVBoxLayout(build_widget);
  build_layout->setContentsMargins(0, 0, 0, 0);
  build_layout->setSpacing(10);

  auto* build_rows = new MetaListPanel(build_widget);
  // The core's list, which the phone's About page shows too.
  QList<MetaListRow> rows;
  for (const auto& fact : BuildInfoFacts()) {
    rows.append({.caption = fact.Caption(), .value = fact.value});
  }
  build_rows->SetRows(rows);

  auto* copy_button = CreateCopyButton(tr("Copy Build Information"),
                                       BuildInfoText(), build_widget);

  build_layout->addWidget(build_rows);
  build_layout->addWidget(copy_button, 0, Qt::AlignRight);

  main_layout->addWidget(
      CreateCard(tr("Build Information"), build_widget, content));

  main_layout->addStretch();

  auto* outer_layout = new QVBoxLayout(this);
  outer_layout->setContentsMargins(0, 0, 0, 0);
  outer_layout->addWidget(CreateScrollArea(content, this));
  setLayout(outer_layout);

  setObjectName(u"BuildInfoTab"_s);
}

TranslatorsTab::TranslatorsTab(QWidget* parent) : QWidget(parent) {
  auto* main_layout = new QVBoxLayout(this);
  main_layout->setContentsMargins(18, 18, 18, 18);
  main_layout->setSpacing(12);

  auto* title_label = CreateBodyLabel(
      u"<b>%1</b>"_s.arg(tr("Thanks to all translators")), this);

  auto* browser = new QTextBrowser(this);
  browser->setOpenExternalLinks(true);
  browser->setReadOnly(true);

  QFile translators_file(u":/TRANSLATORS"_s);
  if (translators_file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    browser->setPlainText(QString::fromUtf8(translators_file.readAll()));
  } else {
    browser->setPlainText(tr("Translator information is not available."));
  }

  auto* notice_label = CreateBodyLabel(
      tr("If you want to help improve "
         "localization, please read the <a "
         "href='https://gpgfrontend.bktus.com/appendix/translate-interface/"
         "'>translation guide</a>.!"),
      this);

  main_layout->addWidget(title_label);
  main_layout->addWidget(browser, 1);
  main_layout->addWidget(notice_label);

  setLayout(main_layout);
}

namespace {

auto StatusRowsToMetaList(const QList<StatusRow>& rows) -> QList<MetaListRow> {
  QList<MetaListRow> out;
  out.reserve(rows.size());
  for (const auto& row : rows) {
    out.append({.caption = row.caption,
                .value = row.value,
                .detail = row.detail,
                .emphasis = row.emphasis,
                .dimmed = row.dimmed,
                .degraded = row.degraded,
                .path = row.path});
  }
  return out;
}

}  // namespace

StatusTab::StatusTab(QWidget* parent) : QWidget(parent) {
  auto* content = new QWidget(this);
  auto* main_layout = new QVBoxLayout(content);
  main_layout->setContentsMargins(18, 18, 18, 18);
  main_layout->setSpacing(10);

  // In order of what somebody opening it wants: anything wrong, first and by
  // itself; then the readings that answer "what am I running and where are my
  // keys"; then everything else in one Details card. The rows and their
  // wording are the shell's, so the phone's Status page says exactly the same.
  const auto report = BuildProfileStatus(GatherProfileStatusFacts());
  const auto sections = ProfileStatusSections(report);

  const auto add_card = [&](const QString& title,
                            const QList<MetaListRow>& rows) {
    if (rows.isEmpty()) return;
    auto* panel = new MetaListPanel(content);
    panel->SetRows(rows);
    main_layout->addWidget(CreateCard(title, panel, content));
  };

  add_card(tr("Needs Attention"), StatusRowsToMetaList(report.attention));
  add_card(tr("At a Glance"), StatusRowsToMetaList(report.glance));

  // One card with headings inside it, not four cards of two rows each: a card
  // costs a border, a title and twenty-six pixels of padding, and four of them
  // around eight readings is mostly padding.
  QList<MetaListRow> details;
  for (const auto& section : report.details) {
    if (section.rows.isEmpty()) continue;
    details.append({.kind = MetaRowKind::kSection, .caption = section.title});
    details.append(StatusRowsToMetaList(section.rows));
  }
  add_card(tr("Details"), details);

  main_layout->addStretch();

  auto* copy_button = CreateCopyButton(tr("Copy Status Information"),
                                       ProfileStatusText(sections), content);
  main_layout->addWidget(copy_button, 0, Qt::AlignRight);

  // A footnote, not another paragraph of body text: it explains the tab rather
  // than reporting anything, so it reads at the detail size and colour every
  // sub-line on this tab already uses.
  auto* tip_label = CreateDetailLabel(
      tr("These values reflect the current startup environment and may help "
         "when reporting issues."),
      content);
  main_layout->addWidget(tip_label);

  auto* outer_layout = new QVBoxLayout(this);
  outer_layout->setContentsMargins(0, 0, 0, 0);
  outer_layout->addWidget(CreateScrollArea(content, this));
  setLayout(outer_layout);

  setObjectName(u"StatusTab"_s);
}

RpgpEngineTab::RpgpEngineTab(QWidget* parent) : QWidget(parent) {
  const auto info = RustEngineBuildInfo();

  const auto or_unknown = [](const QString& value) -> QString {
    return value.isEmpty() ? tr("Unknown") : value;
  };

  auto* content = new QWidget(this);
  auto* main_layout = new QVBoxLayout(content);
  main_layout->setContentsMargins(18, 18, 18, 18);
  main_layout->setSpacing(14);

  auto* intro_label = CreateBodyLabel(
      tr("GpgFrontend supports multiple OpenPGP backends. Alongside GnuPG, it "
         "can use a Rust-based engine (rPGP), giving you the freedom to choose "
         "the backend that best fits your needs. The details below describe "
         "the "
         "rPGP engine compiled into this build."),
      content);
  main_layout->addWidget(intro_label);

  // Engine card: version, compiler, target, profile.
  QList<MetaListRow> engine_rows;
  engine_rows.append({.caption = tr("Engine Version:"),
                      .value = or_unknown(info.engine_version)});
  engine_rows.append({.caption = tr("Rust Compiler:"),
                      .value = or_unknown(info.rustc_version)});
  if (!info.target.isEmpty()) {
    engine_rows.append({.caption = tr("Target:"), .value = info.target});
  }
  if (!info.profile.isEmpty()) {
    engine_rows.append(
        {.caption = tr("Build Profile:"), .value = info.profile});
  }

  auto* engine_panel = new MetaListPanel(content);
  engine_panel->SetRows(engine_rows);
  main_layout->addWidget(CreateCard(tr("rPGP Engine"), engine_panel, content));

  // Dependency card: key crate versions.
  if (!info.dependencies.isEmpty()) {
    auto* deps = new MetaListPanel(content);
    deps->SetRows(ToMetaListRows(info.dependencies));

    main_layout->addWidget(CreateCard(tr("Key Dependencies"), deps, content));
  }

  // Assemble a plain-text summary for the copy button.
  QStringList lines;
  lines << u"Rust Engine: %1"_s.arg(or_unknown(info.engine_version))
        << u"Rust Compiler: %1"_s.arg(or_unknown(info.rustc_version));
  if (!info.target.isEmpty()) {
    lines << u"Target: %1"_s.arg(info.target);
  }
  if (!info.profile.isEmpty()) {
    lines << u"Build Profile: %1"_s.arg(info.profile);
  }
  for (const auto& dep : info.dependencies) {
    lines << u"%1: %2"_s.arg(dep.first, dep.second);
  }

  auto* copy_button = CreateCopyButton(tr("Copy Engine Information"),
                                       lines.join(QLatin1Char('\n')), content);
  main_layout->addWidget(copy_button, 0, Qt::AlignRight);

  main_layout->addStretch();

  auto* outer_layout = new QVBoxLayout(this);
  outer_layout->setContentsMargins(0, 0, 0, 0);
  outer_layout->addWidget(CreateScrollArea(content, this));
  setLayout(outer_layout);

  setObjectName(u"RpgpEngineTab"_s);
}

}  // namespace GpgFrontend::UI