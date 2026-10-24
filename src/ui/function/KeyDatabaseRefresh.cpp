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

#include "ui/function/KeyDatabaseRefresh.h"

#include "core/function/CoreSignalStation.h"
#include "core/function/openpgp/AbstractKeyRepository.h"
#include "core/function/openpgp/ChannelHandle.h"
#include "core/model/DataObject.h"
#include "core/module/ModuleManager.h"
#include "core/thread/Task.h"
#include "core/thread/TaskRunnerGetter.h"
#include "core/utils/GpgUtils.h"
#include "ui/UISignalStation.h"
#include "ui/dialog/WaitingDialog.h"

namespace GpgFrontend::UI {

namespace {

bool g_handler_installed = false;

}  // namespace

void InstallKeyDatabaseRefreshHandler() {
  if (g_handler_installed) return;
  g_handler_installed = true;

  QObject::connect(
      UISignalStation::GetInstance(),
      &UISignalStation::SignalKeyDatabaseRefresh,
      UISignalStation::GetInstance(), []() -> void {
        auto* refresh_task = new Thread::Task(
            [](DataObjectPtr) -> int {
              // Flush the key cache of every database in the Runtime in use,
              // each under a lease so a reload cannot reclaim it mid-flush. A
              // retired Runtime's channels are not refreshed: nothing new is
              // read from them any more.
              for (const auto& info : GetGpgKeyDatabaseInfos()) {
                auto lease = TryAcquireEngineLease(info.channel);
                if (!lease) continue;
                LOG_D() << "refreshing key database at channel: "
                        << info.channel;
                AbstractKeyRepository::GetInstance(info.channel).FlushCache();
              }
              LOG_D() << "refreshing key database at all channel done";
              return 0;
            },
            "update_key_database_task");

        QObject::connect(refresh_task, &Thread::Task::SignalTaskEnd,
                         UISignalStation::GetInstance(),
                         &UISignalStation::SignalKeyDatabaseRefreshDone);

        // Modules cannot reach UISignalStation, and some of them cache answers
        // that depend on what is in the keyring -- a verification that said
        // "key missing" stops being true the moment the key is imported. Give
        // them the same notification the UI gets.
        QObject::connect(refresh_task, &Thread::Task::SignalTaskEnd,
                         UISignalStation::GetInstance(), []() -> void {
                           Module::TriggerEvent("KEY_DATABASE_REFRESH_DONE");
                         });

        // post the task to the default task runner
        LOG_D() << "sending key database refresh task to gpg task runner...";
        Thread::TaskRunnerGetter::GetInstance()
            .GetTaskRunner(Thread::TaskRunnerGetter::kTaskRunnerType_GPG)
            ->PostTask(refresh_task);
      });

  // The core says a new Runtime is in use, from whichever thread published
  // it; widgets hear it on the GUI thread.
  QObject::connect(CoreSignalStation::GetInstance(),
                   &CoreSignalStation::SignalKeyDatabaseRuntimePublished,
                   UISignalStation::GetInstance(),
                   &UISignalStation::SignalKeyDatabaseRuntimeChanged,
                   Qt::QueuedConnection);
  QObject::connect(CoreSignalStation::GetInstance(),
                   &CoreSignalStation::SignalKeyDatabaseChannelsRetired,
                   UISignalStation::GetInstance(),
                   &UISignalStation::SignalKeyDatabaseChannelsSuspended,
                   Qt::QueuedConnection);
}

namespace {

/// The binding KeepBoundToKeyDatabase() attaches to its owner.
class KeyDatabaseBinding : public QObject {
 public:
  KeyDatabaseBinding(QObject* owner, std::function<int()> current,
                     std::function<void(int)> rebound,
                     std::function<void()> lost,
                     std::function<void()> suspended)
      : QObject(owner),
        current_(std::move(current)),
        lease_(TryAcquireEngineLease(current_())),
        rebound_(std::move(rebound)),
        lost_(std::move(lost)),
        suspended_(std::move(suspended)) {
    connect(UISignalStation::GetInstance(),
            &UISignalStation::SignalKeyDatabaseRuntimeChanged, this,
            [this](int, int) { follow(); });
    // Let go of a channel retired so its folder can move; the database is
    // found again by id once the new Runtime is published.
    connect(UISignalStation::GetInstance(),
            &UISignalStation::SignalKeyDatabaseChannelsSuspended, this,
            [this]() {
              if (lease_ && LookupChannelHandle(lease_->Handle()).state ==
                                ChannelHandleState::kRETIRED) {
                if (suspended_) suspended_();
                lease_.reset();
              }
            });

    // Made on a channel already retired -- a dialog opened from work that
    // started before a reload: no lease to let go of, and no reload ahead to
    // move it on, so it looks for its database now. Deferred: the owner is
    // still being constructed.
    if (!lease_ && IsRetiredChannelHandle(current_())) {
      QTimer::singleShot(0, this, [this]() { follow(); });
    }
  }

 private:
  std::function<int()> current_;
  std::optional<EngineLease> lease_;
  std::function<void(int)> rebound_;
  std::function<void()> lost_;
  std::function<void()> suspended_;

