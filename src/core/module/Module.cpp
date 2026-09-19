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

#include <QLocale>
#include <optional>

#include "core/module/GlobalModuleContext.h"
#include "core/module/ModuleManifest.h"
#include "core/utils/CommonUtils.h"
#include "sdk/GFSDKBuildInfo.h"
#include "sdk/GFSDKModuleApi.h"
#include "sdk/GFSDKModuleAttribution.h"
#include "sdk/GFSDKModuleModel.h"

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
        good_(true) {
    identifier_utf8_ = identifier_.toUtf8();
  }

  Impl(ModuleRawPtr m_ptr, std::unique_ptr<QLibrary> module_library,
       QString module_hash)
      : m_ptr_(m_ptr),
        module_hash_(std::move(module_hash)),
        module_library_(std::move(module_library)),
        module_library_path_(module_library_->fileName()),
        good_(false) {
    // ONE way in. A module describes itself through a single versioned table
    // returned by GFModuleGetApi; the host hands over its own ABI so the
    // module can decline a host it cannot work with, rather than loading and
    // failing on the first mismatched call.
    //
    // The ten separately-resolved symbols this replaces could not express
    // that negotiation at all, and gave the host no place to stand to
    // withhold a capability. A module that does not export the bootstrap
    // symbol is rejected here, by name, rather than half-loaded.
    if (try_bootstrap_api(*module_library_)) return;

    LOG_W() << "illegal module: " << module_library_->fileName()
            << ", reason cannot load symbol: GFModuleGetApi"
            << " (module was built against an older sdk; rebuild it)"
            << ", abort...";
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

    // The same decision the verifier makes about a manifest's sdk_abi, made
    // here about what the module's own table reports. Two ranges compared in
    // two places was how they could come to disagree.
    if (const auto why = SdkAbiRejection(static_cast<int>(api->abi_version));
        why) {
      LOG_W() << "incompatible module: " << module_library.fileName()
              << ", reason: " << *why
              << "; rebuild the module against this sdk, abort...";
      return true;
    }

    identifier_ =
        QString::fromUtf8(api->module_id == nullptr ? "" : api->module_id);
    // Kept as bytes so attribution() can hand out a stable C string without
    // re-encoding on every call into the module.
    identifier_utf8_ = identifier_.toUtf8();
    version_ = QString::fromUtf8(api->version == nullptr ? "" : api->version);
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

  /// Every call that hands control to module code is bracketed so that the
  /// handles it asks for are recorded against it. The SDK cannot work this
  /// out for itself: its host table is one static table shared by every
  /// module, so a call arriving through it carries no identity.
  [[nodiscard]] auto attribution() const -> GFSdkModuleAttributionScope {
    return GFSdkModuleAttributionScope(identifier_utf8_.constData());
  }

  auto Register() -> int {
    if (!good_) return -1;
    // A table-based module has no separate register step: whatever it used to
    // do there belongs in activate(), which is the call that receives the
    // host api it needs in order to do anything at all.
    if (api_ != nullptr) return 0;
    return -1;
  }

  auto Active() -> int {
    if (!good_) return -1;
    if (api_ == nullptr || api_->activate == nullptr) return -1;

    // What the host established about this module, handed over so the module
    // does not have to take its own compiled-in constants as authority. Every
    // pointer below is BORROWED for the duration of this call only: the
    // backing storage is in this frame, and the module is required to copy
    // what it keeps before returning.
    const auto locale_utf8 = QLocale().name().toUtf8();

    QByteArray version_utf8;
    QByteArray context_utf8;
    QList<QByteArray> capability_utf8;
    QList<QByteArray> event_utf8;
    QVector<const char*> capabilities;
    QVector<const char*> events;

    GFModuleBootstrapInfo info{};
    info.struct_size = sizeof(GFModuleBootstrapInfo);
    info.abi_version = GF_SDK_ABI_VERSION;
    info.locale = locale_utf8.constData();

    // Identity always comes from the binary itself, because that is the only
    // thing that exists for a loose build. What the flag records is whether a
    // verified manifest AGREED with it -- and by the time this runs, the load
    // path has already refused the package if it did not.
    info.module_id = identifier_utf8_.constData();
    version_utf8 = version_.toUtf8();
    info.module_version = version_utf8.constData();

    if (manifest_.has_value()) {
      const auto& m = manifest_.value();
      info.flags |= GF_MODULE_BOOT_VERIFIED;

      context_utf8 = m.translation_context.toUtf8();
      if (!context_utf8.isEmpty()) {
        info.translation_context = context_utf8.constData();
      }

      capability_utf8.reserve(m.capabilities.size());
      for (const auto& c : m.capabilities) capability_utf8.append(c.toUtf8());
      capabilities.reserve(capability_utf8.size());
      for (const auto& c : capability_utf8) capabilities.append(c.constData());
      info.capabilities = capabilities.constData();
      info.capabilities_size = static_cast<size_t>(capabilities.size());

      event_utf8.reserve(m.events.size());
      for (const auto& e : m.events) event_utf8.append(e.toUtf8());
      events.reserve(event_utf8.size());
      for (const auto& e : event_utf8) events.append(e.constData());
      info.events = events.constData();
      info.events_size = static_cast<size_t>(events.size());
    }

    const auto attributed = attribution();
    // The host table is static and outlives every module, so the module may
    // hold on to it for its whole life. The bootstrap payload may NOT be.
    return api_->activate(GFGetHostApi(), &info);
  }

  auto Exec(const EventReference& event) -> int {
    if (!good_) return -1;
    if (api_ != nullptr) {
      if (api_->execute == nullptr) return -1;
      const auto attributed = attribution();
      return api_->execute(event->ToModuleEvent());
    }
    return -1;
  }

  auto Deactivate() -> int {
    if (!good_) return -1;
    if (api_ != nullptr) {
      if (api_->deactivate == nullptr) return 0;
      const auto attributed = attribution();
      return api_->deactivate();
    }
    return -1;
  }

  auto UnRegister() -> int {
    if (!good_) return -1;
    if (api_ != nullptr) {
      // Returns void in the table: final teardown has nothing useful to
      // report, and a host that is shutting down has nothing to do with a
      // failure code anyway.
      const auto attributed = attribution();
      if (api_->unregister != nullptr) api_->unregister();
      return 0;
    }
    return -1;
  }

  auto UnloadLibrary() -> bool {
    if (module_library_ == nullptr) return false;

    // Before the unmap, not after: this table has static storage inside the
    // image, so keeping it would leave every entry point dangling. Clearing
    // `good_` too makes every lifecycle call above refuse rather than follow
    // a pointer into memory that is no longer mapped.
    api_ = nullptr;
    good_ = false;

    const auto unloaded = module_library_->unload();
    module_library_.reset();
    return unloaded;
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

  void SetModuleMetaData(const ModuleMetaData& meta_data) {
    meta_data_ = meta_data;
  }

  [[nodiscard]] auto GetModuleMetaData() const -> ModuleMetaData {
    return meta_data_;
  }

  [[nodiscard]] auto GetModulePath() const -> QString {
    return source_package_path_.isEmpty() ? module_library_path_
                                          : source_package_path_;
  }

  void SetSourcePackagePath(QString path) {
    source_package_path_ = std::move(path);
  }

  [[nodiscard]] auto IsPackaged() const -> bool {
    return manifest_.has_value();
  }

  [[nodiscard]] auto GetModuleManifest() const
      -> std::optional<ModuleManifest> {
    return manifest_;
  }

  void SetModuleManifest(const ModuleManifest& manifest) {
    manifest_ = manifest;
  }

  [[nodiscard]] auto GetModuleSDKABIVersion() const -> int {
    return sdk_abi_ver_;
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
  QByteArray identifier_utf8_;

  /// Owned, so that teardown has something to unload. It used to be a
  /// reference to a local in the loader, which meant a successfully loaded
  /// module stayed mapped for the life of the process because nothing had a
  /// handle on it any more.
  std::unique_ptr<QLibrary> module_library_;
  QString module_library_path_;

  /// The `*.gfmodule` this was verified from, for a packaged module.
  QString source_package_path_;

  /// The signed manifest, for a packaged module. Its presence is what
  /// "packaged" means -- a loose library has nothing vouching for it.
  std::optional<ModuleManifest> manifest_;

  /// Keeps the materialised image alive for as long as the module is mapped,
  /// which on Windows is the whole of it -- an open image cannot be unlinked
  /// there, so the mapping's destructor is what removes the file.

  QRegularExpression module_identifier_regex_exp_ = QRegularExpression(
      R"(^([A-Za-z]{1}[A-Za-z\d_]*\.)+[A-Za-z][A-Za-z\d_]*$)");
  QRegularExpression module_version_regex_exp_ =
      QRegularExpression(R"(^(\d+\.)?(\d+\.)?(\*|\d+)$)");

  bool good_;

  int sdk_abi_ver_ = 0;

  /// Non-null when this module described itself through the bootstrap table.
  /// Borrowed: it has static storage inside the module's own library.
  const GFModuleApi* api_ = nullptr;

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

Module::Module(std::unique_ptr<QLibrary> module_library, QString module_hash)
    : p_(SecureCreateUniqueObject<Impl>(this, std::move(module_library),
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

auto Module::UnloadLibrary() -> bool { return p_->UnloadLibrary(); }

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

void Module::SetModuleMetaData(const ModuleMetaData& meta_data) {
  p_->SetModuleMetaData(meta_data);
}

[[nodiscard]] auto Module::GetModulePath() const -> QString {
  return p_->GetModulePath();
}

void Module::SetSourcePackagePath(const QString& path) {
  p_->SetSourcePackagePath(path);
}

[[nodiscard]] auto Module::IsPackaged() const -> bool {
  return p_->IsPackaged();
}

[[nodiscard]] auto Module::GetModuleManifest() const
    -> std::optional<ModuleManifest> {
  return p_->GetModuleManifest();
}

void Module::SetModuleManifest(const ModuleManifest& manifest) {
  p_->SetModuleManifest(manifest);
}

[[nodiscard]] auto Module::GetModuleSDKABIVersion() const -> int {
  return p_->GetModuleSDKABIVersion();
}


[[nodiscard]] auto Module::GetModuleHash() const -> QString {
  return p_->GetModuleHash();
}

void Module::SetGPC(GlobalModuleContext* gpc) { p_->SetGPC(gpc); }
}  // namespace GpgFrontend::Module