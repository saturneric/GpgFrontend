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

#include "core/function/openpgp/OpenPGPContext.h"

#include <cassert>

#include "core/function/GFKeyDatabase.h"
#include "core/function/basic/GpgFunctionObject.h"
#include "core/function/openpgp/helper/Async.h"
#include "core/function/openpgp/traits/CommonTraits.h"
#include "core/module/ModuleManager.h"
#include "core/utils/GpgUtils.h"
#include "core/utils/MemoryUtils.h"

namespace GpgFrontend {

class OpenPGPContext::Impl {
 public:
  Impl(OpenPGPContext *parent, const OpenPGPContextInitArgs &args)
      : parent_(parent),
        args_(args),
        engine_(args.engine),
        key_db_name_(args.db_name),
        key_db_path_(args.db_path) {}

  ~Impl() {
    if (key_db_ != nullptr) {
      key_db_.clear();
    }
  }

  [[nodiscard]] auto KeyDBName() const -> QString { return key_db_name_; }

  [[nodiscard]] auto KeyDBPath() const -> QString { return key_db_path_; }

  auto Engine() -> OpenPGPEngine { return engine_; }

  auto KeyDatabase() -> QSharedPointer<GFKeyDatabase> {
    assert(engine_ == OpenPGPEngine::kRPGP);
    return engine_ == OpenPGPEngine::kRPGP ? key_db_ : nullptr;
  }

  auto EngineVersion() -> QString {
    return RunRegisteredForward<GetEngineVersionOpTag>(*parent_);
  }

  auto CommonInitialize() -> bool {
    bool good = true;
    if (key_db_name_.isEmpty()) {
      LOG_E() << "key database name is empty";
      good = false;
      return false;
    }

    QFileInfo db_path_info(args_.db_path);
    if (!db_path_info.exists() || !db_path_info.isDir()) {
      LOG_E() << "key database path is not exists or is not a directory: "
              << args_.db_path;
      good = false;
      return false;
    }

    key_db_path_ = db_path_info.absoluteFilePath();
    assert(!key_db_path_.isEmpty());

    key_db_ = SecureCreateSharedObject<GFKeyDatabase>();
    good = key_db_->Init(key_db_path_);

    if (!good) {
      LOG_E() << "OpenPGPContext common initialization failed, channel: "
              << parent_->GetChannel() << ", key db name: " << key_db_name_
              << ", key db path: " << key_db_path_
              << ", engine: " << ConvertOpenPGPEngine2String(engine_);
    }

    return good;
  }

  auto Args() -> OpenPGPContextInitArgs { return args_; }

 private:
  OpenPGPContext *parent_;
  OpenPGPContextInitArgs args_{};  ///<

  OpenPGPEngine engine_;

  QString key_db_name_;
  QString key_db_path_;
  QSharedPointer<GFKeyDatabase> key_db_;
};

OpenPGPContext::OpenPGPContext(int channel)
    : SingletonFunctionObject<OpenPGPContext>(channel),
      p_(SecureCreateUniqueObject<Impl>(this, OpenPGPContextInitArgs{})) {}

OpenPGPContext::OpenPGPContext(const OpenPGPContextInitArgs &args, int channel)
    : SingletonFunctionObject<OpenPGPContext>(channel),
      p_(SecureCreateUniqueObject<Impl>(this, args)) {}

auto OpenPGPContext::Good() const -> bool { return good_; }

OpenPGPContext::~OpenPGPContext() = default;

auto OpenPGPContext::KeyDBName() const -> QString { return p_->KeyDBName(); }

auto OpenPGPContext::KeyDBPath() const -> QString { return p_->KeyDBPath(); }

auto OpenPGPContext::Engine() const -> OpenPGPEngine { return p_->Engine(); }

auto OpenPGPContext::EngineVersion() const -> QString {
  return p_->EngineVersion();
}

auto OpenPGPContext::KeyDatabase() -> QSharedPointer<GFKeyDatabase> {
  return p_->KeyDatabase();
}

auto OpenPGPContext::init(const OpenPGPContextInitArgs &) -> bool {
  LOG_D() << "OpenPGPContext init with engine: "
          << ConvertOpenPGPEngine2String(Engine())
          << ", key db name: " << KeyDBName()
          << ", key db path: " << KeyDBPath() << ", channel: " << GetChannel();
  return true;
}

auto OpenPGPContext::Initialize() -> bool {
  auto args = p_->Args();

  // First do the common initialization, which is shared by different engine
  // implementation, then call the engine specific initialization after the
  // common initialization
  good_ = p_->CommonInitialize() && init(args);

  if (good_) {
    Module::UpsertRTValue(
        "core", QString("gpgme.ctx.list.%1.channel").arg(GetChannel()),
        GetChannel());
    Module::UpsertRTValue(
        "core", QString("gpgme.ctx.list.%1.database_name").arg(GetChannel()),
        args.db_name);
    Module::UpsertRTValue(
        "core", QString("gpgme.ctx.list.%1.database_path").arg(GetChannel()),
        args.db_path);
    Module::UpsertRTValue(
        "core", QString("gpgme.ctx.list.%1.backend_type").arg(GetChannel()),
        args.engine);
  } else {
    LOG_E() << "OpenPGPContext initialization failed, channel: " << GetChannel()
            << ", key db name: " << KeyDatabase()
            << ", key db path: " << KeyDBPath()
            << ", engine: " << ConvertOpenPGPEngine2String(Engine());
  }
  return Good();
}

}  // namespace GpgFrontend