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

#include "Module.h"

#include "core/module/GlobalModuleContext.h"
#include "core/utils/CommonUtils.h"
#include "core/utils/IOUtils.h"
#include "module/sdk/GFSDKModule.h"

namespace GpgFrontend::Module {

class Module::Impl {
 public:
  friend class GlobalModuleContext;

  using ExecCallback = std::function<void(int)>;

  Impl(ModuleRawPtr m_ptr, ModuleIdentifier id, ModuleVersion version,
       ModuleMetaData meta_data)
      : m_ptr_(m_ptr),
        identifier_(std::move(id)),
        version_(std::move(version)),
        meta_data_(std::move(meta_data)),
        good_(true) {}

  Impl(ModuleRawPtr m_ptr, QLibrary& module_library)
      : m_ptr_(m_ptr),
        module_hash_(CalculateBinaryChacksum(module_library.fileName())),
        module_library_path_(module_library.fileName()),
        good_(false) {
    for (auto& required_symbol : module_required_symbols_) {
      *required_symbol.pointer =
          reinterpret_cast<void*>(module_library.resolve(required_symbol.name));
      if (*required_symbol.pointer == nullptr) {
        GF_CORE_LOG_WARN(
            "illegal module: {}, reason: cannot load symbol: {}, abort...",
            module_library.fileName(), required_symbol.name);
        return;
      }
    }

    identifier_ = GFUnStrDup(get_id_api_());
    version_ = GFUnStrDup(get_version_api_());

    GF_CORE_LOG_INFO("module loaded, id: {}, verison: {}, hash: {}, path: {}",
                     identifier_, version_, module_hash_, module_library_path_);

    ::GFModuleMetaData* p_meta_data = get_metadata_api_();
    ::GFModuleMetaData* l_meta_data;

    while (p_meta_data != nullptr) {
      meta_data_[QString::fromUtf8(p_meta_data->key)] =
          QString::fromUtf8(p_meta_data->value);
      l_meta_data = p_meta_data;
      p_meta_data = p_meta_data->next;
      SecureFree(l_meta_data);
    }

    good_ = true;
  }

  [[nodiscard]] auto IsGood() const -> bool { return good_; }

  auto Register() -> int {
    if (good_ && register_api_ != nullptr) return register_api_();
    return -1;
  }

  auto Active() -> int {
    if (good_ && activate_api_ != nullptr) return activate_api_();
    return -1;
  }

  auto Exec(const EventRefrernce& event) -> int {
    if (good_ && execute_api_ != nullptr) {
      return execute_api_(event->ToModuleEvent());
    }
    return -1;
  }

  auto Deactive() -> int {
    if (good_ && deactivate_api_ != nullptr) return deactivate_api_();
    return -1;
  }

  auto UnRegister() -> int {
    if (good_ && unregister_api_ != nullptr) return unregister_api_();
    return -1;
  }

  auto GetChannel() -> int { return get_gpc()->GetChannel(m_ptr_); }

  auto GetDefaultChannel() -> int {
    return GlobalModuleContext::GetDefaultChannel(m_ptr_);
  }

  auto GetTaskRunner() -> std::optional<TaskRunnerPtr> {
    return get_gpc()->GetTaskRunner(m_ptr_);
  }

  auto ListenEvent(EventIdentifier event) -> bool {
    return get_gpc()->ListenEvent(GetModuleIdentifier(), std::move(event));
  }

  [[nodiscard]] auto GetModuleIdentifier() const -> ModuleIdentifier {
    return identifier_;
  }

  [[nodiscard]] auto GetModuleVersion() const -> ModuleVersion {
    return version_;
  }

  [[nodiscard]] auto GetModuleMetaData() const -> ModuleMetaData {
    return meta_data_;
  }

  [[nodiscard]] auto GetModulePath() const -> QString {
    return module_library_path_;
  }

  [[nodiscard]] auto GetModuleHash() const -> QString { return module_hash_; }

  void SetGPC(GlobalModuleContext* gpc) { gpc_ = gpc; }

