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

#include "ui/help/VersionCheckThread.h"

#include "GpgFrontendBuildInfo.h"
#include "rapidjson/document.h"

using namespace rapidjson;

namespace GpgFrontend::UI {

void VersionCheckThread::run() {
  qDebug() << "Start Version Thread to get latest version from Github";

  auto currentVersion = "v" + QString::number(VERSION_MAJOR) + "." +
                        QString::number(VERSION_MINOR) + "." +
                        QString::number(VERSION_PATCH);

  while (mNetworkReply->isRunning()) {
    QApplication::processEvents();
  }

  if (mNetworkReply->error() != QNetworkReply::NoError) {
    qDebug() << "VersionCheckThread Found Network Error";
    return;
  }

  QByteArray bytes = mNetworkReply->readAll();

  Document d;
  d.Parse(bytes.constData());

  QString latestVersion = d["tag_name"].GetString();

  qDebug() << "Latest Version From Github" << latestVersion;

  QRegularExpression re("^[vV](\\d+\\.)?(\\d+\\.)?(\\*|\\d+)");
  QRegularExpressionMatch match = re.match(latestVersion);
  if (match.hasMatch()) {
    latestVersion = match.captured(0);  // matched == "23 def"
    qDebug() << "Latest Version Matched" << latestVersion;
  } else {
    latestVersion = currentVersion;
    qDebug() << "Latest Version Unknown" << latestVersion;
  }

  if (latestVersion != currentVersion) {
    emit upgradeVersion(currentVersion, latestVersion);
  }
}

VersionCheckThread::VersionCheckThread(QNetworkReply* networkReply)
    : mNetworkReply(networkReply) {}

}  // namespace GpgFrontend::UI
