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

#include "ui/GpgFrontendApplication.h"

#include <QElapsedTimer>
#include <QEventLoop>
#include <QFileOpenEvent>
#include <QThread>

#include "core/profile/ProfilePackage.h"
#include "core/utils/BuildInfoUtils.h"

namespace GpgFrontend::UI {

GpgFrontendApplication::GpgFrontendApplication(int &argc, char *argv[])
    : QApplication(argc, argv) {
#ifndef Q_OS_MACOS
  // Try system theme icon first, fall back to resource
  QIcon app_icon = QIcon::fromTheme("com.bktus.gpgfrontend",
                                    QIcon(":/icons/gpgfrontend.png"));
  GpgFrontend::UI::GpgFrontendApplication::setWindowIcon(app_icon);
#endif

  // set the extra information of the build
  GpgFrontendApplication::setApplicationVersion(GetProjectVersion());
  // prevent the "Unknown Organization" in conf path issue on some platforms
  //
  // This is the on-disk identity, not the friendly name: it decides the
  // QSettings scope and the AppLocalData directory, so a non-stable build must
  // get its own or it will share -- and silently damage -- an installed
  // release's profile. The friendly name is set separately just below.
  GpgFrontendApplication::setApplicationName(GetAppProfileName());
  GpgFrontendApplication::setApplicationDisplayName(GetAppDisplayName());
  GpgFrontendApplication::setOrganizationName(GetProjectOrganization());
  GpgFrontendApplication::setQuitOnLastWindowClosed(true);

  // don't show icons in menus
  GpgFrontendApplication::setAttribute(Qt::AA_DontShowIconsInMenus);
}

bool GpgFrontendApplication::notify(QObject *receiver, QEvent *event) {
#ifdef RELEASE
  try {
    return QApplication::notify(receiver, event);
  } catch (const std::exception &ex) {
    FLOG_W("exception was caught in notify: %s", ex.what());
    QMessageBox::information(
        nullptr, tr("Standard Exception Thrown"),
        tr("Oops, an standard exception was thrown "
           "during the running of the "
           "program. This is not a serious problem, it may "
           "be the negligence of the programmer, "
           "please report this problem if you can."));
  } catch (...) {
    FLOG_W("unknown exception was caught in notify");
    QMessageBox::information(
        nullptr, tr("Unhandled Exception Thrown"),
        tr("Oops, an unhandled exception was thrown "
           "during the running of the program. This is not a "
           "serious problem, it may be the negligence of the programmer, "
           "please report this problem if you can."));
  }
  return -1;
#else
  return QApplication::notify(receiver, event);
#endif
}

bool GpgFrontendApplication::event(QEvent *event) {
#ifdef Q_OS_MACOS
  if (event->type() == QEvent::FileOpen) {
    auto *open = static_cast<QFileOpenEvent *>(event);

    auto path = open->file();
    if (path.isEmpty() && open->url().isLocalFile()) {
      path = open->url().toLocalFile();
    }

    if (!path.isEmpty()) {
      // Queued rather than acted on. At launch there is no window to act with
      // and the profile this document selects has not been resolved yet; a
      // moment later there is a window and the profile is fixed. Queuing is the
      // only thing that works in both.
      pending_documents_ << path;
      LOG_I() << "a document was handed over by the system:" << path;
      Q_EMIT SignalDocumentPending();
    }
    return true;
  }
#endif
  return QApplication::event(event);
}

auto GpgFrontendApplication::TakePendingProfilePackage() -> QString {
  for (int i = 0; i < pending_documents_.size(); ++i) {
    if (pending_documents_.at(i).endsWith(kProfilePackageExtension,
                                          Qt::CaseInsensitive)) {
      return pending_documents_.takeAt(i);
    }
  }
  return {};
}

auto GpgFrontendApplication::WaitForLaunchDocument(int timeout_ms) -> bool {
#ifdef Q_OS_MACOS
  /// Below this the loop has not been given a fair chance to deliver anything;
  /// above it, waiting on a launch that carries no document is pure latency.
  constexpr int kFloorMs = 40;
  constexpr int kIdlePasses = 3;
  constexpr int kSliceMs = 5;

  QElapsedTimer timer;
  timer.start();

  // QCoreApplication::processEvents() reports nothing back, so the pass has to
  // go through an event loop of its own: that is the only overload that says
  // whether it actually dispatched anything.
  QEventLoop loop;

  int idle = 0;
  while (pending_documents_.isEmpty() && timer.elapsed() < timeout_ms) {
    const auto busy = loop.processEvents(QEventLoop::ExcludeUserInputEvents);
    idle = busy ? 0 : idle + 1;
    if (timer.elapsed() >= kFloorMs && idle >= kIdlePasses) break;
    // An empty pass returns immediately, so without this the wait would be a
    // spin rather than a wait.
    if (!busy) QThread::msleep(kSliceMs);
  }
  return !pending_documents_.isEmpty();
#else
  Q_UNUSED(timeout_ms)
  return false;
#endif
}

}  // namespace GpgFrontend::UI
