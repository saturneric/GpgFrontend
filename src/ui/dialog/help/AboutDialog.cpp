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
#include "core/function/SystemSecretStore.h"
#include "core/module/ModuleManager.h"
#include "core/profile/ProfileLoader.h"
#include "core/profile/ProfileMarker.h"
#include "core/profile/ProfileSecureKeyManager.h"
#include "core/profile/ProfileSession.h"
#include "core/utils/BuildInfoUtils.h"
#include "core/utils/GpgUtils.h"
#include "core/utils/RustUtils.h"
#include "ui/UIModuleManager.h"
#include "ui/UserInterfaceUtils.h"
#include "ui/dialog/help/AboutStatusInfo.h"
#include "ui/function/ProfileController.h"

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

// A word-wrapped label that is actually given room for the lines it wraps to.
//
// Declaring heightForWidth on the size policy is not enough on its own: a
// QFormLayout hands the field its sizeHint() and does not consult
// heightForWidth, so a value that wrapped to three lines was allotted one and
// the rest was drawn off the bottom of the row. Rows here read "Opened from a"
// and "The package carries none of" and looked complete. The two overrides
// below make the hints answer for the width the label actually has, and the
// resize handler asks the layout to run again once that width is known.
//
// MinimumExpanding, so the field fills the form's value column instead of
// settling for QLabel's own guess at a pleasing wrap width, which came out
// around a hundred pixels and turned every sentence into a narrow ribbon.
//
// No Q_OBJECT: it declares no signals or slots, like ClickableFrame below.
class WrappingLabel : public QLabel {
 public:
  explicit WrappingLabel(const QString& text, QWidget* parent = nullptr)
      : QLabel(text, parent) {
    setWordWrap(true);

    QSizePolicy policy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
    policy.setHeightForWidth(true);
    setSizePolicy(policy);
  }

  [[nodiscard]] auto sizeHint() const -> QSize override {
    return {minimumWidth(), heightForWidth(width())};
  }

  [[nodiscard]] auto minimumSizeHint() const -> QSize override {
    return {0, heightForWidth(width())};
  }

 protected:
  void resizeEvent(QResizeEvent* event) override {
    QLabel::resizeEvent(event);
    updateGeometry();
  }
};

auto CreateValueLabel(const QString& text, QWidget* parent = nullptr)
    -> QLabel* {
  auto* label = new WrappingLabel(text, parent);
  label->setTextInteractionFlags(Qt::TextSelectableByMouse);
  return label;
}

// A path is the one value here that has no natural length limit, and it stays
// out of a wrapping label even now that WrappingLabel gives one the lines it
// asks for: a path is a single unbreakable token, so it either wraps at some
// arbitrary character or runs past the edge, and a path that looks complete
// and is not is worse than one that is visibly cut. A read-only field is
// always exactly one line tall, scrolls to show the rest, and copies whole.
auto CreatePathValue(const QString& text, QWidget* parent = nullptr)
    -> QLineEdit* {
  auto* edit = new QLineEdit(text, parent);
  edit->setReadOnly(true);
  edit->setFrame(false);
  edit->setToolTip(text);
  edit->setCursorPosition(0);
  edit->setStyleSheet(
      QStringLiteral("QLineEdit { background: transparent; border: none; "
                     "padding: 0; }"));
  return edit;
}

// The sentence that sits under a value. Quieter and a shade smaller, so it
// reads as an explanation of the line above rather than as a second value.
//
// The colour goes through the widget's own palette rather than through an
// inline rich-text span: that keeps the platform font, which a stylesheet on a
// QLabel would otherwise take over.
auto CreateDetailLabel(const QString& text, QWidget* parent = nullptr)
    -> QLabel* {
  auto* label = CreateValueLabel(text, parent);

  auto font = label->font();
  font.setPointSizeF(font.pointSizeF() * 0.92);
  label->setFont(font);

  auto palette = label->palette();
  palette.setColor(QPalette::WindowText, MutedTextColor(palette));
  palette.setColor(QPalette::Text, MutedTextColor(palette));
  label->setPalette(palette);

  return label;
}

