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

#include <iterator>
#include <regex>

#include "GpgFrontendBuildInfo.h"
#include "json/json.hpp"

namespace GpgFrontend::UI {

void VersionCheckThread::run() {
  using namespace nlohmann;

  LOG(INFO) << "get latest version from Github";

  auto current_version = std::string("v") + std::to_string(VERSION_MAJOR) +
                         "." + std::to_string(VERSION_MINOR) + "." +
                         std::to_string(VERSION_PATCH);

  while (mNetworkReply->isRunning()) {
    QApplication::processEvents();
  }

  if (mNetworkReply->error() != QNetworkReply::NoError) {
    LOG(ERROR) << "network error";
    return;
  }

  auto bytes = mNetworkReply->readAll();

  auto reply_json = nlohmann::json::parse(bytes.toStdString());

  std::string latest_version = reply_json["tag_name"];

  LOG(INFO) << "latest version from Github" << latest_version;

  std::regex re(R"(^[vV](\d+\.)?(\d+\.)?(\*|\d+))");
  auto version_begin =
      std::sregex_iterator(latest_version.begin(), latest_version.end(), re);
  auto version_end = std::sregex_iterator();
  if (std::distance(version_begin, version_end)) {
    std::smatch match = *version_begin;
    latest_version = match.str();
    LOG(INFO) << "latest version matched" << latest_version;
  } else {
    latest_version = current_version;
    LOG(WARNING) << "latest version unknown";
  }

  emit upgradeVersion(current_version.c_str(), latest_version.c_str());
}

VersionCheckThread::VersionCheckThread(QNetworkReply* networkReply)
    : mNetworkReply(networkReply) {}

}  // namespace GpgFrontend::UI
