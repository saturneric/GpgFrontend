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
#include "core/function/gpg/GpgContext.h"
#include "core/module/ModuleManager.h"
#include "core/profile/ProfileLoader.h"
#include "core/profile/ProfileMarker.h"
#include "core/profile/ProfileSecureKeyManager.h"
#include "core/profile/ProfileSession.h"
#include "core/utils/BuildInfoUtils.h"
#include "core/utils/GpgUtils.h"
#include "core/utils/RustUtils.h"
#include "ui/UIModuleManager.h"
#include "ui/dialog/help/AboutStatusInfo.h"
#include "ui/function/ProfileController.h"
#include "ui/function/SecurityDisplayNames.h"
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
  auto* resources = new MetaListPanel(content);
  resources->SetRows({
      {.caption = tr("Website:"),
       .value = QStringLiteral("gpgfrontend.bktus.com"),
       .link = QStringLiteral("https://gpgfrontend.bktus.com")},
      {.caption = tr("Documentation:"),
       .value = tr("User guides and overview"),
       .link = QStringLiteral("https://gpgfrontend.bktus.com/overview/glance")},
      {.caption = tr("Source code:"),
       .value = QStringLiteral("github.com/saturneric/GpgFrontend"),
       .link = QStringLiteral("https://github.com/saturneric/GpgFrontend")},
      {.caption = tr("Release notes:"),
       .value = tr("Changelog and downloads"),
       .link = QStringLiteral(
           "https://github.com/saturneric/GpgFrontend/releases")},
  });

  main_layout->addWidget(CreateCard(tr("Resources"), resources, content));

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

  auto* build_rows = new MetaListPanel(build_widget);
  build_rows->SetRows({
      {.caption = tr("GpgFrontend:"), .value = GetProjectVersion()},
      {.caption = tr("Qt:"), .value = GetProjectQtVersion()},
      {.caption = tr("GPGME:"), .value = GetProjectGpgMEVersion()},
      {.caption = tr("Assuan:"), .value = GetProjectAssuanVersion()},
      {.caption = tr("Libarchive:"), .value = GetProjectLibarchiveVersion()},
      {.caption = tr("OpenSSL:"), .value = GetProjectOpenSSLVersion()},
      {.caption = tr("Sodium:"), .value = GetSodiumVersion()},
      {.caption = tr("Git Branch:"), .value = GetProjectBuildGitBranchName()},
      {.caption = tr("Git Commit:"), .value = GetProjectBuildGitCommitHash()},
      {.caption = tr("Built at:"),
       .value = QLocale().toString(GetProjectBuildTimestamp())},
  });

  auto* copy_button = CreateCopyButton(tr("Copy Build Information"),
                                       CreateBuildInfoText(), build_widget);

  build_layout->addWidget(build_rows);
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
  main_layout->setSpacing(10);

  // This tab used to print everything it knew, in the order it happened to
  // learn it: how the key is protected, whether the install is portable, which
  // pinentry GnuPG uses, three paths and a layout version, all at one weight.
  // Every line was true and the page answered no question.
  //
  // It is now three things in order of what somebody opening it actually wants.
  // Anything wrong, first and by itself. Then the four readings that answer
  // "what am I running and where are my keys". Everything else is one click
  // away -- still here, still copied by the button at the bottom, no longer in
  // the way of the four rows that matter.
  QVector<MetaListRow> attention;  // anything not as it should be
  QVector<MetaListRow> glance;     // what the page is for
  QVector<QPair<QString, QVector<MetaListRow>>> details;  // true, but not now

  const auto& session = ProfileSession::Instance();
  const auto& profile = session.Profile();

  // Read from the registry rather than probed -- opening About must never raise
  // a keyring unlock prompt. Reported whether or not it is in use: when "System
  // keychain" is greyed out in the settings, this is the only place that says
  // why, and the loader's reason rides along untranslated and selectable so it
  // can be pasted into a bug report exactly as it was produced.
  auto* secret_store = GetSystemSecretStore();

  // Nothing here reports the key databases. There is a settings page whose
  // whole subject they are, and a second, shorter account of them on a tab the
  // user reaches from a different menu is how the two come to disagree.
  const auto protection = DescribeAppKeyProtection(
      ProfileLoader::AppKeyProtectionFromApp(), profile.AllowsSystemKeychain());

  // One row or two, decided by whether the profile has a name of its own: a
  // root profile is named after its kind, and a type row under it printed that
  // same word a second time.
  glance.append(BuildProfileIdentityRows(
      profile.Kind(), CurrentProfileDisplayName(), profile.IsTransient()));
  glance.append({.caption = tr("Application Key Protection:"),
                 .value = protection.value,
                 .detail = protection.detail});
  glance.append({.caption = tr("Secure Level:"),
                 .value = SecureLevelDisplayName(secure_level)});

  if (secret_store == nullptr) {
    attention.append({.caption = tr("System Credential Store:"),
                      .value = tr("Unavailable"),
                      .detail = SystemSecretStoreReason(),
                      .degraded = true});
  }

  // Only ever set when the home directory could not host the agent sockets, so
  // the row says that outright and carries the measured length and the platform
  // limit underneath.
  if (const auto gnupg_home_detail = GpgContext::FirstUnusableHomeReason();
      !gnupg_home_detail.isEmpty()) {
    attention.append({.caption = tr("GnuPG Home:"),
                      .value = tr("Unusable"),
                      .detail = gnupg_home_detail,
                      .degraded = true});
  }

  QVector<MetaListRow> application;
  application.append(
      {.caption = tr("Running Mode:"),
       .value = portable_mode ? tr("Portable Mode") : tr("Installed Mode")});
  if (secret_store != nullptr) {
    application.append({.caption = tr("System Credential Store:"),
                        .value = secret_store->Name(),
                        .detail = SystemSecretStoreReason()});
  }
  details.append({tr("Application"), application});

  QVector<MetaListRow> profile_rows;

  // The one value that identifies this profile in a bug report, which is what
  // the footnote at the bottom of this tab invites. A uuid is an unbreakable
  // token, so it gets the same treatment as a path.
  profile_rows.append(
      {.caption = tr("Profile ID:"), .value = profile.Id(), .path = true});

  // Only for a session opened from a package, because only there was there a
  // choice to make. DescribeSessionStorage() carries the wording and decides
  // which outcome counts as a fallback.
  if (profile.Kind() == ProfileKind::kPACKAGED) {
    const auto& storage = session.Accessor();
    const auto session_storage = DescribeSessionStorage(
        storage.IsVolatile(), storage.IsEncryptedAtRest());

    auto& into = session_storage.degraded ? attention : profile_rows;
    into.append({.caption = tr("Session Storage:"),
                 .value = session_storage.value,
                 .detail = session_storage.detail,
                 .degraded = session_storage.degraded});

    // Stated separately from the row above, and deliberately narrow. The
    // storage line describes where the *profile* went; this describes one thing
    // inside it. Folding them together would suggest the keyring is held in
    // memory too, and it is not -- GnuPG needs real files for that.
    if (storage.IsAreaResident(ProfileArea::kSecure)) {
      profile_rows.append(
          {.caption = tr("Profile Key:"),
           .value = tr("Held in memory only"),
           .detail = tr(
               "The key that protects this profile's own saved data is never "
               "written here. Your OpenPGP keys are a separate thing and do "
               "live in the session storage above, because GnuPG needs real "
               "files for them.")});
    }
  }

  // Asked only once the key set is attached: Keys() aborts before that, and
  // this tab is reachable from a window that opened without one.
  if (session.KeysLoaded() &&
      session.Keys().Mode() == ProfileKeyMode::kROTATING) {
    profile_rows.append(
        {.caption = tr("Profile Key Rotation:"),
         .value = tr("On a schedule"),
         .detail = tr("New saved data uses the current period's key, and the "
                      "keys that open what earlier periods wrote are kept "
                      "alongside it.")});
  }

  const auto& marker = session.Marker();
  if (marker.schema_version > 0) {
    // "Layout" was this project's word for it and nobody else's. The number is
    // a format version, and saying so is the difference between a reader
    // knowing what it means and guessing.
    profile_rows.append({.caption = tr("Profile Format Version:"),
                         .value = QString::number(marker.schema_version)});
  }

  // The file this window is running out of. The status bar has always shown it
  // and this tab never did, which left the one place that collects everything
  // for a bug report unable to say which document was open.
  if (const auto* packaged = dynamic_cast<const PackagedProfile*>(&profile);
      packaged != nullptr && !packaged->PackagePath().isEmpty()) {
    profile_rows.append(
        {.caption = tr("Profile File:"),
         .value = QDir::toNativeSeparators(packaged->PackagePath()),
         .path = true});
  }

  // A different fact from the row above, and previously wearing its caption.
  // This one is stamped by an *import*, which mints a fresh identity and never
  // touches the file again; the row above is the file a session is open on.
  if (!marker.package_id.isEmpty()) {
    profile_rows.append({.caption = tr("Imported From:"),
                         .value = marker.package_id,
                         .path = true});
  }
  details.append({tr("Profile"), profile_rows});

  // Every path together. Scattered among the readings they belong to, they were
  // the rows a reader had to hunt for, and each one made the card it sat in
  // twice as tall as the readings around it.
  QVector<MetaListRow> folders;
  folders.append({.caption = tr("Profile Folder:"),
                  .value = QDir::toNativeSeparators(profile.Root()),
                  .path = true});

  const auto workspace = ProfileSession::Instance().WorkspacePath();
  if (workspace.isEmpty()) {
    folders.append(
        {.caption = tr("Workspace:"), .value = tr("None"), .dimmed = true});
  } else {
    folders.append({.caption = tr("Workspace:"),
                    .value = QDir::toNativeSeparators(workspace),
                    .path = true});
  }

  // Mirrored onto the application rather than asked of the profile: where this
  // machine keeps its persisted profiles is a property of the installation, not
  // of the one profile in front of us.
  if (const auto profiles_root = qApp->property("GFProfilesRoot").toString();
      !profiles_root.isEmpty()) {
    folders.append({.caption = tr("Profiles Folder:"),
                    .value = QDir::toNativeSeparators(profiles_root),
                    .path = true});
  }
  details.append({tr("Folders"), folders});

  // The engines, and the settings that belong to one of them: a pinentry path
  // is not a property of GpgFrontend, it is how GnuPG asks for a passphrase.
  QVector<MetaListRow> engines;
  for (const auto& engine : GetGSS().AllSupportedEngines()) {
    engines.append({.caption = engine, .value = tr("Available")});
  }

  if (GetGSS().IsEngineSupported(OpenPGPEngine::kGNUPG)) {
    engines.append({.caption = tr("GnuPG Offline Mode:"),
                    .value = gnupg_offline_mode ? tr("Active") : tr("Disabled"),
                    .dimmed = !gnupg_offline_mode});

    // A configured path is a path, and gets the read-only field every other
    // path on this tab gets rather than a label that wraps it at some arbitrary
    // character. The default is a statement, not a path, so it stays a label.
    engines.append(
        {.caption = tr("Pinentry Program Path:"),
         .value = pinentry_program_path.isEmpty()
                      ? tr("Default Pinentry Program")
                      : QDir::toNativeSeparators(pinentry_program_path),
         .dimmed = pinentry_program_path.isEmpty(),
         .path = !pinentry_program_path.isEmpty()});
  }
  details.append({tr("OpenPGP Engines"), engines});

  // Built from the rows themselves rather than accumulated alongside them, so
  // what is copied cannot drift from what is shown -- and so the rows folded
  // away below are copied too. This tab tells the user its values "may help
  // when reporting issues"; the button is what makes that actionable.
  QVector<MetaListRow> summary_rows;

  const auto add_card = [&](QWidget* into_parent, QVBoxLayout* into,
                            const QString& title,
                            const QVector<MetaListRow>& rows) {
    if (rows.isEmpty()) return;

    auto* panel = new MetaListPanel(into_parent);
    panel->SetRows(rows);
    into->addWidget(CreateCard(title, panel, into_parent));

    summary_rows.append({.kind = MetaRowKind::kSection, .caption = title});
    summary_rows.append(rows);
  };

  add_card(content, main_layout, tr("Needs Attention"), attention);
  add_card(content, main_layout, tr("At a Glance"), glance);

  // One card with headings inside it, not four cards of two rows each: a card
  // costs a border, a title and twenty-six pixels of padding, and four of them
  // around eight readings is mostly padding.
  QVector<MetaListRow> more_rows;
  for (const auto& section : details) {
    if (section.second.isEmpty()) continue;
    more_rows.append({.kind = MetaRowKind::kSection, .caption = section.first});
    more_rows.append(section.second);
  }

  auto* more = new MetaListPanel(content);
  more->SetRows(more_rows);
  summary_rows.append(more_rows);

  main_layout->addWidget(CreateDisclosure(
      tr("More details"), CreateCard(tr("Details"), more, content), content));

  main_layout->addStretch();

  auto* copy_button =
      CreateCopyButton(tr("Copy Status Information"),
                       MetaListSummaryText(summary_rows), content);
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
  QVector<MetaListRow> engine_rows;
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