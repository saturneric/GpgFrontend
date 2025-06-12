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

#include "ModuleInit.h"

#include <openssl/evp.h>
#include <openssl/pem.h>

#include <QCoreApplication>
#include <QDir>

#include "core/function/GlobalSettingStation.h"
#include "core/module/ModuleManager.h"
#include "core/thread/Task.h"
#include "core/thread/TaskRunnerGetter.h"
#include "core/utils/IOUtils.h"

namespace {

auto LoadEmbeddedPublicKey() -> EVP_PKEY* {
  auto [succ, buffer] = GpgFrontend::ReadFileGFBuffer(":/keys/public.pem");
  if (!succ) {
    qWarning()
        << "unable to read public key from resource file: /keys/public.pem";
    return nullptr;
  }

  auto* bio = BIO_new_mem_buf(buffer.Data(), static_cast<int>(buffer.Size()));
  if (bio == nullptr) {
    qWarning() << "BIO_new_mem_buf error";
    return nullptr;
  }

  EVP_PKEY* pub_key = PEM_read_bio_PUBKEY(bio, nullptr, nullptr, nullptr);
  BIO_free(bio);

  if (pub_key == nullptr) {
    qWarning()
        << "PEM_read_bio_PUBKEY parsing failed, is it a valid PEM format?";
  }

  return pub_key;
}

auto VerifyModuleSignature(const GpgFrontend::GFBuffer& lib_data,
                           const GpgFrontend::GFBuffer& sig_data,
                           EVP_PKEY* pub_key) -> bool {
  if (pub_key == nullptr) return false;

  EVP_MD_CTX* md_ctx = EVP_MD_CTX_new();
  if (md_ctx == nullptr) {
    qWarning() << "EVP_MD_CTX_new error";
    return false;
  }

  if (EVP_DigestVerifyInit(md_ctx, nullptr, EVP_sha256(), nullptr, pub_key) <=
      0) {
    qWarning() << "EVP_DigestVerifyInit error";
    EVP_MD_CTX_free(md_ctx);
    return false;
  }

  if (EVP_DigestVerifyUpdate(
          md_ctx, reinterpret_cast<const unsigned char*>(lib_data.Data()),
          static_cast<int>(lib_data.Size())) <= 0) {
    qWarning() << "EVP_DigestVerifyUpdate error";
    EVP_MD_CTX_free(md_ctx);
    return false;
  }

  int ret = EVP_DigestVerifyFinal(
      md_ctx, reinterpret_cast<const unsigned char*>(sig_data.Data()),
      static_cast<int>(sig_data.Size()));

  EVP_MD_CTX_free(md_ctx);

  if (ret == 1) return true;

  if (ret == 0) {
    qWarning()
        << "signature does not match, the module may have been tampered with!";
    return false;
  }

  qWarning() << "EVP_DigestVerifyFinal error";
  return false;
}

auto ValidateModule(const QString& path, EVP_PKEY* key) -> bool {
  auto [succ, mod_buf] = GpgFrontend::ReadFileGFBuffer(path);
  if (!succ) {
    LOG_W() << "cannot read module: " << path;
    return false;
  }

  auto sig_path = path + ".sig";

  auto [succ_1, sig_buf] = GpgFrontend::ReadFileGFBuffer(sig_path);
  if (!succ_1) {
    LOG_W() << "cannot read signature of module: " << sig_path;
    return false;
  }

  if (key == nullptr) return false;

  return VerifyModuleSignature(mod_buf, sig_buf, key);
}

auto SearchModuleFromPath(const QString& mods_path, bool integrated)
    -> QMap<QString, bool> {
  QMap<QString, bool> modules;

  QDir dir(mods_path);
  if (!dir.exists()) return modules;

  const auto entries = dir.entryInfoList(
      QStringList() << "*.so" << "*.dll" << "*.dylib", QDir::Files);

  const QRegularExpression rx(QStringLiteral("^libgf_mod_.+$"));

  for (const auto& info : entries) {
    if (rx.match(info.fileName()).hasMatch()) {
      modules.insert(info.absoluteFilePath(), integrated);
    }
  }

  return modules;
}

auto LoadIntegratedMods() -> QMap<QString, bool> {
  const auto module_path = GpgFrontend::GlobalSettingStation::GetInstance()
                               .GetIntegratedModulePath();
  LOG_I() << "loading integrated modules from path:" << module_path;

  if (!QDir(module_path).exists()) {
    LOG_W() << "integrated modules at path: " << module_path
            << " not found, abort...";
    return {};
  }

  return SearchModuleFromPath(module_path, true);
}

auto LoadExternalMods() -> QMap<QString, bool> {
  auto mods_path =
      GpgFrontend::GlobalSettingStation::GetInstance().GetModulesDir();

  if (!QDir(mods_path).exists()) {
    LOG_W() << "external module directory at path " << mods_path
            << " not found, abort...";
    return {};
  }

  return SearchModuleFromPath(mods_path, false);
}

}  // namespace

namespace GpgFrontend::Module {

void LoadGpgFrontendModules(ModuleInitArgs) {
  // give user ability to give up all modules
  auto disable_loading_all_modules =
      GetSettings().value("basic/disable_loading_all_modules", false).toBool();
  if (disable_loading_all_modules) return;

  // must init at default thread before core
  Thread::TaskRunnerGetter::GetInstance()
      .GetTaskRunner(Thread::TaskRunnerGetter::kTaskRunnerType_Module)
      ->PostTask(new Thread::Task(
          [](const DataObjectPtr&) -> int {
            QMap<QString, bool> modules;

            // if do self checking
            const auto self_check = qApp->property("GFSelfCheck").toBool();

            // only check integrated modules at first
            auto* key = LoadEmbeddedPublicKey();
            QMap<QString, bool> integrated_modules = LoadIntegratedMods();
            for (auto it = integrated_modules.keyValueBegin();
                 it != integrated_modules.keyValueEnd(); ++it) {
              // validate integrated modules
              if (self_check && it->second && !ValidateModule(it->first, key)) {
                LOG_W() << "refuse to load integrated module: " << it->first;
                continue;
              }
              modules.insert(it->first, it->second);
            }

            modules.insert(LoadExternalMods());

            auto& manager = ModuleManager::GetInstance();
            manager.SetNeedRegisterModulesNum(static_cast<int>(modules.size()));

            for (auto it = modules.keyValueBegin(); it != modules.keyValueEnd();
                 ++it) {
              manager.LoadModule(it->first, it->second);
            }

            LOG_D() << "all modules are loaded into memory: " << modules.size();
            return 0;
          },
          "modules_system_init_task"));

  LOG_D() << "are all modules registered? answer: "
          << ModuleManager::GetInstance().IsAllModulesRegistered();
}

void ShutdownGpgFrontendModules() {}

}  // namespace GpgFrontend::Module