// Put an already built value widget over its detail sentence. Separate from
// CreateValueWithDetail() because a path value is a read-only field rather
// than a label, and must stay one, but still deserves its explanation.
auto StackValueAndDetail(QWidget* value_widget, const QString& detail,
                         QWidget* parent = nullptr) -> QWidget* {
  if (detail.isEmpty()) return value_widget;

  auto* holder = new QWidget(parent);
  auto* layout = new QVBoxLayout(holder);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(2);

  value_widget->setParent(holder);
  layout->addWidget(value_widget);
  layout->addWidget(CreateDetailLabel(detail, holder));

  // Same reasoning as WrappingLabel above: the stack has to fill the value
  // column, or both of its lines wrap inside a ribbon of QLabel's choosing.
  QSizePolicy policy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
  policy.setHeightForWidth(true);
  holder->setSizePolicy(policy);

  return holder;
}

// A value with an explanation under it.
//
// A form row carries one string, so a value that needed a sentence to be
// honest either grew into a paragraph in the value column ("Memory - not
// written to this disk in the normal course of things") or got a "... Detail:"
// row of its own two rows down, detached from the thing it explained. Both
// were happening on this tab. This gives the row a second line instead: the
// value stays a value, and the sentence sits under it.
//
// @param value the reading itself
// @param detail the sentence under it; empty yields the bare value label
// @param degraded tint the value, for a state that is a fallback not an intent
auto CreateValueWithDetail(const QString& value, const QString& detail,
                           bool degraded = false, QWidget* parent = nullptr)
    -> QWidget* {
  auto* value_label = CreateValueLabel(value, parent);

  if (degraded) {
    auto palette = value_label->palette();
    const auto warning = WarningColor(palette);
    palette.setColor(QPalette::WindowText, warning);
    palette.setColor(QPalette::Text, warning);
    value_label->setPalette(palette);
  }

  if (detail.isEmpty()) return value_label;

  return StackValueAndDetail(value_label, detail, parent);
}

