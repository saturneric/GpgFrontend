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

#include <boost/algorithm/string.hpp>

#include "core/function/gpg/GpgCommandExecutor.h"

namespace GpgFrontend::Module::Integrated::GnuPGInfoGatheringModule {

std::optional<std::string> check_binary_chacksum(std::filesystem::path path) {
  // check file info and access rights
  QFileInfo info(QString::fromStdString(path.u8string()));
  if (!info.exists() || !info.isFile() || !info.isReadable()) {
    SPDLOG_ERROR("get info for file {} error, exists: {}",
                 info.filePath().toStdString(), info.exists());
    return {};
  }

  // open and read file
  QFile f(info.filePath());
  if (!f.open(QIODevice::ReadOnly)) {
    SPDLOG_ERROR("open {} to calculate check sum error: {}", path.u8string(),
                 f.errorString().toStdString());
    return {};
  }

  // read all data from file
  auto buffer = f.readAll();
  f.close();

  auto hash_sha = QCryptographicHash(QCryptographicHash::Sha256);
  // md5
  hash_sha.addData(buffer);
  auto sha = hash_sha.result().toHex().toStdString();
  SPDLOG_DEBUG("checksum for file {} is {}", path.u8string(), sha);

  return sha.substr(0, 6);
}

GnuPGInfoGatheringModule::GnuPGInfoGatheringModule()
    : Module(
          "com.bktus.gpgfrontend.module.integrated.gnupginfogathering", "1.0.0",
          ModuleMetaData{{"description", "try to gathering gnupg informations"},
                         {"author", "saturneric"}}) {}

GnuPGInfoGatheringModule::~GnuPGInfoGatheringModule() = default;

bool GnuPGInfoGatheringModule::Register() {
  MODULE_LOG_INFO("gnupg info gathering module registering");
  listenEvent("GPGFRONTEND_CORE_INITLIZED");
  return true;
}

bool GnuPGInfoGatheringModule::Active() {
  MODULE_LOG_INFO("gnupg info gathering module activating");
  return true;
}

int GnuPGInfoGatheringModule::Exec(EventRefrernce event) {
  MODULE_LOG_INFO("gnupg info gathering module executing, event id: {}",
                  event->GetIdentifier());

  const auto gpgme_version = RetrieveRTValueTypedOrDefault<>(
      "core", "gpgme.version", std::string{"2.0.0"});
  SPDLOG_DEBUG("got gpgme version from rt: {}", gpgme_version);

  const auto gpgconf_path = RetrieveRTValueTypedOrDefault<>(
      "core", "gpgme.ctx.gpgconf_path", std::string{});
  SPDLOG_DEBUG("got gpgconf path from rt: {}", gpgconf_path);

  SPDLOG_DEBUG("start to load extra info...");

  // get all components
  GpgCommandExecutor::GetInstance().ExecuteSync(
      {gpgconf_path,
       {"--list-components"},
       [this, gpgme_version, gpgconf_path](
           int exit_code, const std::string &p_out, const std::string &p_err) {
         SPDLOG_DEBUG(
             "gpgconf components exit_code: {} process stdout size: {}",
             exit_code, p_out.size());

         if (exit_code != 0) {
           SPDLOG_ERROR(
               "gpgconf execute error, process stderr: {} ,process stdout: "
               "{}",
               p_err, p_out);
           return;
         }

         auto &components_info = info_.ComponentsInfo;
         components_info["gpgme"] = {"GPG Made Easy", gpgme_version,
                                     _("Embedded In"), "/"};

         auto gpgconf_binary_checksum = check_binary_chacksum(gpgconf_path);
         components_info["gpgconf"] = {"GPG Configure", "/", gpgconf_path,
                                       gpgconf_binary_checksum.has_value()
                                           ? gpgconf_binary_checksum.value()
                                           : "/"};

         std::vector<std::string> line_split_list;
         boost::split(line_split_list, p_out, boost::is_any_of("\n"));

         for (const auto &line : line_split_list) {
           std::vector<std::string> info_split_list;
           boost::split(info_split_list, line, boost::is_any_of(":"));

           if (info_split_list.size() != 3) continue;

           auto component_name = info_split_list[0];
           auto component_desc = info_split_list[1];
           auto component_path = info_split_list[2];

           boost::algorithm::trim(component_name);
           boost::algorithm::trim(component_desc);
           boost::algorithm::trim(component_path);

#ifdef WINDOWS
           // replace some special substrings on windows platform
           boost::replace_all(component_path, "%3a", ":");
#endif

           auto binary_checksum = check_binary_chacksum(component_path);

           SPDLOG_DEBUG(
               "gnupg component name: {} desc: {} checksum: {} path: {} ",
               component_name, component_desc,
               binary_checksum.has_value() ? binary_checksum.value() : "/",
               component_path);

           std::string version = "/";

           if (component_name == "gpg") {
             const auto version = RetrieveRTValueTypedOrDefault<>(
                 "core", "gpgme.ctx.gnupg_version", std::string{"2.0.0"});
           }
           if (component_name == "gpg-agent") {
             UpsertRTValue(GetModuleIdentifier(), "gnupg.gpg_agent_path",
                           std::string(component_path));
           }
           if (component_name == "dirmngr") {
             UpsertRTValue(GetModuleIdentifier(), "gnupg.dirmngr_path",
                           std::string(component_path));
           }
           if (component_name == "keyboxd") {
             UpsertRTValue(GetModuleIdentifier(), "gnupg.keyboxd_path",
                           std::string(component_path));
           }

           {
             // try lock
             std::unique_lock lock(info_.Lock);
             // add component info to list
             components_info[component_name] = {
                 component_desc, version, component_path,
                 binary_checksum.has_value() ? binary_checksum.value() : "/"};
           }
         }
       }});

  SPDLOG_DEBUG("start to get dirs info");

  GpgCommandExecutor::ExecuteContexts exec_contexts;

  exec_contexts.emplace_back(GpgCommandExecutor::ExecuteContext{
      gpgconf_path,
      {"--list-dirs"},
      [this](int exit_code, const std::string &p_out,
             const std::string &p_err) {
        SPDLOG_DEBUG(
            "gpgconf configurations exit_code: {} process stdout size: {}",
            exit_code, p_out.size());

        if (exit_code != 0) {
          SPDLOG_ERROR(
              "gpgconf execute error, process stderr: {} process stdout: "
              "{}",
              p_err, p_out);
          return;
        }

        auto &configurations_info = info_.ConfigurationsInfo;

        std::vector<std::string> line_split_list;
        boost::split(line_split_list, p_out, boost::is_any_of("\n"));

        for (const auto &line : line_split_list) {
          std::vector<std::string> info_split_list;
          boost::split(info_split_list, line, boost::is_any_of(":"));
          SPDLOG_DEBUG("gpgconf info line: {} info size: {}", line,
                       info_split_list.size());

          if (info_split_list.size() != 2) continue;

          auto configuration_name = info_split_list[0];
          auto configuration_value = info_split_list[1];
          boost::algorithm::trim(configuration_name);
          boost::algorithm::trim(configuration_value);

#ifdef WINDOWS
          // replace some special substrings on windows platform
          boost::replace_all(configuration_value, "%3a", ":");
#endif

          // record gnupg home path
          if (configuration_name == "homedir") {
            info_.GnuPGHomePath = info_split_list[1];
          }

          {
            // try lock
            std::unique_lock lock(info_.Lock);
            configurations_info[configuration_name] = {configuration_value};
          }
        }
      }});

  SPDLOG_DEBUG("start to get components info");

  for (const auto &component : info_.ComponentsInfo) {
    SPDLOG_DEBUG("gpgconf check options ready", "component", component.first);

    if (component.first == "gpgme" || component.first == "gpgconf") continue;

    exec_contexts.emplace_back(GpgCommandExecutor::ExecuteContext{
        gpgconf_path,
        {"--check-options", component.first},
        [this, component](int exit_code, const std::string &p_out,
                          const std::string &p_err) {
          SPDLOG_DEBUG(
              "gpgconf {} options exit_code: {} process stdout "
              "size: {} ",
              component.first, exit_code, p_out.size());

          if (exit_code != 0) {
            SPDLOG_ERROR(
                "gpgconf {} options execute error, process "
                "stderr: {} , process stdout:",
                component.first, p_err, p_out);
            return;
          }

          auto &options_info = info_.OptionsInfo;

          std::vector<std::string> line_split_list;
          boost::split(line_split_list, p_out, boost::is_any_of("\n"));

          for (const auto &line : line_split_list) {
            std::vector<std::string> info_split_list;
            boost::split(info_split_list, line, boost::is_any_of(":"));

            SPDLOG_DEBUG("component {} options line: {} info size: {}",
                         component.first, line, info_split_list.size());

            if (info_split_list.size() != 6) continue;

            auto configuration_name = info_split_list[0];
            boost::algorithm::trim(configuration_name);
            {
              // try lock
              std::unique_lock lock(info_.Lock);
              options_info[configuration_name] = {
                  info_split_list[1], info_split_list[2], info_split_list[3],
                  info_split_list[4], info_split_list[5]};

              boost::algorithm::trim(options_info[configuration_name][0]);
              boost::algorithm::trim(options_info[configuration_name][1]);
              boost::algorithm::trim(options_info[configuration_name][2]);
              boost::algorithm::trim(options_info[configuration_name][3]);
              boost::algorithm::trim(options_info[configuration_name][4]);
            }
          }
        }});
  }

  SPDLOG_DEBUG("start to get avaliable component options info");

  for (const auto &component : info_.ComponentsInfo) {
    SPDLOG_DEBUG("gpgconf list options ready", "component", component.first);

    if (component.first == "gpgme" || component.first == "gpgconf") continue;

    exec_contexts.emplace_back(GpgCommandExecutor::ExecuteContext{
        gpgconf_path,
        {"--list-options", component.first},
        [this, component](int exit_code, const std::string &p_out,
                          const std::string &p_err) {
          SPDLOG_DEBUG(
              "gpgconf {} avaliable options exit_code: {} process stdout "
              "size: {} ",
              component.first, exit_code, p_out.size());

          if (exit_code != 0) {
            SPDLOG_ERROR(
                "gpgconf {} avaliable options execute error, process stderr: "
                "{} , process stdout:",
                component.first, p_err, p_out);
            return;
          }

          auto &available_options_info = info_.AvailableOptionsInfo;

          std::vector<std::string> line_split_list;
          boost::split(line_split_list, p_out, boost::is_any_of("\n"));

          for (const auto &line : line_split_list) {
            std::vector<std::string> info_split_list;
            boost::split(info_split_list, line, boost::is_any_of(":"));

            SPDLOG_DEBUG(
                "component {} avaliable options line: {} info size: {}",
                component.first, line, info_split_list.size());

            if (info_split_list.size() != 10) continue;

            auto configuration_name = info_split_list[0];
            boost::algorithm::trim(configuration_name);
            {
              // try lock
              std::unique_lock lock(info_.Lock);
              available_options_info[configuration_name] = {
                  info_split_list[1], info_split_list[2], info_split_list[3],
                  info_split_list[4], info_split_list[5], info_split_list[6],
                  info_split_list[7], info_split_list[8], info_split_list[9]};

              boost::algorithm::trim(
                  available_options_info[configuration_name][0]);
              boost::algorithm::trim(
                  available_options_info[configuration_name][1]);
              boost::algorithm::trim(
                  available_options_info[configuration_name][2]);
              boost::algorithm::trim(
                  available_options_info[configuration_name][3]);
              boost::algorithm::trim(
                  available_options_info[configuration_name][4]);
              boost::algorithm::trim(
                  available_options_info[configuration_name][5]);
              boost::algorithm::trim(
                  available_options_info[configuration_name][6]);
              boost::algorithm::trim(
                  available_options_info[configuration_name][7]);
              boost::algorithm::trim(
                  available_options_info[configuration_name][8]);
            }
          }
        }});
  }

  GpgCommandExecutor::GetInstance().ExecuteConcurrentlySync(exec_contexts);

  UpsertRTValue(GetModuleIdentifier(), "gnupg.gathering_done", true);

  return 0;
}

bool GnuPGInfoGatheringModule::Deactive() { return true; }

}  // namespace GpgFrontend::Module::Integrated::GnuPGInfoGatheringModule
