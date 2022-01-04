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
 * by Saturneric<eric@bktus.com><eric@bktus.com> starting on May 12, 2021.
 *
 */

#include "TestListedKeyServerThread.h"

void GpgFrontend::UI::TestListedKeyServerThread::run() {
  for (const auto& url : urls_) {
    const auto keyserver_url = url;

    auto key_url = QUrl{keyserver_url};

    LOG(INFO) << "key server domain" << key_url.host().toStdString();

    QTcpSocket socket(nullptr);
    socket.abort();
    socket.connectToHost(key_url.host(), 80);
    if (socket.waitForConnected(timeout_)) {
      result_.push_back("Reachable");
    } else {
      result_.push_back("Not Reachable");
    }
    socket.close();
  }

  emit signalKeyServerListTestResult(result_);
}
