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
#include "rapidjson/document.h"
#include "rapidjson/writer.h"

using namespace rapidjson;

namespace GpgFrontend::UI {

AboutDialog::AboutDialog(int defaultIndex, QWidget* parent) : QDialog(parent) {
  this->setWindowTitle(QString(_("About")) + " " + qApp->applicationName());

  auto* tabWidget = new QTabWidget;
  auto* infoTab = new InfoTab();
  auto* translatorsTab = new TranslatorsTab();
  updateTab = new UpdateTab();

  tabWidget->addTab(infoTab, _("General"));
  tabWidget->addTab(translatorsTab, _("Translators"));
  tabWidget->addTab(updateTab, _("Update"));

  connect(tabWidget, &QTabWidget::currentChanged, this,
          [&](int index) { qDebug() << "Current Index" << index; });

  if (defaultIndex < tabWidget->count() && defaultIndex >= 0) {
    tabWidget->setCurrentIndex(defaultIndex);
  }

  auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok);
  connect(buttonBox, SIGNAL(accepted()), this, SLOT(close()));
  
  auto* mainLayout = new QVBoxLayout;
  mainLayout->addWidget(tabWidget);
  mainLayout->addWidget(buttonBox);
  setLayout(mainLayout);

  this->resize(320, 580);
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
      "<center>" + GIT_VERSION + "</center>" +
      _("<br><center>GpgFrontend is an easy-to-use, compact, cross-platform, "
        "<br>"
        "and installation-free gpg front-end tool.<br>"
        "It visualizes most of the common operations of gpg commands.<br>"
        "It's licensed under the GPL v3<br><br>"
        "<b>Developer:</b><br>"
        "Saturneric<br><br>"
        "If you have any questions or suggestions, raise an issue<br/>"
        "at <a href=\"https://github.com/saturneric/GpgFrontend\">GitHub</a> "
        "or send a mail to my mailing list at <a "
        "href=\"mailto:eric@bktus.com\">eric@bktus.com</a>.") +
      "<br><br> " + _("Built with Qt") + " " + qVersion() + " " +
      _("and GPGME") + " " +
      GpgFrontend::GpgContext::getGpgmeVersion().c_str() + "<br>" +
      _("Built at") + " " + BUILD_TIMESTAMP + "</center>");

  auto* layout = new QGridLayout();
  auto* pixmapLabel = new QLabel();
  pixmapLabel->setPixmap(*pixmap);
  layout->addWidget(pixmapLabel, 0, 0, 1, -1, Qt::AlignCenter);
  auto* aboutLabel = new QLabel();
  aboutLabel->setText(*text);
  aboutLabel->setOpenExternalLinks(true);
  layout->addWidget(aboutLabel, 1, 0, 1, -1);
  layout->addItem(
      new QSpacerItem(20, 10, QSizePolicy::Minimum, QSizePolicy::Fixed), 2, 1,
      1, 1);

  setLayout(layout);
}

TranslatorsTab::TranslatorsTab(QWidget* parent) : QWidget(parent) {
  QFile translatorsFile;
  translatorsFile.setFileName(qApp->applicationDirPath() + "/About");
  translatorsFile.open(QIODevice::ReadOnly);
  QByteArray inBuffer = translatorsFile.readAll();

  auto* label = new QLabel(inBuffer);
  auto* mainLayout = new QVBoxLayout(this);
  mainLayout->addWidget(label);

  setLayout(mainLayout);
}

UpdateTab::UpdateTab(QWidget* parent) {
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
      "</center><br><center>" +
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
      QString(
          _("The current version is inconsistent with the latest version on "
            "github.")) +
      "</center><br><center>" + _("Please click") +
      " <a "
      "href=\"https://github.com/saturneric/GpgFrontend/releases\">here</a> " +
      _("to download the latest version.") + "</center>");
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

  connect(this, SIGNAL(replyFromUpdateServer(QByteArray)), this,
          SLOT(processReplyDataFromUpdateServer(QByteArray)));

  setLayout(layout);
}

void UpdateTab::getLatestVersion() {
  this->pb->setHidden(false);

  qDebug() << "Try to get latest version";

  QString baseUrl =
      "https://api.github.com/repos/saturneric/gpgfrontend/releases/latest";

  auto manager = new QNetworkAccessManager(this);

  QNetworkRequest request;
  request.setUrl(QUrl(baseUrl));
  QNetworkReply* replay = manager->get(request);
  auto thread = QThread::create([replay, this]() {
    while (replay->isRunning()) QApplication::processEvents();
    emit replyFromUpdateServer(replay->readAll());
  });
  connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
  thread->start();
}

void UpdateTab::processReplyDataFromUpdateServer(const QByteArray& data) {
  qDebug() << "Try to Process Reply Data From Update Server";

  this->pb->setHidden(true);

  Document d;
  if (d.Parse(data.constData()).HasParseError() || !d.IsObject()) {
    qDebug() << "VersionCheckThread Found Network Error";
    auto latestVersion = "Unknown";
    latestVersionLabel->setText(QString("<center><b>") +
                                _("Latest Version From Github") + ": " +
                                latestVersion + "</b></center>");
    return;
  }

  QString latestVersion = d["tag_name"].GetString();

  qDebug() << "Latest Version From Github" << latestVersion;

  QRegularExpression re(R"(^[vV](\d+\.)?(\d+\.)?(\*|\d+))");
  QRegularExpressionMatch match = re.match(latestVersion);
  if (match.hasMatch()) {
    latestVersion = match.captured(0);
    qDebug() << "Latest Version Matched" << latestVersion;
  } else
    latestVersion = "Unknown";

  latestVersionLabel->setText("<center><b>" +
                              QString(_("Latest Version From Github")) + ": " +
                              latestVersion + "</b></center>");

  if (latestVersion > currentVersion) upgradeLabel->setHidden(false);
}

}  // namespace GpgFrontend::UI
