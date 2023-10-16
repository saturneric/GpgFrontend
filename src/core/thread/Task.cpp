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

#include "core/thread/Task.h"

#include <memory>

#include "TaskImpl.hpp"

namespace GpgFrontend::Thread {

const std::string DEFAULT_TASK_NAME = "unnamed-task";

Task::Task(std::string name) : p_(std::make_unique<Impl>(this, name)) {}

Task::Task(TaskRunnable runnable, std::string name, DataObjectPtr data_object,
           bool sequency)
    : p_(std::make_unique<Impl>(this, runnable, name, data_object, sequency)) {}

Task::Task(TaskRunnable runnable, std::string name, DataObjectPtr data_object,
           TaskCallback callback, bool sequency)
    : p_(std::make_unique<Impl>(this, runnable, name, data_object, callback,
                                sequency)) {}

Task::~Task() = default;

/**
 * @brief
 *
 * @return std::string
 */
std::string Task::GetFullID() const { return p_->GetFullID(); }

std::string Task::GetUUID() const { return p_->GetUUID(); }

bool Task::GetSequency() const { return p_->GetSequency(); }

void Task::HoldOnLifeCycle(bool hold_on) { p_->HoldOnLifeCycle(hold_on); }

void Task::SetRTN(int rtn) { p_->SetRTN(rtn); }

void Task::SlotRun() { p_->SlotRun(); }

void Task::Run() { p_->Run(); }

void Task::run() { p_->RunnableInterfaceRun(); }

}  // namespace GpgFrontend::Thread