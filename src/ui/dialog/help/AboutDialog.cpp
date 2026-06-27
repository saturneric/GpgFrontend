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
#include "core/utils/RustUtils.h"
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

// A QFrame that invokes a callback when clicked. It needs no signals/slots, so
// it deliberately omits Q_OBJECT and just overrides the mouse handler.
class ClickableFrame : public QFrame {
 public:
  explicit ClickableFrame(std::function<void()> on_click,
                          QWidget* parent = nullptr)
      : QFrame(parent), on_click_(std::move(on_click)) {}

 protected:
  void mouseReleaseEvent(QMouseEvent* event) override {
    if (event->button() == Qt::LeftButton && rect().contains(event->pos()) &&
        on_click_) {
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
  const auto url = QStringLiteral("https://github.com/saturneric/GpgFrontend");

  auto* frame = new ClickableFrame(
      [parent, url]() { OpenExternalUrlWithFallback(parent, url); }, parent);
  frame->setObjectName(QStringLiteral("AboutStarCard"));
  frame->setFrameShape(QFrame::StyledPanel);
  frame->setCursor(Qt::PointingHandCursor);
  frame->setStyleSheet(
      QStringLiteral("QFrame#AboutStarCard {"
                     "  border: 1px solid #e3b341;"
                     "  border-radius: 8px;"
                     "}"));

  // The title looks like a link but isn't interactive itself: the parent frame
  // handles the click so the whole card is the hit target.
  auto* title_label = new QLabel(
      QStringLiteral("<span style=\"color:#d29922;\">&#9733;</span> <b>%1</b>")
          .arg(QObject::tr("Star GpgFrontend on GitHub")),
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
             "Sodium: %7\n"
             "Git Branch: %8\n"
             "Git Commit: %9\n"
             "Built at: %10")
      .arg(GetProjectVersion(), GetProjectQtVersion(), GetProjectGpgMEVersion(),
           GetProjectAssuanVersion(), GetProjectLibarchiveVersion(),
           GetProjectOpenSSLVersion(), GetSodiumVersion(),
           GetProjectBuildGitBranchName(), GetProjectBuildGitCommitHash(),
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

  main_layout->addWidget(CreateStarCard(content));

  auto* developer_label = CreateBodyLabel(
      QStringLiteral(
          "%1<br/><br/>"
          "<a href=\"https://github.com/saturneric/GpgFrontend/issues\">%2</a>"
          "<br/>"
          "<a href=\"https://gpgfrontend.bktus.com/overview/contact/\">%3</a>"
          "<br/>"
          "<a href=\"mailto:eric@bktus.com\">eric@bktus.com</a>")
          .arg(tr("Developed and maintained by Saturneric."),
               tr("Report an issue on GitHub"),
               tr("About and contact information")),
      content);

  main_layout->addWidget(CreateCard(tr("Developer"), developer_label, content));

  // Resources card: static links to the project's main destinations. Each is
  // opened only when the user clicks it; nothing is fetched here.
  auto* resources_widget = new QWidget(content);
  auto* resources_form = CreateInfoForm(resources_widget);
  resources_widget->setLayout(resources_form);

  const auto link = [](const QString& url, const QString& text) -> QString {
    return QStringLiteral("<a href=\"%1\">%2</a>").arg(url, text);
  };

  resources_form->addRow(
      tr("Website:"),
      CreateBodyLabel(link(QStringLiteral("https://gpgfrontend.bktus.com"),
                           QStringLiteral("gpgfrontend.bktus.com")),
                      resources_widget));
  resources_form->addRow(
      tr("Documentation:"),
      CreateBodyLabel(
          link(QStringLiteral("https://gpgfrontend.bktus.com/overview/glance"),
               tr("User guides and overview")),
          resources_widget));
  resources_form->addRow(
      tr("Source code:"),
      CreateBodyLabel(
          link(QStringLiteral("https://github.com/saturneric/GpgFrontend"),
               QStringLiteral("github.com/saturneric/GpgFrontend")),
          resources_widget));
  resources_form->addRow(
      tr("Release notes:"),
      CreateBodyLabel(
          link(QStringLiteral(
                   "https://github.com/saturneric/GpgFrontend/releases"),
               tr("Changelog and downloads")),
          resources_widget));

  main_layout->addWidget(
      CreateCard(tr("Resources"), resources_widget, content));

  main_layout->addStretch();

  // Footer: license notice and copyright, kept small and muted.
  auto* license_label = CreateBodyLabel(
      QStringLiteral("<div align=\"center\" style=\"color:gray;\">%1</div>")
          .arg(tr("GpgFrontend is free software, licensed under "
                  "<a href=\"https://www.gnu.org/licenses/gpl-3.0.html\">"
                  "GPL-3.0-or-later</a>.")),
      content);
  license_label->setAlignment(Qt::AlignCenter);

  auto* copyright_label = new QLabel(
      QStringLiteral(
          "<div align=\"center\" style=\"color:gray; font-size:12px;\">"
          "&copy; 2021-2026 Saturneric &lt;eric@bktus.com&gt;</div>"),
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

  setObjectName(QStringLiteral("InfoTab"));
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

  auto* build_form_widget = new QWidget(build_widget);
  auto* build_form = CreateInfoForm(build_form_widget);
  build_form_widget->setLayout(build_form);

  build_form->addRow(tr("GpgFrontend:"),
                     CreateValueLabel(GetProjectVersion(), build_form_widget));
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
  build_form->addRow(tr("Sodium:"),
                     CreateValueLabel(GetSodiumVersion(), build_form_widget));
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
  outer_layout->addWidget(CreateScrollArea(content, this));
  setLayout(outer_layout);

  setObjectName(QStringLiteral("BuildInfoTab"));
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
  if (GetGSS().IsEngineSupported(OpenPGPEngine::kGNUPG)) {
    status_form->addRow(
        tr("GnuPG Offline Mode:"),
        CreateValueLabel(gnupg_offline_mode_str, status_widget));
    status_form->addRow(
        tr("Pinentry Program Path:"),
        CreateValueLabel(pinentry_program_path_str, status_widget));
  }

  main_layout->addWidget(
      CreateCard(tr("Application Status"), status_widget, content));

  const auto active_engines = GetGSS().AllSupportedEngines();

  if (!active_engines.isEmpty()) {
    auto* engines_widget = new QWidget(content);
    auto* engines_layout = new QVBoxLayout(engines_widget);
    engines_layout->setContentsMargins(0, 0, 0, 0);
    engines_layout->setSpacing(6);

    for (const auto& engine : active_engines) {
      auto* engine_label =
          CreateValueLabel(QStringLiteral("• %1").arg(engine), engines_widget);
      engines_layout->addWidget(engine_label);
    }

    main_layout->addWidget(
        CreateCard(tr("Supported OpenPGP Engines"), engines_widget, content));
  }

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
  auto* engine_widget = new QWidget(content);
  auto* engine_form = CreateInfoForm(engine_widget);
  engine_widget->setLayout(engine_form);

  engine_form->addRow(
      tr("Engine Version:"),
      CreateValueLabel(or_unknown(info.engine_version), engine_widget));
  engine_form->addRow(
      tr("Rust Compiler:"),
      CreateValueLabel(or_unknown(info.rustc_version), engine_widget));
  if (!info.target.isEmpty()) {
    engine_form->addRow(tr("Target:"),
                        CreateValueLabel(info.target, engine_widget));
  }
  if (!info.profile.isEmpty()) {
    engine_form->addRow(tr("Build Profile:"),
                        CreateValueLabel(info.profile, engine_widget));
  }

  main_layout->addWidget(CreateCard(tr("rPGP Engine"), engine_widget, content));

  // Dependency card: key crate versions.
  if (!info.dependencies.isEmpty()) {
    auto* deps_widget = new QWidget(content);
    auto* deps_form = CreateInfoForm(deps_widget);
    deps_widget->setLayout(deps_form);

    for (const auto& dep : info.dependencies) {
      deps_form->addRow(QStringLiteral("%1:").arg(dep.first),
                        CreateValueLabel(dep.second, deps_widget));
    }

    main_layout->addWidget(
        CreateCard(tr("Key Dependencies"), deps_widget, content));
  }

  // Assemble a plain-text summary for the copy button.
  QStringList lines;
  lines
      << QStringLiteral("Rust Engine: %1").arg(or_unknown(info.engine_version))
      << QStringLiteral("Rust Compiler: %1")
             .arg(or_unknown(info.rustc_version));
  if (!info.target.isEmpty()) {
    lines << QStringLiteral("Target: %1").arg(info.target);
  }
  if (!info.profile.isEmpty()) {
    lines << QStringLiteral("Build Profile: %1").arg(info.profile);
  }
  for (const auto& dep : info.dependencies) {
    lines << QStringLiteral("%1: %2").arg(dep.first, dep.second);
  }

  auto* copy_button = CreateCopyButton(tr("Copy Engine Information"),
                                       lines.join(QLatin1Char('\n')), content);
  main_layout->addWidget(copy_button, 0, Qt::AlignRight);

  main_layout->addStretch();

  auto* outer_layout = new QVBoxLayout(this);
  outer_layout->setContentsMargins(0, 0, 0, 0);
  outer_layout->addWidget(CreateScrollArea(content, this));
  setLayout(outer_layout);

  setObjectName(QStringLiteral("RpgpEngineTab"));
}

}  // namespace GpgFrontend::UI