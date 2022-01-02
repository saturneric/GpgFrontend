/**
 * This file is part of GpgFrontend.
 *
 * GpgFrontend is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Foobar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Foobar.  If not, see <https://www.gnu.org/licenses/>.
 *
 * The initial version of the source code is inherited from gpg4usb-team.
 * Their source code version also complies with GNU General Public License.
 *
 * The source code version of this software was modified and released
 * by Saturneric<eric@bktus.com> starting on May 12, 2021.
 *
 */

#include "ui/help/AboutDialog.h"

#include "GpgFrontendBuildInfo.h"
#include "function/VersionCheckThread.h"
#include "ui/settings/GlobalSettingStation.h"

namespace GpgFrontend::UI {

AboutDialog::AboutDialog(int defaultIndex, QWidget* parent) : QDialog(parent) {
  this->setWindowTitle(QString(_("About")) + " " + qApp->applicationName());

  auto* tabWidget = new QTabWidget;
  auto* infoTab = new InfoTab();
  auto* translatorsTab = new TranslatorsTab();
  updateTab = new UpdateTab();

  tabWidget->addTab(infoTab, _("About Software"));
  tabWidget->addTab(translatorsTab, _("Translators"));
  tabWidget->addTab(updateTab, _("Update"));

  connect(tabWidget, &QTabWidget::currentChanged, this,
          [&](int index) { LOG(INFO) << "Current Index" << index; });

  if (defaultIndex < tabWidget->count() && defaultIndex >= 0) {
    tabWidget->setCurrentIndex(defaultIndex);
  }

  auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok);
  connect(buttonBox, SIGNAL(accepted()), this, SLOT(close()));

  auto* mainLayout = new QVBoxLayout;
  mainLayout->addWidget(tabWidget);
  mainLayout->addWidget(buttonBox);
  setLayout(mainLayout);

  this->resize(450, 580);
  this->setMinimumWidth(450);
  this->show();
}

void AboutDialog::showEvent(QShowEvent* ev) {
  QDialog::showEvent(ev);
  updateTab->getLatestVersion();
}

InfoTab::InfoTab(QWidget* parent) : QWidget(parent) {
  auto* pixmap = new QPixmap(":gpgfrontend-logo.png");
  auto* text = new QString(
      "<center><h2>" + qApp->applicationName() + "</h2></center>" +
      "<center><b>" + qApp->applicationVersion() + "</b></center>" +
      "<center>" + GIT_VERSION + "</center>" + "<br><center>" +
      _("GpgFrontend is an easy-to-use, compact, cross-platform, "
        "and installation-free gpg front-end tool."
        "It visualizes most of the common operations of gpg commands."
        "It's licensed under the GPL v3") +
      "<br><br>"
      "<b>" +
      _("Developer:") + "</b><br>" + "Saturneric" + "<br><br>" +
      _("If you have any questions or suggestions, raise an issue at") +
      "<br/>"
      " <a href=\"https://github.com/saturneric/GpgFrontend\">GitHub</a> " +
      _("or send a mail to my mailing list at") + " <a " +
      "href=\"mailto:eric@bktus.com\">eric@bktus.com</a>." + "<br><br> " +
      _("Built with Qt") + " " + qVersion() + " " + _("and GPGME") + " " +
      GpgFrontend::GpgContext::GetInstance().GetInfo().GpgMEVersion.c_str() +
      "<br>" + _("Built at") + " " + BUILD_TIMESTAMP + "</center>");

  auto* layout = new QGridLayout();
  auto* pixmapLabel = new QLabel();
  pixmapLabel->setPixmap(*pixmap);
  layout->addWidget(pixmapLabel, 0, 0, 1, -1, Qt::AlignCenter);
  auto* aboutLabel = new QLabel();
  aboutLabel->setText(*text);
  aboutLabel->setWordWrap(true);
  aboutLabel->setOpenExternalLinks(true);
  layout->addWidget(aboutLabel, 1, 0, 1, -1);
  layout->addItem(
      new QSpacerItem(20, 10, QSizePolicy::Minimum, QSizePolicy::Fixed), 2, 1,
      1, 1);

  setLayout(layout);
}