auto CreateCard(const QString& title, QWidget* content,
                QWidget* parent = nullptr) -> QFrame* {
  auto* frame = new QFrame(parent);
  frame->setObjectName(QStringLiteral("AboutCard"));
  frame->setFrameShape(QFrame::StyledPanel);

  // A hairline drawn from the palette rather than the platform's StyledPanel
  // groove, which is a bevel on some styles and nothing at all on others. This
  // is the same shape the star card above already uses, so every card in the
  // dialog now reads as one set.
  frame->setStyleSheet(QStringLiteral("QFrame#AboutCard {"
                                      "  border: 1px solid %1;"
                                      "  border-radius: 8px;"
                                      "}")
                           .arg(BorderColor(frame->palette()).name()));

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
  // Enough room that a row carrying a value over a detail sentence is still
  // told apart from the row under it.
  form->setVerticalSpacing(10);
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

  // Build a single-line link label for a resource row. Word wrap is left off so
  // the form's field column sizes to the full link text: a word-wrapped
  // rich-text label reports a collapsed width hint here and gets clipped (e.g.
  // "User guides and…", "github.com/…").
  const auto link_label = [resources_widget](const QString& url,
                                             const QString& text) -> QLabel* {
    auto* label =
        CreateBodyLabel(QStringLiteral("<a href=\"%1\">%2</a>").arg(url, text),
                        resources_widget);
    label->setWordWrap(false);
    return label;
  };

  resources_form->addRow(
      tr("Website:"),
      link_label(QStringLiteral("https://gpgfrontend.bktus.com"),
                 QStringLiteral("gpgfrontend.bktus.com")));
  resources_form->addRow(
      tr("Documentation:"),
      link_label(
          QStringLiteral("https://gpgfrontend.bktus.com/overview/glance"),
          tr("User guides and overview")));
  resources_form->addRow(
      tr("Source code:"),
      link_label(QStringLiteral("https://github.com/saturneric/GpgFrontend"),
                 QStringLiteral("github.com/saturneric/GpgFrontend")));
  resources_form->addRow(
      tr("Release notes:"),
      link_label(
          QStringLiteral("https://github.com/saturneric/GpgFrontend/releases"),
          tr("Changelog and downloads")));

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
  const bool gnupg_offline_mode = qApp->property("GFGnuPGOfflineMode").toBool();
  const QString pinentry_program_path =
      qApp->property("GFPinentryProgramPath").toString();

  auto* content = new QWidget(this);
  auto* main_layout = new QVBoxLayout(content);
  main_layout->setContentsMargins(18, 18, 18, 18);
  main_layout->setSpacing(14);

  // Built up alongside the rows themselves so the two cannot drift: this tab
  // tells the user its values "may help when reporting issues", and until now
  // the only way to act on that was to select them out of a scroll area by
  // hand, one row at a time.
  QStringList summary;

  const auto begin_section = [&summary](const QString& title) {
    if (!summary.isEmpty()) summary << QString();
    summary << QStringLiteral("[%1]").arg(title);
  };

  // Add a row and record it, so every reading on screen is in the clipboard
  // text too. The detail sentence is indented under its value there, the same
  // way it sits under it here.
  const auto add_row = [&summary](QFormLayout* form, const QString& label,
                                  const QString& value,
                                  const QString& detail = {},
                                  bool degraded = false) {
    auto* holder = form->parentWidget();
    form->addRow(label, CreateValueWithDetail(value, detail, degraded, holder));

    summary << QStringLiteral("%1 %2").arg(label, value);
    if (!detail.isEmpty()) summary << QStringLiteral("    %1").arg(detail);
  };

  // Paths go through CreatePathValue() rather than a label, so they need their
  // own adder; see that function for why a wrapped label cannot be trusted
  // with a path.
  const auto add_path_row = [&summary](QFormLayout* form, const QString& label,
                                       const QString& path,
                                       const QString& detail = {}) {
    auto* holder = form->parentWidget();
    form->addRow(label, StackValueAndDetail(CreatePathValue(path, holder),
                                            detail, holder));

    summary << QStringLiteral("%1 %2").arg(label, path);
    if (!detail.isEmpty()) summary << QStringLiteral("    %1").arg(detail);
  };

  begin_section(tr("Application Status"));

  auto* status_widget = new QWidget(content);
  auto* status_form = CreateInfoForm(status_widget);
  status_widget->setLayout(status_form);

  add_row(status_form, tr("Secure Level:"),
          SecureLevelDisplayName(secure_level));
  add_row(
      status_form, tr("Application Key Protection:"),
      AppKeyProtectionDisplayName(ProfileLoader::AppKeyProtectionFromApp()));

  // Reported whether or not it is in use: when "System keychain" is greyed out
  // in the settings, this is the only place that says why. Read from the
  // registry rather than probed -- opening About must never raise a keyring
  // unlock prompt.
  //
  // The loader's reason rides along as this row's detail rather than as a
  // "Credential Store Detail:" row of its own two rows down. It stays
  // untranslated and selectable, so it can be pasted into a bug report exactly
  // as the loader produced it.
  auto* secret_store = GetSystemSecretStore();
  add_row(status_form, tr("System Credential Store:"),
          secret_store != nullptr ? secret_store->Name() : tr("Unavailable"),
          SystemSecretStoreReason(), secret_store == nullptr);

  // Only ever set when the home directory could not host the agent sockets, so
  // the row says that outright and carries the measured length and the
  // platform limit underneath. Untranslated and selectable, for the same
  // reason as the credential store reason above.
  if (const auto gnupg_home_detail = GnuPGHomePathUnusableReason();
      !gnupg_home_detail.isEmpty()) {
    add_row(status_form, tr("GnuPG Home:"), tr("Unusable"), gnupg_home_detail,
            true);
  }

  add_row(status_form, tr("Running Mode:"),
          portable_mode ? tr("Portable Mode") : tr("Installed Mode"));

  if (GetGSS().IsEngineSupported(OpenPGPEngine::kGNUPG)) {
    add_row(status_form, tr("GnuPG Offline Mode:"),
            gnupg_offline_mode ? tr("Active") : tr("Disabled"));
    add_row(status_form, tr("Pinentry Program Path:"),
            pinentry_program_path.isEmpty() ? tr("Default Pinentry Program")
                                            : pinentry_program_path);
  }

  main_layout->addWidget(
      CreateCard(tr("Application Status"), status_widget, content));

  // Which profile this window is using, and where it keeps things. With two
  // windows open on two profiles, "which keys am I looking at" is the first
  // question of any bug report, and the paths below answer it exactly.
  const auto& session = ProfileSession::Instance();
  const auto& profile = session.Profile();

  begin_section(tr("Profile"));

  auto* profile_widget = new QWidget(content);
  auto* profile_form = CreateInfoForm(profile_widget);
  profile_widget->setLayout(profile_form);

  add_row(profile_form, tr("Profile:"), CurrentProfileDisplayName());
  add_row(profile_form, tr("Profile Type:"),
          ProfileKindDisplayName(profile.Kind()));
  add_path_row(profile_form, tr("Profile Folder:"),
               QDir::toNativeSeparators(profile.Root()));

  // Where the *keyring* comes from, which is not the credential store the card
  // above already reports. "System keyring" was ambiguous between the two, and
  // sat two rows under "System Credential Store: libsecret".
  const auto keys =
      DescribeKeySource(profile.Policy().self_contained, profile.Kind());
  add_row(profile_form, tr("Keys:"), keys.value, keys.detail, keys.degraded);

  // Only for a session opened from a package, because only there was there a
  // choice to make. DescribeSessionStorage() carries the wording and decides
  // which outcome counts as a fallback.
  if (profile.Kind() == ProfileKind::kPACKAGED) {
    const auto& storage = session.Accessor();
    const auto session_storage = DescribeSessionStorage(
        storage.IsVolatile(), storage.IsEncryptedAtRest());

    add_row(profile_form, tr("Session Storage:"), session_storage.value,
            session_storage.detail, session_storage.degraded);
  }

  const auto workspace = ProfileSession::Instance().WorkspacePath();
  if (workspace.isEmpty()) {
    add_row(profile_form, tr("Workspace:"), tr("None"));
  } else {
    add_path_row(profile_form, tr("Workspace:"),
                 QDir::toNativeSeparators(workspace));
  }

  const auto& marker = session.Marker();
  if (marker.schema_version > 0) {
    add_row(profile_form, tr("Profile Layout Version:"),
            QString::number(marker.schema_version));
  }

  // Present only on a profile that came from a package, and worth showing
  // there: it is the identity that says which document this copy came from.
  if (!marker.package_id.isEmpty()) {
    add_path_row(profile_form, tr("Imported From Package:"), marker.package_id);
  }

  // Mirrored onto the application rather than asked of the profile: where this
  // machine keeps its persisted profiles is a property of the installation, not
  // of the one profile in front of us.
  if (const auto profiles_root = qApp->property("GFProfilesRoot").toString();
      !profiles_root.isEmpty()) {
    add_path_row(profile_form, tr("Profiles Folder:"),
                 QDir::toNativeSeparators(profiles_root));
  }

  main_layout->addWidget(CreateCard(tr("Profile"), profile_widget, content));

  const auto active_engines = GetGSS().AllSupportedEngines();

  if (!active_engines.isEmpty()) {
    begin_section(tr("Supported OpenPGP Engines"));

    auto* engines_widget = new QWidget(content);
    auto* engines_layout = new QVBoxLayout(engines_widget);
    engines_layout->setContentsMargins(0, 0, 0, 0);
    engines_layout->setSpacing(6);

    for (const auto& engine : active_engines) {
      auto* engine_label =
          CreateValueLabel(QStringLiteral("• %1").arg(engine), engines_widget);
      engines_layout->addWidget(engine_label);
      summary << QStringLiteral("- %1").arg(engine);
    }

    main_layout->addWidget(
        CreateCard(tr("Supported OpenPGP Engines"), engines_widget, content));
  }

  main_layout->addStretch();

  auto* copy_button = CreateCopyButton(
      tr("Copy Status Information"), summary.join(QLatin1Char('\n')), content);
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

  setObjectName(QStringLiteral("StatusTab"));
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