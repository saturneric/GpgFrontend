/**
 * Copyright (C) 2021 Saturneric
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
 * Saturneric<eric@bktus.com> starting on May 12, 2021.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#ifndef GPGFRONTEND_SDK_PLUGIN_H
#define GPGFRONTEND_SDK_PLUGIN_H

#include <core/plugin/Plugin.h>
#include <core/plugin/PluginManager.h>

#include "GpgFrontendPluginSDK.h"
#include "Task.h"

namespace GpgFrontend::Plugin::SDK {

class SPlugin;

using SEventRefrernce = std::shared_ptr<Event>;
using SEventIdentifier = std::string;
using SPluginIdentifier = std::string;
using SPluginVersion = std::string;
using SPluginMetaData = std::map<std::string, std::string>;
using SEvnets = std::vector<Event>;
using GlobalPluginContextPtr = std::shared_ptr<GlobalPluginContext>;
using SPluginPtr = std::shared_ptr<SPlugin>;
using SPluginList = std::list<std::string>;

class GPGFRONTEND_PLUGIN_SDK_EXPORT SPlugin
    : public GpgFrontend::Plugin::Plugin {
  Q_OBJECT
 public:
  SPlugin(SPluginIdentifier, SPluginVersion, SPluginMetaData);

  virtual ~SPlugin() = default;

  SPluginIdentifier GetSPluginIdentifier() const;

  virtual bool Register() override;

  virtual bool Active() override;

  virtual int Exec(SEventRefrernce) override;

  virtual bool Deactive() override;
};

class GPGFRONTEND_PLUGIN_SDK_EXPORT SEvent
    : protected GpgFrontend::Plugin::Event {};

bool GPGFRONTEND_PLUGIN_SDK_EXPORT ListenEvent(SPluginPtr, SEvent);

Thread::TaskRunner* GetPluginTaskRunner(SPlugin*);
}  // namespace GpgFrontend::Plugin::SDK

#endif  // GPGFRONTEND_SDK_PLUGIN_H