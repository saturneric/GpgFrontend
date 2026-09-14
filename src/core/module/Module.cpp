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

#include "Module.h"

#include <optional>

#include "core/module/GlobalModuleContext.h"
#include "core/utils/CommonUtils.h"
#include "sdk/GFSDKBuildInfo.h"
#include "sdk/GFSDKModuleApi.h"
#include "sdk/GFSDKModuleModel.h"
#include "utils/BuildInfoUtils.h"

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

  Impl(ModuleRawPtr m_ptr, QLibrary& module_library, QString module_hash)
      : m_ptr_(m_ptr),
        module_hash_(std::move(module_hash)),
        module_library_path_(module_library.fileName()),
        good_(false) {
    // Prefer the single bootstrap symbol. A module that exports it describes
    // itself through one versioned table instead of ten separately-resolved
    // symbols, which is what lets the host and the module NEGOTIATE an ABI
    // rather than discover a mismatch on the first call.
    //
    // Both paths are accepted while the in-tree modules are ported; the
    // ten-symbol path goes away once nothing uses it.
    if (try_bootstrap_api(module_library)) return;

    for (auto& required_symbol : module_required_symbols_) {
      *required_symbol.pointer =
          reinterpret_cast<void*>(module_library.resolve(required_symbol.name));
      if (*required_symbol.pointer == nullptr) {
        LOG_W() << "illegal module: " << module_library.fileName()
                << ", reason cannot load symbol: " << required_symbol.name
                << ", abort...";
        return;
      }
    }

    // Borrowed, like every other string crossing this boundary: the module
    // returns static storage and the host must not free it. These used to be
    // fresh allocations the host consumed, which is the convention the
    // ownership rule replaced.
    // Borrowed, like every other string crossing this boundary: the module
    // returns static storage and the host must not free it. These used to be
    // fresh allocations the host consumed, which is the convention the
    // ownership rule replaced. Converted inline rather than through the SDK's
    // GFStrView because gf_core cannot link gf_sdk -- gf_sdk depends on it.
    const auto borrowed = [](const char* s) -> QString {
      return s == nullptr ? QString() : QString::fromUtf8(s);
    };

    identifier_ = borrowed(get_id_api_());
    version_ = borrowed(get_version_api_());
    gf_sdk_ver_ = borrowed(get_sdk_ver_api_());
    qt_env_ver_ = borrowed(get_qt_ver_api_());

    if (!module_identifier_regex_exp_.match(identifier_).hasMatch()) {
      LOG_W() << "illegal module: " << identifier_
              << ", reason invalid module id, abort...";
      return;
    }

    if (!module_version_regex_exp_.match(version_).hasMatch()) {
      LOG_W() << "illegal module: " << identifier_
              << ", reason invalid version: " << version_ << ", abort...";
      return;
    }

    if (!module_version_regex_exp_.match(gf_sdk_ver_).hasMatch()) {
      LOG_W() << "illegal module: " << identifier_
              << ", reason invalid sdk version: " << gf_sdk_ver_
              << ", abort...";
      return;
    }

    if (GFCompareSoftwareVersion(gf_sdk_ver_, GetProjectVersion()) > 0) {
      LOG_W() << "uncompatible module: " << identifier_
              << ", reason sdk version: " << gf_sdk_ver_
              << "current sdk version: " << GetProjectVersion() << ", abort...";
      return;
    }

    // The ABI gate proper. The version comparison above is one-sided: it only
    // rejects a module built against a NEWER sdk, so a stale module built
    // against an older ABI passed it, loaded, and then crashed on the first
    // changed entry point. A floor is what actually prevents that.
    sdk_abi_ver_ = get_sdk_abi_api_();
    if (sdk_abi_ver_ < GF_SDK_ABI_MIN_SUPPORTED ||
        sdk_abi_ver_ > GF_SDK_ABI_VERSION) {
      LOG_W() << "incompatible module: " << identifier_
              << ", reason module sdk abi version: " << sdk_abi_ver_
              << ", but this application supports ["
              << GF_SDK_ABI_MIN_SUPPORTED << ", " << GF_SDK_ABI_VERSION
              << "]; rebuild the module against this sdk, abort...";
      return;
    }

    auto qt_env_ver_regex_match = module_version_regex_exp_.match(qt_env_ver_);
    if (!qt_env_ver_regex_match.hasMatch()) {
      LOG_W() << "illegal module: " << identifier_
              << ", reason invalid qt env version: " << qt_env_ver_
              << ", abort...";
      return;
    }

    auto qt_env_ver_major = qt_env_ver_regex_match.captured(1);
    auto qt_env_ver_minor = qt_env_ver_regex_match.captured(2);

    if (qt_env_ver_major != QString::number(QT_VERSION_MAJOR) + "." ||
        qt_env_ver_minor != QString::number(QT_VERSION_MINOR) + ".") {
      LOG_W() << "module: " << identifier_
              << "is not compatible, reason module qt version: " << qt_env_ver_
              << ", but application qt version: "
              << QString::fromUtf8(QT_VERSION_STR) << ", abort...";
      return;
    }

    ::GFModuleMetaData* p_meta_data = get_metadata_api_();

    while (p_meta_data != nullptr) {
      ::GFModuleMetaData* l_meta_data;
      meta_data_[QString::fromUtf8(p_meta_data->key)] =
          QString::fromUtf8(p_meta_data->value);
      l_meta_data = p_meta_data;
      p_meta_data = p_meta_data->next;
      SMAFree(l_meta_data);
    }

    good_ = true;
  }

  /**
   * @brief Resolve and validate the table-based entry point.
   *
   * @return true when this module has been fully decided through the table --
   *         accepted OR rejected. false means "no such symbol, try the older
   *         ten-symbol path", and only that.
   */
  auto try_bootstrap_api(QLibrary& module_library) -> bool {
    auto* get_api = reinterpret_cast<GFModuleGetApiFn>(
        module_library.resolve("GFModuleGetApi"));
    if (get_api == nullptr) return false;

    // Hand the module OUR abi so it can decline a host it cannot work with,
    // rather than being loaded and failing later.
    const auto* api = get_api(GF_SDK_ABI_VERSION);
    if (api == nullptr) {
      LOG_W() << "module declined this host: " << module_library.fileName()
              << ", host sdk abi version: " << GF_SDK_ABI_VERSION
              << ", abort...";
      return true;
    }

    // struct_size is written by whichever side COMPILED the struct, so a
    // module built against an older, smaller table is still usable: only the
    // prefix both sides agree on is read. A table smaller than the fields we
    // actually touch is not.
    static constexpr size_t kMinUsableSize =
        offsetof(GFModuleApi, unregister) + sizeof(void*);
    if (api->struct_size < kMinUsableSize) {
      LOG_W() << "illegal module: " << module_library.fileName()
              << ", reason module api struct is too small: " << api->struct_size
              << "<" << kMinUsableSize << ", abort...";
      return true;
    }

    if (api->abi_version < GF_SDK_ABI_MIN_SUPPORTED ||
        api->abi_version > GF_SDK_ABI_VERSION) {
      LOG_W() << "incompatible module: " << module_library.fileName()
              << ", reason module sdk abi version: " << api->abi_version
              << ", but this application supports ["
              << GF_SDK_ABI_MIN_SUPPORTED << ", " << GF_SDK_ABI_VERSION
              << "]; rebuild the module against this sdk, abort...";
      return true;
    }

    identifier_ = QString::fromUtf8(
        api->module_id == nullptr ? "" : api->module_id);
    version_ =
        QString::fromUtf8(api->version == nullptr ? "" : api->version);
    gf_sdk_ver_ = GetProjectVersion();
    qt_env_ver_ = QString::fromUtf8(QT_VERSION_STR);
    sdk_abi_ver_ = static_cast<int>(api->abi_version);

    if (!module_identifier_regex_exp_.match(identifier_).hasMatch()) {
      LOG_W() << "illegal module: " << module_library.fileName()
              << ", reason invalid module id: " << identifier_ << ", abort...";
      return true;
    }

    if (!module_version_regex_exp_.match(version_).hasMatch()) {
      LOG_W() << "illegal module: " << identifier_
              << ", reason invalid version: " << version_ << ", abort...";
      return true;
    }

    api_ = api;
    good_ = true;
    return true;
  }

  [[nodiscard]] auto IsGood() const -> bool { return good_; }

  auto Register() -> int {
    if (!good_) return -1;
    // A table-based module has no separate register step: whatever it used to
    // do there belongs in activate(), which is the call that receives the
    // host api it needs in order to do anything at all.
    if (api_ != nullptr) return 0;
    if (register_api_ != nullptr) return register_api_();
    return -1;
  }

  auto Active() -> int {
    if (!good_) return -1;
    if (api_ != nullptr) {
      if (api_->activate == nullptr) return -1;
      // The host table is static and outlives every module, so the module may
      // hold on to it for its whole life.
      return api_->activate(GFGetHostApi(), nullptr);
    }
    if (activate_api_ != nullptr) return activate_api_();
    return -1;
  }

  auto Exec(const EventReference& event) -> int {
    if (!good_) return -1;
    if (api_ != nullptr) {
      if (api_->execute == nullptr) return -1;
      return api_->execute(event->ToModuleEvent());
    }
    if (execute_api_ != nullptr) return execute_api_(event->ToModuleEvent());
    return -1;
  }

  auto Deactivate() -> int {
    if (!good_) return -1;
    if (api_ != nullptr) {
      if (api_->deactivate == nullptr) return 0;
      return api_->deactivate();
    }
    if (deactivate_api_ != nullptr) return deactivate_api_();
    return -1;
  }

  auto UnRegister() -> int {
    if (!good_) return -1;
    if (api_ != nullptr) {
      // Returns void in the table: final teardown has nothing useful to
      // report, and a host that is shutting down has nothing to do with a
      // failure code anyway.
      if (api_->unregister != nullptr) api_->unregister();
      return 0;
    }
    if (unregister_api_ != nullptr) return unregister_api_();
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

  [[nodiscard]] auto GetModuleSDKVersion() const -> QString {
    return gf_sdk_ver_;
  }

  [[nodiscard]] auto GetModuleQtEnvVersion() const -> QString {
    return qt_env_ver_;
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

  QRegularExpression module_identifier_regex_exp_ = QRegularExpression(
      R"(^([A-Za-z]{1}[A-Za-z\d_]*\.)+[A-Za-z][A-Za-z\d_]*$)");
  QRegularExpression module_version_regex_exp_ =
      QRegularExpression(R"(^(\d+\.)?(\d+\.)?(\*|\d+)$)");

  bool good_;

  int sdk_abi_ver_ = 0;

  /// Non-null when this module described itself through the bootstrap table.
  /// Borrowed: it has static storage inside the module's own library.
  const GFModuleApi* api_ = nullptr;

  GFModuleAPIGetModuleGFSDKVersion get_sdk_ver_api_;
  GFModuleAPIGetModuleSDKABIVersion get_sdk_abi_api_;
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

  QContainer<Symbol> module_required_symbols_ = {
      {"GFGetModuleGFSDKVersion", reinterpret_cast<void**>(&get_sdk_ver_api_)},
      {"GFGetModuleGFSDKABIVersion",
       reinterpret_cast<void**>(&get_sdk_abi_api_)},
      {"GFGetModuleQtEnvVersion", reinterpret_cast<void**>(&get_qt_ver_api_)},
      {"GFGetModuleID", reinterpret_cast<void**>(&get_id_api_)},
      {"GFGetModuleVersion", reinterpret_cast<void**>(&get_version_api_)},
      {"GFGetModuleMetaData", reinterpret_cast<void**>(&get_metadata_api_)},
      {"GFRegisterModule", reinterpret_cast<void**>(&register_api_)},
      {"GFActiveModule", reinterpret_cast<void**>(&activate_api_)},
      {"GFExecuteModule", reinterpret_cast<void**>(&execute_api_)},
      {"GFDeactivateModule", reinterpret_cast<void**>(&deactivate_api_)},
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

Module::Module(QLibrary& module_library, QString module_hash)
    : p_(SecureCreateUniqueObject<Impl>(this, module_library,
                                        std::move(module_hash))) {}

Module::~Module() = default;

auto Module::IsGood() -> bool { return p_->IsGood(); }

auto Module::Register() -> int { return p_->Register(); }

auto Module::Active() -> int { return p_->Active(); }

auto Module::Exec(EventReference event) -> int {
  LOG_D() << "module" << GetModuleIdentifier() << "executing...";
  return p_->Exec(event);
}

auto Module::Deactivate() -> int { return p_->Deactivate(); }

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

[[nodiscard]] auto Module::GetModuleSDKVersion() const -> QString {
  return p_->GetModuleSDKVersion();
}

[[nodiscard]] auto Module::GetModuleQtEnvVersion() const -> QString {
  return p_->GetModuleQtEnvVersion();
}

void Module::SetGPC(GlobalModuleContext* gpc) { p_->SetGPC(gpc); }
}  // namespace GpgFrontend::Module