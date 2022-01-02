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

#include "SMTPTestThread.h"
namespace GpgFrontend::UI {

void SMTPTestThread::run() {
  SmtpClient smtp(host_.c_str(), port_, connection_type_);
  if (identify_) {
    smtp.setUser(username_.c_str());
    smtp.setPassword(password_.c_str());
  }
  if (!smtp.connectToHost()) {
    emit signalSMTPTestResult("Fail to connect SMTP server");
    return;
  }
  if (!smtp.login()) {
    emit signalSMTPTestResult("Fail to login");
    return;
  }
  smtp.quit();
  emit signalSMTPTestResult("Succeed in testing connection");
}

}  // namespace GpgFrontend::UI
