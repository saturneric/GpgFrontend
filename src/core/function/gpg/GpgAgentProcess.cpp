/**
 * Copyright (C) 2021-2024 Saturneric <eric@bktus.com>
 *
 * This file is part of GpgFrontend.
 *
 * GpgFrontend is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * GpgFrontend is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GpgFrontend. If not, see <https://www.gnu.org/licenses/>.
 *
 * The initial version of the source code is inherited from
 * the gpg4usb project, which is under GPL-3.0-or-later.
 *
 * All the source code of GpgFrontend was modified and released by
 * Saturneric <eric@bktus.com> starting on May 12, 2021.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include "GpgAgentProcess.h"

#include "core/utils/GpgUtils.h"

namespace GpgFrontend {

auto GpgAgentProcess::Start() -> bool {
  assert(!gpg_agent_path_.isEmpty());
  assert(!db_path_.isEmpty());

  if (gpg_agent_path_.trimmed().isEmpty()) {
    LOG_E() << "gpg-agent path is empty!";
    return false;
  }

  LOG_D() << "get gpg-agent path: " << gpg_agent_path_;
  QFileInfo info(gpg_agent_path_);
  if (!info.exists() || !info.isFile()) {
    LOG_E() << "gpg-agent is not exists or is not a binary file!";
    return false;
  }

  auto args = QStringList{};

  if (!db_path_.isEmpty()) {
    args.append({"--homedir", QDir::toNativeSeparators(db_path_)});
  }

  args.append({"--daemon", "--enable-ssh-support"});

  // auto decide pinentry program path
  auto pinentry = DecidePinentry();

  // GFPinentryProgramPath
  auto user_pinentry = qApp->property("GFPinentryProgramPath").toString();
  if (!user_pinentry.isEmpty()) {
    QFileInfo pinentry_info(user_pinentry);
    if (pinentry_info.exists() && pinentry_info.isFile()) {
      pinentry = pinentry_info.absoluteFilePath();
    } else {
      LOG_W() << "the user defined pinentry program path is illegal: "
              << user_pinentry;
    }
  }

  LOG_D() << "decided pinentry program path: " << pinentry;

  if (!pinentry.trimmed().isEmpty()) {
    args.append({"--pinentry-program", pinentry});
  }

  if (channel_ != kGpgFrontendDefaultChannel) {
    args.append("--disable-scdaemon");
  }

  LOG_D() << "gpg-agent start args: " << args << "channel:" << channel_;

  process_.setProgram(info.absoluteFilePath());
  process_.setArguments(args);
  process_.setProcessChannelMode(QProcess::MergedChannels);
  process_.start();

  if (!process_.waitForStarted()) {
    LOG_W() << "timeout starting gpg-agent: " << gpg_agent_path_
            << "ags: " << args;
    return false;
  }

  LOG_D() << "gpg-agent started, channel: " << channel_
          << "pid: " << process_.processId();

  return true;
}

GpgAgentProcess::~GpgAgentProcess() {
  // The spawned gpg-agent is terminated authoritatively via `gpgconf --kill all`
  // (GpgContext::kill_gpg_agent), which is always paired with releasing this
  // object. Do NOT run any blocking QProcess wait here: this QProcess may have
  // been created on a worker thread (channels are built off the main thread)
  // while teardown runs on the main thread, so a cross-thread waitForFinished()
  // cannot observe completion and burns its full timeout -- accumulated across
  // channels that overran the shutdown watchdog and force-exited the process
  // before a pending deep restart could relaunch. Just signal a kill and return.
  if (process_.state() != QProcess::NotRunning) {
    qInfo() << "killing gpg-agent, channel: " << channel_;
    process_.kill();
  }
}
}  // namespace GpgFrontend