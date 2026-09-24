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

#include "core/module/ModuleCapability.h"
#include "core/module/ModuleDispatchGate.h"
#include "core/module/ModuleManifest.h"
#include "core/module/ModuleNamespace.h"
#include "core/module/ModuleSdkBridge.h"
#include "sdk/GFSDKBuildInfo.h"
// For the GFHostApi and GFModuleApi TYPES only. gf_core must not reference a
// gf_sdk FUNCTION: that would make the two libraries mutually dependent, which
// neither a MinGW DLL nor a Mach-O dylib can link. The functions this file
// needs arrive through ModuleSdkBridge.
#include "sdk/GFSDKModuleApi.h"
#include "sdk/GFSDKTypes.h"

namespace GpgFrontend::Module {

namespace {

/// How long a deactivation waits for calls already inside the module. Bounded,
/// because such a call may itself be waiting on the thread doing the waiting.
constexpr int kEntryDrainTimeoutMs = 3000;

}  // namespace

class Module::Impl {
 public:
  Impl(std::unique_ptr<QLibrary> module_library, QString module_hash)
      : module_hash_(std::move(module_hash)),
        module_library_(std::move(module_library)),
        module_library_path_(module_library_->fileName()) {
    // ONE way in. A module describes itself through a single versioned table
    // returned by GFModuleGetApi; the host hands over its own ABI so the
    // module can decline a host it cannot work with, rather than loading and
    // failing on the first mismatched call.
    auto* get_api = reinterpret_cast<GFModuleGetApiFn>(
        module_library_->resolve("GFModuleGetApi"));
    if (get_api == nullptr) {
      LOG_W() << "rejected module" << module_library_path_
              << "- missing symbol GFModuleGetApi (built against an older SDK; "
                 "rebuild it)";
      return;
    }

    const auto* api = get_api(GF_SDK_ABI_VERSION);
    if (api == nullptr) {
      LOG_W() << "module" << module_library_path_
              << "declined this host (host SDK ABI version"
              << GF_SDK_ABI_VERSION << ")";
      return;
    }
    adopt(api, module_library_path_);
  }

  Impl(const GFModuleApi* api, QString module_hash)
      : module_hash_(std::move(module_hash)) {
    if (api != nullptr) adopt(api, QStringLiteral("<in-process table>"));
  }

  [[nodiscard]] auto IsGood() const -> bool { return api_ != nullptr; }

  /// Every call that hands control to module code is bracketed so that the
  /// handles created while it runs are recorded against it, including ones
  /// the host creates for it without a module context on hand.
  [[nodiscard]] auto attribution() const -> ModuleAttributionScope {
    return ModuleAttributionScope(identifier_utf8_.constData());
  }

  auto Active() -> int {
    if (api_ == nullptr || api_->activate == nullptr) return -1;

    // The grant is computed from the signed manifest and from nothing the
    // binary said about itself. Every module the host loads came from a
    // verified package, so there is always one; a module without it has
    // nothing a grant could be derived from.
    if (!manifest_.has_value()) {
      LOG_W() << "refusing to activate module" << identifier_
              << ": it has no signed manifest";
      return -1;
    }
    const auto& m = manifest_.value();

    // What the host established about this module, handed over so the module
    // does not have to take its own compiled-in constants as authority. Every
    // pointer below is BORROWED for the duration of this call only.
    const auto locale_utf8 = QLocale().name().toUtf8();
    const auto version_utf8 = version_.toUtf8();
    const auto context_utf8 = m.translation_context.toUtf8();
    const auto capability_utf8 = Utf8(m.capabilities);
    const auto event_utf8 = Utf8(m.events);
    const auto command_utf8 = Utf8(m.commands);
    const auto capabilities = Pointers(capability_utf8);
    const auto events = Pointers(event_utf8);
    const auto commands = Pointers(command_utf8);

    GFModuleBootstrapInfo info{};
    info.struct_size = sizeof(GFModuleBootstrapInfo);
    info.abi_version = GF_SDK_ABI_VERSION;
    info.flags = GF_MODULE_BOOT_VERIFIED;
    info.locale = locale_utf8.constData();
    info.module_id = identifier_utf8_.constData();
    info.module_version = version_utf8.constData();
    if (!context_utf8.isEmpty()) {
      info.translation_context = context_utf8.constData();
    }
    info.capabilities = capabilities.constData();
    info.capabilities_size = static_cast<size_t>(capabilities.size());
    info.events = events.constData();
    info.events_size = static_cast<size_t>(events.size());
    info.commands = commands.constData();
    info.commands_size = static_cast<size_t>(commands.size());

    const auto granted = ModuleCapabilityMask(m.capabilities);
    const auto advisory = AdvisoryDeclarationsOf(m.capabilities);
    LOG_I() << "module" << identifier_ << "granted host capabilities:"
            << QString("%1%2").arg(
                   ModuleCapabilityMaskToString(granted),
                   advisory.isEmpty()
                       ? QString()
                       : QString("; declared but not host-mediated: %1")
                             .arg(advisory.join(", ")));

    // Refused rather than activated with nothing: a module handed a null host
    // api could call nothing and could not say why.
    const auto* host_api =
        ModuleSdkMintHostApi(identifier_utf8_.constData(), granted);
    if (host_api == nullptr) {
      LOG_W() << "refusing to activate module" << identifier_
              << ": the SDK bridge is not installed, so no host API can be "
                 "created for it";
      return -1;
    }
    minted_ = true;

    // Open before activate(): the module may already register commands and
    // widgets there, and the host must be able to call them back.
    ModuleSdkNotifyActivating(identifier_utf8_.constData());
    gate().Open();

    int rc = -1;
    {
      const auto attributed = attribution();
      rc = api_->activate(static_cast<const GFHostApi*>(host_api), &info);
    }

    // A module that refused to activate is not running: it goes back to
    // exactly where it started, whatever it registered on the way.
    if (rc != 0) {
      withdraw();
      ReleaseGrant();
    }
    return rc;
  }