  void follow() {
    const auto info = LookupChannelHandle(current_());
    if (info.state == ChannelHandleState::kAVAILABLE &&
        info.generation == CurrentRuntimeGeneration()) {
      // The owner may have moved to another channel of this Runtime itself.
      if (!lease_ || lease_->Handle() != info.handle) {
        lease_ = TryAcquireEngineLease(info.handle);
      }
      return;
    }

    const auto next = info.database_id.isEmpty()
                          ? -1
                          : ResolveDatabaseHandle(info.database_id);
    if (next < 0) {
      lease_.reset();
      if (lost_) lost_();
      return;
    }

    // The new lease before the old one goes, so the database is held
    // throughout.
    auto lease = TryAcquireEngineLease(next);
    if (rebound_) rebound_(next);
    lease_ = std::move(lease);
    MarkKeyDatabaseFound(qobject_cast<QWidget*>(parent()));
  }
};

struct ApplyState {
  bool running{false};
  bool again{false};
  QList<KeyDatabaseMove> pending;
  QPointer<QWidget> parent;          ///< where the waiting dialog is shown
  QPointer<WaitingDialog> waiting;  ///< up while an apply runs
};

auto Applying() -> ApplyState& {
  static ApplyState state;
  return state;
}

void ReportApplied(const KeyDatabaseApplyResult& result) {
  if (result.published && result.errors.isEmpty() &&
      result.unavailable.isEmpty()) {
    emit UISignalStation::GetInstance()
        -> SignalRefreshStatusBar(QObject::tr("Key databases updated."), 5000);
    return;
  }

  QStringList lines = result.errors;
  if (!result.unavailable.isEmpty()) {
    lines.append(QObject::tr("These key databases could not be opened: %1")
                     .arg(result.unavailable.join(", ")));
  }
  if (!result.published) {
    lines.append(QObject::tr("The key databases in use are unchanged."));
  }

  emit UISignalStation::GetInstance() -> SignalRefreshStatusBar(
      result.published ? QObject::tr("Key databases updated, with problems.")
                       : QObject::tr("Key database changes were not applied."),
      5000);
  QMessageBox::warning(QApplication::activeWindow(),
                       QObject::tr("Key Database Changes"), lines.join("\n\n"));
}

}  // namespace

void KeepBoundToKeyDatabase(QWidget* owner, std::function<int()> current,
                            std::function<void(int)> rebound,
                            std::function<void()> lost,
                            std::function<void()> suspended) {
  if (owner == nullptr) return;
  if (!lost) lost = [owner]() { MarkKeyDatabaseLost(owner); };
  new KeyDatabaseBinding(owner, std::move(current), std::move(rebound),
                         std::move(lost), std::move(suspended));
}

namespace {

/// The title a widget had before MarkKeyDatabaseLost() changed it.
constexpr const char* kTitleBeforeLostProperty = "GFTitleBeforeKeyDatabaseLost";

}  // namespace

void MarkKeyDatabaseLost(QWidget* widget) {
  if (widget == nullptr) return;
  widget->setEnabled(false);

  // Said once: every later reload that still cannot find the database calls
  // this again.
  if (!widget->isWindow() ||
      widget->property(kTitleBeforeLostProperty).isValid()) {
    return;
  }
  widget->setProperty(kTitleBeforeLostProperty, widget->windowTitle());
  widget->setWindowTitle(QObject::tr("%1 (key database no longer available)")
                             .arg(widget->windowTitle()));
}

void MarkKeyDatabaseFound(QWidget* widget) {
  if (widget == nullptr) return;
  const auto title = widget->property(kTitleBeforeLostProperty);
  if (!title.isValid()) return;

  widget->setProperty(kTitleBeforeLostProperty, QVariant());
  widget->setWindowTitle(title.toString());
  widget->setEnabled(true);
}

auto IsApplyingKeyDatabaseChanges() -> bool { return Applying().running; }

void ApplyKeyDatabaseChanges(const QList<KeyDatabaseMove>& moves,
                             QWidget* parent) {
  auto& state = Applying();
  if (parent != nullptr) state.parent = parent;
  if (state.running) {
    state.pending.append(moves);
    state.again = true;
    return;
  }

  state.running = true;
  emit UISignalStation::GetInstance() -> SignalRefreshStatusBar(
      QObject::tr("Applying key database changes..."), 0);

  // Modal, and closed only when the apply is over. Opening a key database can
  // take a while (GnuPG on Windows especially), and anything started meanwhile
  // would run on a database about to be replaced, or wait for one being
  // opened.
  if (state.waiting == nullptr) {
    state.waiting = new WaitingDialog(
        QObject::tr("Applying Key Database Changes"), false,
        state.parent != nullptr ? state.parent.data()
                                : QApplication::activeWindow());
    state.waiting->setWindowModality(Qt::ApplicationModal);
    state.waiting->SetUserClosable(false);
    state.waiting->show();
  }

  auto result = QSharedPointer<KeyDatabaseApplyResult>::create();
  auto* task = new Thread::Task(
      [moves, result](const DataObjectPtr&) -> int {
        *result = ApplyKeyDatabaseRuntime(moves);
        return 0;
      },
      "apply_key_database_runtime", TransferParams(),
      [result](int, const DataObjectPtr&) {
        auto& state = Applying();
        // Down before the report, which may be a box of its own; a follow-up
        // puts it up again.
        if (state.waiting != nullptr) state.waiting->SlotFinish();

        // Still running while the report is up: it can be a modal box, and a
        // request made from inside its loop must queue behind this one.
        ReportApplied(*result);
        state.running = false;

        if (state.again) {
          state.again = false;
          auto pending = std::move(state.pending);
          state.pending.clear();
          ApplyKeyDatabaseChanges(pending);
          return;
        }

        emit UISignalStation::GetInstance()
            -> SignalKeyDatabaseChangesApplied();
      });

  Thread::TaskRunnerGetter::GetInstance()
      .GetTaskRunner(Thread::TaskRunnerGetter::kTaskRunnerType_Default)
      ->PostTask(task);
}

}  // namespace GpgFrontend::UI
