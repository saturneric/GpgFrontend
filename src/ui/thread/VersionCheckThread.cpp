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

#include "VersionCheckThread.h"

#include <QMetaType>
#include <nlohmann/json.hpp>

#include "GpgFrontendBuildInfo.h"

namespace GpgFrontend::UI {

void VersionCheckThread::run() {
  auto current_version = std::string("v") + std::to_string(VERSION_MAJOR) +
                         "." + std::to_string(VERSION_MINOR) + "." +
                         std::to_string(VERSION_PATCH);

  SoftwareVersion version;
  version.current_version = current_version;

  auto manager = std::make_unique<QNetworkAccessManager>(nullptr);

  try {
    using namespace nlohmann;
    LOG(INFO) << "current version" << current_version;

    std::string latest_version_url =
        "https://api.github.com/repos/saturneric/gpgfrontend/releases/latest";
    std::string current_version_url =
        "https://api.github.com/repos/saturneric/gpgfrontend/releases/tags/" +
        current_version;

    QNetworkRequest latest_request, current_request;
    latest_request.setUrl(QUrl(latest_version_url.c_str()));
    current_request.setUrl(QUrl(current_version_url.c_str()));
    auto _reply = manager->get(latest_request);
    while (_reply->isRunning()) QApplication::processEvents();
    if (_reply->error() != QNetworkReply::NoError) {
      LOG(ERROR) << "current version request error";
      version.latest_version = current_version;
    } else {
      latest_reply_bytes_ = _reply->readAll();
      auto latest_reply_json =
          nlohmann::json::parse(latest_reply_bytes_.toStdString());

      std::string latest_version = latest_reply_json["tag_name"];

      LOG(INFO) << "latest version from Github" << latest_version;

      QRegularExpression re(R"(^[vV](\d+\.)?(\d+\.)?(\*|\d+))");
      auto version_match = re.match(latest_version.c_str());
      if (version_match.hasMatch()) {
        latest_version = version_match.captured(0).toStdString();
        LOG(INFO) << "latest version matched" << latest_version;
      } else {
        latest_version = current_version;
        LOG(WARNING) << "latest version unknown";
      }

      bool prerelease = latest_reply_json["prerelease"],
           draft = latest_reply_json["draft"];
      std::string publish_date = latest_reply_json["published_at"];
      std::string release_note = latest_reply_json["body"];
      version.latest_version = latest_version;
      version.latest_prerelease = prerelease;
      version.latest_draft = draft;
      version.publish_date = publish_date;
      version.release_note = release_note;
    }

    _reply->deleteLater();

    _reply = manager->get(current_request);
    while (_reply->isRunning()) QApplication::processEvents();
    current_reply_bytes_ = _reply->readAll();
    if (_reply->error() != QNetworkReply::NoError) {
      LOG(ERROR) << "current version request network error";
      version.current_version_found = false;
    } else {
      version.current_version_found = true;
      auto current_reply_json =
          nlohmann::json::parse(current_reply_bytes_.toStdString());
      bool current_prerelease = current_reply_json["prerelease"],
           current_draft = current_reply_json["draft"];
      version.latest_prerelease = current_prerelease;
      version.latest_draft = current_draft;
    }
    _reply->deleteLater();

    // loading done
    version.load_info_done = true;

  } catch (...) {
    LOG(INFO) << "error occurred";
    version.load_info_done = false;
  }
  emit upgradeVersion(version);
}

VersionCheckThread::VersionCheckThread() : QThread(nullptr) {
  qRegisterMetaType<SoftwareVersion>("SoftwareVersion");
};

}  // namespace GpgFrontend::UI