  auto Exec(const EventReference& event) -> int {
    if (api_ == nullptr || api_->execute == nullptr) return -1;
    const auto attributed = attribution();
    return api_->execute(event->ToModuleEvent());
  }

  auto Deactivate(bool revoke) -> int {
    if (api_ == nullptr) return -1;

    // Nothing enters the module from here on, and nothing the host holds for
    // it can reach it again -- BEFORE its own hook runs, so that hook never
    // races a host call into the state it is tearing down.
    const auto drained = withdraw();

    int rc = 0;
    if (!drained) {
      LOG_W() << "module" << identifier_
              << "still has calls inside it; its deactivate hook is skipped "
                 "and it stays inert behind its closed gate";
      rc = -1;
    } else if (api_->deactivate != nullptr) {
      const auto attributed = attribution();
      rc = api_->deactivate();
    }

    if (revoke) ReleaseGrant();
    return rc;
  }

  auto UnRegister() -> int {
    if (api_ == nullptr) return -1;
    // Returns void in the table: final teardown has nothing useful to report.
    const auto attributed = attribution();
    if (api_->unregister != nullptr) api_->unregister();
    return 0;
  }

  void ReleaseGrant() {
    if (!minted_) return;
    ModuleSdkReleaseHostApi(identifier_utf8_.constData());
    minted_ = false;
  }

