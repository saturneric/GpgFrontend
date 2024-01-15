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

#include "GnuPGInfoGatheringModule.h"

#include <vector>

#include "GpgInfo.h"
#include "Log.h"
#include "core/function/gpg/GpgCommandExecutor.h"
#include "core/module/ModuleManager.h"

namespace GpgFrontend::Module::Integrated::GnuPGInfoGatheringModule {

auto CheckBinaryChacksum(QString path) -> std::optional<QString> {
  // check file info and access rights
  QFileInfo info(path);
  if (!info.exists() || !info.isFile() || !info.isReadable()) {
    MODULE_LOG_ERROR("get info for file {} error, exists: {}", info.filePath(),
                     info.exists());
    return {};
  }

  // open and read file
  QFile f(info.filePath());
  if (!f.open(QIODevice::ReadOnly)) {
    MODULE_LOG_ERROR("open {} to calculate check sum error: {}", path,
                     f.errorString());
    return {};
  }

  // read all data from file
  auto buffer = f.readAll();
  f.close();

  auto hash_sha = QCryptographicHash(QCryptographicHash::Sha256);
  // md5
  hash_sha.addData(buffer);
  return QString(hash_sha.result().toHex()).left(6);
}

GnuPGInfoGatheringModule::GnuPGInfoGatheringModule()
    : Module(
          "com.bktus.gpgfrontend.module.integrated.gnupg-info-gathering",
          "1.0.0",
          ModuleMetaData{{"description", "try to gathering gnupg informations"},
                         {"author", "saturneric"}}) {}

GnuPGInfoGatheringModule::~GnuPGInfoGatheringModule() = default;

auto GnuPGInfoGatheringModule::Register() -> bool {
  MODULE_LOG_DEBUG("gnupg info gathering module registering");
  listenEvent("GPGFRONTEND_CORE_INITLIZED");
  return true;
}

auto GnuPGInfoGatheringModule::Active() -> bool {
  MODULE_LOG_DEBUG("gnupg info gathering module activating");
  return true;
}

auto GnuPGInfoGatheringModule::Exec(EventRefrernce event) -> int {
  MODULE_LOG_DEBUG("gnupg info gathering module executing, event id: {}",
                   event->GetIdentifier());

  const auto gpgme_version = RetrieveRTValueTypedOrDefault<>(
      "core", "gpgme.version", QString{"0.0.0"});
  MODULE_LOG_DEBUG("got gpgme version from rt: {}", gpgme_version);

  const auto gpgconf_path = RetrieveRTValueTypedOrDefault<>(
      "core", "gpgme.ctx.gpgconf_path", QString{});
  MODULE_LOG_DEBUG("got gpgconf path from rt: {}", gpgconf_path);

  MODULE_LOG_DEBUG("start to load extra info at module gnupginfogathering...");

  // get all components
  GpgCommandExecutor::ExecuteSync(
      {gpgconf_path,
       {"--list-components"},
       [this, gpgme_version, gpgconf_path](int exit_code, const QString &p_out,
                                           const QString &p_err) {
         MODULE_LOG_DEBUG(
             "gpgconf components exit_code: {} process stdout size: {}",
             exit_code, p_out.size());

         if (exit_code != 0) {
           MODULE_LOG_ERROR(
               "gpgconf execute error, process stderr: {}, "
               "process stdout: {}",
               p_err, p_out);
           return;
         }

         std::vector<GpgComponentInfo> component_infos;
         GpgComponentInfo c_i_gpgme;
         c_i_gpgme.name = "gpgme";
         c_i_gpgme.desc = "GPG Made Easy";
         c_i_gpgme.version = gpgme_version;
         c_i_gpgme.path = _("Embedded In");
         c_i_gpgme.binary_checksum = "/";

         GpgComponentInfo c_i_gpgconf;
         c_i_gpgconf.name = "gpgconf";
         c_i_gpgconf.desc = "GPG Configure";
         c_i_gpgconf.version = "/";
         c_i_gpgconf.path = gpgconf_path;
         auto gpgconf_binary_checksum = CheckBinaryChacksum(gpgconf_path);
         c_i_gpgconf.binary_checksum = (gpgconf_binary_checksum.has_value()
                                            ? gpgconf_binary_checksum.value()
                                            : QString("/"));

         component_infos.push_back(c_i_gpgme);
         component_infos.push_back(c_i_gpgconf);

         auto const jsonlized_gpgme_component_info = c_i_gpgme.Json();
         auto const jsonlized_gpgconf_component_info = c_i_gpgconf.Json();
         UpsertRTValue(GetModuleIdentifier(), "gnupg.components.gpgme",
                       QJsonDocument(jsonlized_gpgme_component_info).toJson());
         UpsertRTValue(
             GetModuleIdentifier(), "gnupg.components.gpgconf",
             QJsonDocument(jsonlized_gpgconf_component_info).toJson());

         auto line_split_list = p_out.split("\n");

         for (const auto &line : line_split_list) {
           auto info_split_list = line.split(":");

           if (info_split_list.size() != 3) continue;

           auto component_name = info_split_list[0].trimmed();
           auto component_desc = info_split_list[1].trimmed();
           auto component_path = info_split_list[2].trimmed();

#ifdef WINDOWS
           // replace some special substrings on windows platform
           component_path.replace("%3a", ":");
#endif

           auto binary_checksum = CheckBinaryChacksum(component_path);

           MODULE_LOG_DEBUG(
               "gnupg component name: {} desc: {} checksum: {} path: {} ",
               component_name, component_desc,
               binary_checksum.has_value() ? binary_checksum.value() : "/",
               component_path);

           QString version = "/";

           if (component_name == "gpg") {
             version = RetrieveRTValueTypedOrDefault<>(
                 "core", "gpgme.ctx.gnupg_version", QString{"2.0.0"});
           }
           if (component_name == "gpg-agent") {
             UpsertRTValue(GetModuleIdentifier(), "gnupg.gpg_agent_path",
                           QString(component_path));
           }
           if (component_name == "dirmngr") {
             UpsertRTValue(GetModuleIdentifier(), "gnupg.dirmngr_path",
                           QString(component_path));
           }
           if (component_name == "keyboxd") {
             UpsertRTValue(GetModuleIdentifier(), "gnupg.keyboxd_path",
                           QString(component_path));
           }

           {
             GpgComponentInfo c_i;
             c_i.name = component_name;
             c_i.desc = component_desc;
             c_i.version = version;
             c_i.path = component_path;
             c_i.binary_checksum =
                 (binary_checksum.has_value() ? binary_checksum.value()
                                              : QString("/"));

             auto const jsonlized_component_info = c_i.Json();
             UpsertRTValue(GetModuleIdentifier(),
                           QString("gnupg.components.%1").arg(component_name),
                           QJsonDocument(jsonlized_component_info).toJson());

             component_infos.push_back(c_i);
           }
         }
       },
       getTaskRunner()});

  GpgCommandExecutor::ExecuteContexts exec_contexts;

  exec_contexts.emplace_back(GpgCommandExecutor::ExecuteContext{
      gpgconf_path,
      {"--list-dirs"},
      [this](int exit_code, const QString &p_out, const QString &p_err) {
        MODULE_LOG_DEBUG(
            "gpgconf configurations exit_code: {} process stdout size: {}",
            exit_code, p_out.size());

        if (exit_code != 0) {
          MODULE_LOG_ERROR(
              "gpgconf execute error, process stderr: {} process stdout: "
              "{}",
              p_err, p_out);
          return;
        }

        auto line_split_list = p_out.split("\n");

        for (const auto &line : line_split_list) {
          auto info_split_list = line.split(":");
          MODULE_LOG_DEBUG("gpgconf info line: {} info size: {}", line,
                           info_split_list.size());

          if (info_split_list.size() != 2) continue;

          auto configuration_name = info_split_list[0].trimmed();
          auto configuration_value = info_split_list[1].trimmed();

#ifdef WINDOWS
          // replace some special substrings on windows platform
          configuration_value.replace("%3a", ":");
#endif

          // record gnupg home path
          if (configuration_name == "homedir") {
            UpsertRTValue(GetModuleIdentifier(), "gnupg.home_path",
                          configuration_value);
          }

          UpsertRTValue(GetModuleIdentifier(),
                        QString("gnupg.dirs.%1").arg(configuration_name),
                        configuration_value);
        }
      },
      getTaskRunner()});

  auto components = ListRTChildKeys(
      "com.bktus.gpgfrontend.module.integrated.gnupg-info-gathering",
      "gnupg.components");

  for (const auto &component : components) {
    auto component_info_json = RetrieveRTValueTypedOrDefault(
        "com.bktus.gpgfrontend.module.integrated.gnupg-info-gathering",
        QString("gnupg.components.%1").arg(component), QByteArray{});

    auto jsonlized_component_info =
        QJsonDocument::fromJson(component_info_json);
    assert(jsonlized_component_info.isObject());

    auto component_info = GpgComponentInfo(jsonlized_component_info.object());
    MODULE_LOG_DEBUG("gpgconf check options ready, component: {}",
                     component_info.name);

    if (component_info.name == "gpgme" || component_info.name == "gpgconf") {
      continue;
    }

    exec_contexts.emplace_back(GpgCommandExecutor::ExecuteContext{
        gpgconf_path,
        {"--list-options", component_info.name},
        [this, component_info](int exit_code, const QString &p_out,
                               const QString &p_err) {
          MODULE_LOG_DEBUG(
              "gpgconf {} avaliable options exit_code: {} process stdout "
              "size: {} ",
              component_info.name, exit_code, p_out.size());

          if (exit_code != 0) {
            MODULE_LOG_ERROR(
                "gpgconf {} avaliable options execute error, process stderr: "
                "{} , process stdout:",
                component_info.name, p_err, p_out);
            return;
          }

          std::vector<GpgOptionsInfo> options_infos;

          auto line_split_list = p_out.split("\n");

          for (const auto &line : line_split_list) {
            auto info_split_list = line.split(":");

            MODULE_LOG_DEBUG(
                "component {} avaliable options line: {} info size: {}",
                component_info.name, line, info_split_list.size());

            if (info_split_list.size() < 10) continue;

            // The format of each line is:
            // name:flags:level:description:type:alt-type:argname:default:argdef:value

            auto option_name = info_split_list[0].trimmed();
            auto option_flags = info_split_list[1].trimmed();
            auto option_level = info_split_list[2].trimmed();
            auto option_desc = info_split_list[3].trimmed();
            auto option_type = info_split_list[4].trimmed();
            auto option_alt_type = info_split_list[5].trimmed();
            auto option_argname = info_split_list[6].trimmed();
            auto option_default = info_split_list[7].trimmed();
            auto option_argdef = info_split_list[8].trimmed();
            auto option_value = info_split_list[9].trimmed();

            GpgOptionsInfo info;
            info.name = option_name;
            info.flags = option_flags;
            info.level = option_level;
            info.description = option_desc;
            info.type = option_type;
            info.alt_type = option_alt_type;
            info.argname = option_argname;
            info.default_value = option_default;
            info.argdef = option_argdef;
            info.value = option_value;

            auto const jsonlized_option_info = info.Json();
            UpsertRTValue(GetModuleIdentifier(),
                          QString("gnupg.components.%1.options.%2")
                              .arg(component_info.name)
                              .arg(option_name),
                          QJsonDocument(jsonlized_option_info).toJson());
            options_infos.push_back(info);
          }
        },
        getTaskRunner()});
  }

  GpgCommandExecutor::ExecuteConcurrentlySync(exec_contexts);
  UpsertRTValue(GetModuleIdentifier(), "gnupg.gathering_done", true);
  event->ExecuteCallback(GetModuleIdentifier(), TransferParams(true));

  MODULE_LOG_DEBUG("gnupg external info gathering done");
  return 0;
}

auto GnuPGInfoGatheringModule::Deactive() -> bool { return true; }

}  // namespace GpgFrontend::Module::Integrated::GnuPGInfoGatheringModule