 private:
  GlobalModuleContext* gpc_{};
  Module* m_ptr_;
  ModuleIdentifier identifier_;
  ModuleVersion version_;
  ModuleMetaData meta_data_;
  QString module_hash_;
  QString module_library_path_;
  QString gf_sdk_ver_;
  QString qt_env_ver_;

  bool good_;

  GFModuleAPIGetModuleGFSDKVersion get_sdk_ver_api_;
  GFModuleAPIGetModuleQtEnvVersion get_qt_ver_api_;

  GFModuleAPIGetModuleID get_id_api_;
  GFModuleAPIGetModuleVersion get_version_api_;
  GFModuleAPIGetModuleMetaData get_metadata_api_;
  GFModuleAPIRegisterModule register_api_;
  GFModuleAPIActivateModule activate_api_;
  GFModuleAPIExecuteModule execute_api_;
  GFModuleAPIDeactivateModule deactivate_api_;
  GFModuleAPIUnregisterModule unregister_api_;

  struct Symbol {
    const char* name;
    void** pointer;
  };

  QList<Symbol> module_required_symbols_ = {
      {"GFGetModuleGFSDKVersion", reinterpret_cast<void**>(&get_sdk_ver_api_)},
      {"GFGetModuleQtEnvVersion", reinterpret_cast<void**>(&get_qt_ver_api_)},
      {"GFGetModuleID", reinterpret_cast<void**>(&get_id_api_)},
      {"GFGetModuleVersion", reinterpret_cast<void**>(&get_version_api_)},
      {"GFGetModuleMetaData", reinterpret_cast<void**>(&get_metadata_api_)},
      {"GFRegisterModule", reinterpret_cast<void**>(&register_api_)},
      {"GFActiveModule", reinterpret_cast<void**>(&activate_api_)},
      {"GFExecuteModule", reinterpret_cast<void**>(&execute_api_)},
      {"GFDeactiveModule", reinterpret_cast<void**>(&deactivate_api_)},
      {"GFUnregisterModule", reinterpret_cast<void**>(&unregister_api_)},
  };

  auto get_gpc() -> GlobalModuleContext* {
    if (gpc_ == nullptr) {
      throw std::runtime_error("module is not registered by module manager");
    }
    return gpc_;
  }
};

Module::Module(ModuleIdentifier id, ModuleVersion version,
               const ModuleMetaData& meta_data)
    : p_(SecureCreateUniqueObject<Impl>(this, id, version, meta_data)) {}

Module::Module(QLibrary& module_library)
    : p_(SecureCreateUniqueObject<Impl>(this, module_library)) {}

Module::~Module() = default;

auto Module::IsGood() -> bool { return p_->IsGood(); }

auto Module::Register() -> int { return p_->Register(); }

auto Module::Active() -> int { return p_->Active(); }

auto Module::Exec(EventRefrernce event) -> int {
  return p_->Exec(std::move(event));
}

auto Module::Deactive() -> int { return p_->Deactive(); }

auto Module::UnRegister() -> int { return p_->UnRegister(); }

auto Module::getChannel() -> int { return p_->GetChannel(); }

auto Module::getDefaultChannel() -> int { return p_->GetDefaultChannel(); }

auto Module::getTaskRunner() -> TaskRunnerPtr {
  return p_->GetTaskRunner().value_or(nullptr);
}

auto Module::listenEvent(EventIdentifier event) -> bool {
  return p_->ListenEvent(std::move(event));
}

auto Module::GetModuleIdentifier() const -> ModuleIdentifier {
  return p_->GetModuleIdentifier();
}

[[nodiscard]] auto Module::GetModuleVersion() const -> ModuleVersion {
  return p_->GetModuleVersion();
}

[[nodiscard]] auto Module::GetModuleMetaData() const -> ModuleMetaData {
  return p_->GetModuleMetaData();
}

[[nodiscard]] auto Module::GetModulePath() const -> QString {
  return p_->GetModulePath();
}

[[nodiscard]] auto Module::GetModuleHash() const -> QString {
  return p_->GetModuleHash();
}

void Module::SetGPC(GlobalModuleContext* gpc) { p_->SetGPC(gpc); }
}  // namespace GpgFrontend::Module