  auto UnloadLibrary() -> bool {
    // Before the unmap, not after: the table has static storage inside the
    // image, so keeping it would leave every entry point dangling.
    api_ = nullptr;
    // Only a gate this instance opened: a package rejected before activation
    // shares its claimed id with whatever module really owns it.
    if (minted_) gate().Close();

    // The grant dies with the module. A call that somehow arrives afterwards
    // -- from a thread the module failed to stop -- presents a context the
    // host has revoked and is refused, rather than followed into an unmapped
    // image.
    ReleaseGrant();

    if (module_library_ == nullptr) return false;
    const auto unloaded = module_library_->unload();
    module_library_.reset();
    return unloaded;
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

 private:
  ModuleIdentifier identifier_;
  QByteArray identifier_utf8_;
  ModuleVersion version_;
  ModuleMetaData meta_data_;
  QString module_hash_;

  /// Owned, so that teardown has something to unload.
  std::unique_ptr<QLibrary> module_library_;
  QString module_library_path_;

  /// The `*.gfmodule` this was verified from.
  QString source_package_path_;

  /// The signed manifest. Its presence is what "packaged" means.
  std::optional<ModuleManifest> manifest_;

  /// Whether THIS instance holds a grant. A package rejected before
  /// activation shares its claimed id with whatever module really owns it,
  /// and dropping it must not revoke that module's grant.
  bool minted_ = false;

  int sdk_abi_ver_ = 0;

  /// Non-null once the table was accepted. Borrowed: static storage inside
  /// the module's own library.
  const GFModuleApi* api_ = nullptr;

  static auto Utf8(const QStringList& list) -> QList<QByteArray> {
    QList<QByteArray> out;
    out.reserve(list.size());
    for (const auto& s : list) out.append(s.toUtf8());
    return out;
  }

  static auto Pointers(const QList<QByteArray>& list) -> QVector<const char*> {
    QVector<const char*> out;
    out.reserve(list.size());
    for (const auto& s : list) out.append(s.constData());
    return out;
  }

  [[nodiscard]] auto gate() const -> ModuleDispatchGate& {
    return ModuleEntryGate(identifier_);
  }

  /**
   * @brief Stop every host-to-module path and withdraw what the host holds.
   *
   * Idempotent. @return false when calls already inside the module did not
   * finish in time.
   */
  auto withdraw() -> bool {
    gate().Close();
    ModuleSdkNotifyDeactivated(identifier_utf8_.constData());
    return gate().WaitQuiescent(kEntryDrainTimeoutMs);
  }

  /// Validate a module's self-description. Leaves api_ null on refusal.
  void adopt(const GFModuleApi* api, const QString& source) {
    // struct_size is written by whichever side COMPILED the struct, so a
    // module built against an older, smaller table is still usable: only the
    // prefix both sides agree on is read.
    static constexpr size_t kMinUsableSize =
        offsetof(GFModuleApi, unregister) + sizeof(void*);
    if (api->struct_size < kMinUsableSize) {
      LOG_W() << "rejected module" << source
              << "- its module API table is too small:" << api->struct_size
              << "<" << kMinUsableSize;
      return;
    }

    // The same decision the verifier makes about a manifest's sdk_abi.
    if (const auto why = SdkAbiRejection(static_cast<int>(api->abi_version));
        why) {
      LOG_W() << "rejected module" << source << "-" << *why
              << "; rebuild it against this SDK";
      return;
    }

    const auto id =
        QString::fromUtf8(api->module_id == nullptr ? "" : api->module_id);
    const auto version =
        QString::fromUtf8(api->version == nullptr ? "" : api->version);

    // The one identity rule, shared with the manifest parser and the
    // packager, so a binary cannot call itself something a package could
    // never be signed as.
    if (!IsValidModuleId(id)) {
      LOG_W() << "rejected module" << source << "- invalid module id:" << id;
      return;
    }

    static const QRegularExpression kVersion(R"(^(\d+\.)?(\d+\.)?(\*|\d+)$)");
    if (!kVersion.match(version).hasMatch()) {
      LOG_W() << "rejected module" << id << "- invalid version:" << version;
      return;
    }

    identifier_ = id;
    // Kept as bytes so attribution() can hand out a stable C string without
    // re-encoding on every call into the module.
    identifier_utf8_ = id.toUtf8();
    version_ = version;
    sdk_abi_ver_ = static_cast<int>(api->abi_version);
    api_ = api;
  }
};

Module::Module(std::unique_ptr<QLibrary> module_library, QString module_hash)
    : p_(SecureCreateUniqueObject<Impl>(std::move(module_library),
                                        std::move(module_hash))) {}

Module::Module(const GFModuleApi* api, QString module_hash)
    : p_(SecureCreateUniqueObject<Impl>(api, std::move(module_hash))) {}

Module::~Module() = default;

auto Module::IsGood() const -> bool { return p_->IsGood(); }

auto Module::Active() -> int { return p_->Active(); }

auto Module::Exec(const EventReference& event) -> int {
  return p_->Exec(event);
}

auto Module::Deactivate(bool revoke) -> int { return p_->Deactivate(revoke); }

auto Module::UnRegister() -> int { return p_->UnRegister(); }

void Module::ReleaseGrant() { p_->ReleaseGrant(); }

auto Module::UnloadLibrary() -> bool { return p_->UnloadLibrary(); }

auto Module::GetModuleIdentifier() const -> ModuleIdentifier {
  return p_->GetModuleIdentifier();
}

auto Module::GetModuleVersion() const -> ModuleVersion {
  return p_->GetModuleVersion();
}

auto Module::GetModuleMetaData() const -> ModuleMetaData {
  return p_->GetModuleMetaData();
}

void Module::SetModuleMetaData(const ModuleMetaData& meta_data) {
  p_->SetModuleMetaData(meta_data);
}

auto Module::GetModulePath() const -> QString { return p_->GetModulePath(); }

void Module::SetSourcePackagePath(const QString& path) {
  p_->SetSourcePackagePath(path);
}

auto Module::IsPackaged() const -> bool { return p_->IsPackaged(); }

auto Module::GetModuleManifest() const -> std::optional<ModuleManifest> {
  return p_->GetModuleManifest();
}

void Module::SetModuleManifest(const ModuleManifest& manifest) {
  p_->SetModuleManifest(manifest);
}

auto Module::GetModuleSDKABIVersion() const -> int {
  return p_->GetModuleSDKABIVersion();
}

auto Module::GetModuleHash() const -> QString { return p_->GetModuleHash(); }

}  // namespace GpgFrontend::Module
