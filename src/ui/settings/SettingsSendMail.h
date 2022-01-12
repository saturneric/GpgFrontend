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

#ifndef GPGFRONTEND_SETTINGSSENDMAIL_H
#define GPGFRONTEND_SETTINGSSENDMAIL_H

#include "ui/GpgFrontendUI.h"

class Ui_SendMailSettings;

namespace GpgFrontend::UI {
class SendMailTab : public QWidget {
  Q_OBJECT

 public:
  explicit SendMailTab(QWidget* parent = nullptr);

  void setSettings();

  void applySettings();

 private slots:

  void slotTestSMTPConnectionResult(const QString& result);

#ifdef SMTP_SUPPORT
  void slotCheckConnection();

  void slotSendTestMail();
#endif

 private:
  std::shared_ptr<Ui_SendMailSettings> ui;
  QRegularExpression re_email{
      R"((?:[a-z0-9!#$%&'*+/=?^_`{|}~-]+(?:\.[a-z0-9!#$%&'*+/=?^_`{|}~-]+)*|"(?:[\x01-\x08\x0b\x0c\x0e-\x1f\x21\x23-\x5b\x5d-\x7f]|\\[\x01-\x09\x0b\x0c\x0e-\x7f])*")@(?:(?:[a-z0-9](?:[a-z0-9-]*[a-z0-9])?\.)+[a-z0-9](?:[a-z0-9-]*[a-z0-9])?|\[(?:(?:(2(5[0-5]|[0-4][0-9])|1[0-9][0-9]|[1-9]?[0-9]))\.){3}(?:(2(5[0-5]|[0-4][0-9])|1[0-9][0-9]|[1-9]?[0-9])|[a-z0-9-]*[a-z0-9]:(?:[\x01-\x08\x0b\x0c\x0e-\x1f\x21-\x5a\x53-\x7f]|\\[\x01-\x09\x0b\x0c\x0e-\x7f])+)\]))"};
  SmtpClient::ConnectionType connection_type_ =
      SmtpClient::ConnectionType::TcpConnection;

  void switch_ui_enabled(bool enabled);

  void switch_ui_identity_enabled(bool enabled);

 signals:

  void signalRestartNeeded(bool needed);
};
}  // namespace GpgFrontend::UI

#endif  // GPGFRONTEND_SETTINGSSENDMAIL_H
