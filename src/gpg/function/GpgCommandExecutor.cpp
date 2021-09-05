/**
 * This file is part of GPGFrontend.
 *
 * GPGFrontend is free software: you can redistribute it and/or modify
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
#include "gpg/function/GpgCommandExecutor.h"

void GpgFrontend::GpgCommandExecutor::Execute(
    const QStringList &arguments,
    const std::function<void(QProcess *)> &interact_func) {
  QEventLoop looper;
  auto *gpg_process = new QProcess(&looper);
  gpg_process->setProcessChannelMode(QProcess::MergedChannels);
  connect(gpg_process,
          qOverload<int, QProcess::ExitStatus>(&QProcess::finished), &looper,
          &QEventLoop::quit);
  connect(gpg_process, &QProcess::errorOccurred,
          []() -> void { qDebug("Error in Process"); });
  connect(gpg_process, &QProcess::errorOccurred, &looper, &QEventLoop::quit);
  connect(gpg_process, &QProcess::started,
          []() -> void { qDebug() << "Gpg Process Started Success"; });
  connect(gpg_process, &QProcess::readyReadStandardOutput,
          [interact_func, gpg_process]() {
            qDebug() << "Function Called";
            interact_func(gpg_process);
          });
  gpg_process->setProgram(ctx.GetInfo().appPath.c_str());
  gpg_process->setArguments(arguments);
  gpg_process->start();
  looper.exec();
}