TranslatorsTab::TranslatorsTab(QWidget* parent) : QWidget(parent) {
  QFile translatorsFile;
  auto translators_file =
      GlobalSettingStation::GetInstance().GetResourceDir() / "TRANSLATORS";
  translatorsFile.setFileName(translators_file.string().c_str());
  translatorsFile.open(QIODevice::ReadOnly);
  QByteArray inBuffer = translatorsFile.readAll();

  auto* label = new QLabel(inBuffer);

  auto* mainLayout = new QVBoxLayout(this);
  mainLayout->addWidget(label);
  mainLayout->addStretch();

  auto notice_label = new QLabel(
      _("If you think there are any problems with the translation, why not "
        "participate in the translation work? If you want to participate, "
        "please "
        "read the document or contact me via email."),
      this);
  notice_label->setWordWrap(true);
  mainLayout->addWidget(notice_label);

  setLayout(mainLayout);
}

UpdateTab::UpdateTab(QWidget* parent) : QWidget(parent) {
  auto* pixmap = new QPixmap(":gpgfrontend-logo.png");
  auto* layout = new QGridLayout();
  auto* pixmapLabel = new QLabel();
  pixmapLabel->setPixmap(*pixmap);
  layout->addWidget(pixmapLabel, 0, 0, 1, -1, Qt::AlignCenter);

  currentVersion = "v" + QString::number(VERSION_MAJOR) + "." +
                   QString::number(VERSION_MINOR) + "." +
                   QString::number(VERSION_PATCH);

  auto tipsLabel = new QLabel();
  tipsLabel->setText(
      "<center>" +
      QString(_("It is recommended that you always check the version "
                "of GpgFrontend and upgrade to the latest version.")) +
      "</center><center>" +
      _("New versions not only represent new features, but "
        "also often represent functional and security fixes.") +
      "</center>");
  tipsLabel->setWordWrap(true);

  currentVersionLabel = new QLabel();
  currentVersionLabel->setText("<center>" + QString(_("Current Version")) +
                               _(": ") + "<b>" + currentVersion +
                               "</b></center>");
  currentVersionLabel->setWordWrap(true);

  latestVersionLabel = new QLabel();
  latestVersionLabel->setWordWrap(true);

  upgradeLabel = new QLabel();
  upgradeLabel->setText(
      "<center>" +
      QString(_("The current version is less than the latest version on "
                "github.")) +
      "</center><center>" + _("Please click") +
      " <a "
      "href=\"https://github.com/saturneric/GpgFrontend/releases\">" +
      _("Here") + "</a> " + _("to download the latest version.") + "</center>");
  upgradeLabel->setWordWrap(true);
  upgradeLabel->setOpenExternalLinks(true);
  upgradeLabel->setHidden(true);

  pb = new QProgressBar();
  pb->setRange(0, 0);
  pb->setTextVisible(false);

  layout->addWidget(tipsLabel, 1, 0, 1, -1);
  layout->addWidget(currentVersionLabel, 2, 0, 1, -1);
  layout->addWidget(latestVersionLabel, 3, 0, 1, -1);
  layout->addWidget(upgradeLabel, 4, 0, 1, -1);
  layout->addWidget(pb, 5, 0, 1, -1);
  layout->addItem(
      new QSpacerItem(20, 10, QSizePolicy::Minimum, QSizePolicy::Fixed), 2, 1,
      1, 1);

  setLayout(layout);
}

void UpdateTab::getLatestVersion() {
  this->pb->setHidden(false);

  LOG(INFO) << _("try to get latest version");

  QString base_url =
      "https://api.github.com/repos/saturneric/gpgfrontend/releases/latest";
  QNetworkRequest request;
  request.setUrl(QUrl(base_url));
  auto version_thread = new VersionCheckThread(manager->get(request));

  connect(version_thread, SIGNAL(finished()), version_thread,
          SLOT(deleteLater()));
  connect(version_thread, &VersionCheckThread::upgradeVersion, this,
          &UpdateTab::slotShowVersionStatus);

  version_thread->start();
}

void UpdateTab::slotShowVersionStatus(const QString& current,
                                      const QString& server) {
  this->pb->setHidden(true);

  latestVersionLabel->setText("<center><b>" +
                              QString(_("Latest Version From Github")) + ": " +
                              server + "</b></center>");

  if (current < server) {
    upgradeLabel->show();
  }
}

}  // namespace GpgFrontend::UI
