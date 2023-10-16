/**
 * Copyright (C) 2021 Saturneric <eric@bktus.com>
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
#include "GpgCommandExecutor.h"

#include <memory>
#include <string>

#include "GpgFunctionObject.h"
#include "core/thread/TaskRunnerGetter.h"
#include "thread/DataObject.h"

GpgFrontend::GpgCommandExecutor::GpgCommandExecutor(int channel)
    : SingletonFunctionObject<GpgCommandExecutor>(channel) {}

void GpgFrontend::GpgCommandExecutor::Execute(
    std::string cmd, std::vector<std::string> arguments,
    GpgCommandExecutorCallback callback,
    GpgCommandExecutorInteractor interact_func) {
  SPDLOG_DEBUG("called cmd {} arguments size: {}", cmd, arguments.size());

  Thread::Task::TaskCallback result_callback =
      [](int rtn, Thread::DataObjectPtr data_object) {
        SPDLOG_DEBUG("data object use count: {}", data_object.use_count());
        if (!data_object->Check<int, std::string, std::string>())
          throw std::runtime_error("invalid data object size");

        auto exit_code = Thread::ExtractParams<int>(data_object, 0);
        auto process_stdout =
            Thread::ExtractParams<std::string>(data_object, 1);
        auto process_stderr =
            Thread::ExtractParams<std::string>(data_object, 2);
        auto callback =
            Thread::ExtractParams<GpgCommandExecutorCallback>(data_object, 3);

        // call callback
        callback(exit_code, process_stdout, process_stderr);
      };

  Thread::Task::TaskRunnable runner =
      [](Thread::DataObjectPtr data_object) -> int {
    SPDLOG_DEBUG("process runner called, data object size: {}",
                 data_object->GetObjectSize());

    if (!data_object
             ->Check<std::string, std::string, GpgCommandExecutorInteractor>())
      throw std::runtime_error("invalid data object size");

    // get arguments
    auto cmd = Thread::ExtractParams<std::string>(data_object, 0);
    SPDLOG_DEBUG("get cmd: {}", cmd);
    auto arguments =
        Thread::ExtractParams<std::vector<std::string>>(data_object, 1);
    auto interact_func =
        Thread::ExtractParams<GpgCommandExecutorInteractor>(data_object, 2);

    auto *cmd_process = new QProcess();
    cmd_process->setProcessChannelMode(QProcess::MergedChannels);

    QObject::connect(cmd_process, &QProcess::started,
                     []() -> void { SPDLOG_DEBUG("process started"); });
    QObject::connect(
        cmd_process, &QProcess::readyReadStandardOutput,
        [interact_func, cmd_process]() { interact_func(cmd_process); });
    QObject::connect(cmd_process, &QProcess::errorOccurred,
                     [=](QProcess::ProcessError error) {
                       SPDLOG_ERROR("error in executing command: {} error: {}",
                                    cmd, error);
                     });
    QObject::connect(
        cmd_process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
        [=](int, QProcess::ExitStatus status) {
          if (status == QProcess::NormalExit)
            SPDLOG_DEBUG(
                "proceess finished, succeed in executing command: {}, exit "
                "status: {}",
                cmd, status);
          else
            SPDLOG_ERROR(
                "proceess finished, error in executing command: {}, exit "
                "status: {}",
                cmd, status);
        });

    cmd_process->setProgram(QString::fromStdString(cmd));

    QStringList q_arguments;
    for (const auto &argument : arguments)
      q_arguments.append(QString::fromStdString(argument));
    cmd_process->setArguments(q_arguments);

    SPDLOG_DEBUG("process execute ready, cmd: {} {}", cmd,
                 q_arguments.join(" ").toStdString());

    cmd_process->start();
    cmd_process->waitForFinished();

    std::string process_stdout =
                    cmd_process->readAllStandardOutput().toStdString(),
                process_stderr =
                    cmd_process->readAllStandardError().toStdString();
    int exit_code = cmd_process->exitCode();

    cmd_process->close();
    cmd_process->deleteLater();

    data_object->Swap({exit_code, process_stdout, process_stderr});
    return 0;
  };

  auto *process_task = new GpgFrontend::Thread::Task(
      std::move(runner), fmt::format("Execute/{}", cmd),
      Thread::TransferParams(cmd, arguments, interact_func, callback),
      std::move(result_callback));

  QEventLoop looper;
  QObject::connect(process_task, &Thread::Task::SignalTaskEnd, &looper,
                   &QEventLoop::quit);

  GpgFrontend::Thread::TaskRunnerGetter::GetInstance()
      .GetTaskRunner(Thread::TaskRunnerGetter::kTaskRunnerType_External_Process)
      ->PostTask(process_task);

  // block until task finished
  // this is to keep reference vaild until task finished
  looper.exec();
}

void GpgFrontend::GpgCommandExecutor::ExecuteConcurrently(
    std::string cmd, std::vector<std::string> arguments,
    GpgCommandExecutorCallback callback,
    GpgCommandExecutorInteractor interact_func) {
  SPDLOG_DEBUG("called cmd {} arguments size: {}", cmd, arguments.size());

  Thread::Task::TaskCallback result_callback =
      [](int rtn, Thread::DataObjectPtr data_object) {
        if (!data_object->Check<int, std::string, std::string,
                                GpgCommandExecutorCallback>())
          throw std::runtime_error("invalid data object size");

        auto exit_code = Thread::ExtractParams<int>(data_object, 0);
        auto process_stdout =
            Thread::ExtractParams<std::string>(data_object, 1);
        auto process_stderr =
            Thread::ExtractParams<std::string>(data_object, 2);
        auto callback =
            Thread::ExtractParams<GpgCommandExecutorCallback>(data_object, 3);

        // call callback
        callback(exit_code, process_stdout, process_stderr);
      };

  Thread::Task::TaskRunnable runner =
      [](GpgFrontend::Thread::DataObjectPtr data_object) -> int {
    SPDLOG_DEBUG("process runner called, data object size: {}",
                 data_object->GetObjectSize());

    if (!data_object
             ->Check<std::string, std::string, GpgCommandExecutorInteractor>())
      throw std::runtime_error("invalid data object size");

    // get arguments
    auto cmd = Thread::ExtractParams<std::string>(data_object, 0);
    auto arguments =
        Thread::ExtractParams<std::vector<std::string>>(data_object, 1);
    auto interact_func =
        Thread::ExtractParams<std::function<void(QProcess *)>>(data_object, 2);

    auto *cmd_process = new QProcess();
    cmd_process->setProcessChannelMode(QProcess::MergedChannels);

    QObject::connect(cmd_process, &QProcess::started,
                     []() -> void { SPDLOG_DEBUG("process started"); });
    QObject::connect(
        cmd_process, &QProcess::readyReadStandardOutput,
        [interact_func, cmd_process]() { interact_func(cmd_process); });
    QObject::connect(cmd_process, &QProcess::errorOccurred,
                     [=](QProcess::ProcessError error) {
                       SPDLOG_ERROR("error in executing command: {} error: {}",
                                    cmd, error);
                     });
    QObject::connect(
        cmd_process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
        [=](int, QProcess::ExitStatus status) {
          if (status == QProcess::NormalExit)
            SPDLOG_DEBUG(
                "proceess finished, succeed in executing command: {}, exit "
                "status: {}",
                cmd, status);
          else
            SPDLOG_ERROR(
                "proceess finished, error in executing command: {}, exit "
                "status: {}",
                cmd, status);
        });

    cmd_process->setProgram(QString::fromStdString(cmd));
    cmd_process->setProcessChannelMode(QProcess::SeparateChannels);

    QStringList q_arguments;
    for (const auto &argument : arguments)
      q_arguments.append(QString::fromStdString(argument));
    cmd_process->setArguments(q_arguments);

    SPDLOG_DEBUG("process start ready, cmd: {} {}", cmd,
                 q_arguments.join(" ").toStdString());

    cmd_process->start();
    cmd_process->waitForFinished();

    std::string process_stdout =
                    cmd_process->readAllStandardOutput().toStdString(),
                process_stderr =
                    cmd_process->readAllStandardError().toStdString();
    int exit_code = cmd_process->exitCode();

    cmd_process->close();
    cmd_process->deleteLater();

    data_object->Swap({exit_code, process_stdout, process_stderr});
    return 0;
  };

  // data transfer into task
  auto data_object =
      Thread::TransferParams(cmd, arguments, interact_func, callback);

  auto *process_task = new GpgFrontend::Thread::Task(
      std::move(runner), fmt::format("ExecuteConcurrently/{}", cmd),
      data_object, std::move(result_callback), false);

  GpgFrontend::Thread::TaskRunnerGetter::GetInstance()
      .GetTaskRunner(Thread::TaskRunnerGetter::kTaskRunnerType_External_Process)
      ->PostTask(process_task);
}
