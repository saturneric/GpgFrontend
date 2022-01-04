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

#ifndef GPGFRONTEND_SETTINGSNETWORK_H
#define GPGFRONTEND_SETTINGSNETWORK_H

#include "ui/GpgFrontendUI.h"

class Ui_NetworkSettings;

namespace GpgFrontend::UI {
class NetworkTab : public QWidget {
  Q_OBJECT

 public:
  explicit NetworkTab(QWidget* parent = nullptr);

  void setSettings();

  void applySettings();

 private slots:

  void slotTestProxyConnectionResult();

 private:
  std::shared_ptr<Ui_NetworkSettings> ui;
  QNetworkProxy::ProxyType proxy_type_ = QNetworkProxy::HttpProxy;

  void apply_proxy_settings();
};
}  // namespace GpgFrontend::UI

#endif  // GPGFRONTEND_SETTINGSNETWORK_H
