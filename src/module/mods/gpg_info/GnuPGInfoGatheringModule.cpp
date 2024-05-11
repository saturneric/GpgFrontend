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

#include <GFSDKBasic.h>
#include <GFSDKBuildInfo.h>
#include <GFSDKLog.h>
#include <spdlog/spdlog.h>

// qt
#include <QCryptographicHash>
#include <QFileInfo>
#include <QJsonDocument>
#include <QString>

// c++
#include <optional>

#include "GpgInfo.h"

template <>
struct fmt::formatter<QString> {
  // Parses format specifications.
  constexpr auto parse(format_parse_context &ctx) -> decltype(ctx.begin()) {
    return ctx.begin();
  }

  // Formats the QString qstr and writes it to the output.
  template <typename FormatContext>
  auto format(const QString &qstr, FormatContext &ctx) const
      -> decltype(ctx.out()) {
    // Convert QString to UTF-8 QString (to handle Unicode characters
    // correctly)
    QByteArray utf8_array = qstr.toUtf8();
    return fmt::format_to(ctx.out(), "{}", utf8_array.constData());
  }
};

template <>
struct fmt::formatter<QByteArray> {
  // Parses format specifications.
  constexpr auto parse(format_parse_context &ctx) -> decltype(ctx.begin()) {
    return ctx.begin();
  }

  // Formats the QString qstr and writes it to the output.
  template <typename FormatContext>
  auto format(const QByteArray &qarray, FormatContext &ctx) const
      -> decltype(ctx.out()) {
    // Convert QString to UTF-8 QString (to handle Unicode characters
    // correctly)
    return fmt::format_to(ctx.out(), "{}", qarray.constData());
  }
};

extern auto CalculateBinaryChacksum(const QString &path)
    -> std::optional<QString>;

extern void GetGpgComponentInfos(void *, int, const char *, const char *);

extern void GetGpgDirectoryInfos(void *, int, const char *, const char *);

extern void GetGpgOptionInfos(void *, int, const char *, const char *);

using Context = struct {
  QString gpgme_version;
  QString gpgconf_path;
  GpgComponentInfo component_info;
};

auto GFGetModuleGFSDKVersion() -> const char * {
  return GFModuleStrDup(GF_SDK_VERSION_STR);
}

auto GFGetModuleQtEnvVersion() -> const char * {
  return GFModuleStrDup(QT_VERSION_STR);
}

auto GFGetModuleID() -> const char * {
  return GFModuleStrDup("com.bktus.gpgfrontend.module.gnupg_info_gathering");
}

auto GFGetModuleVersion() -> const char * { return GFModuleStrDup("1.0.0"); }

auto GFGetModuleMetaData() -> GFModuleMetaData * {
  auto *p_meta = static_cast<GFModuleMetaData *>(
      GFAllocateMemory(sizeof(GFModuleMetaData)));
  auto *h_meta = p_meta;

  p_meta->key = "Name";
  p_meta->value = "GatherGnupgInfo";
  p_meta->next = static_cast<GFModuleMetaData *>(
      GFAllocateMemory(sizeof(GFModuleMetaData)));
  p_meta = p_meta->next;

  p_meta->key = "Description";
  p_meta->value = "Try gathering gnupg informations";
  p_meta->next = static_cast<GFModuleMetaData *>(
      GFAllocateMemory(sizeof(GFModuleMetaData)));
  p_meta = p_meta->next;

  p_meta->key = "Author";
  p_meta->value = "Saturneric";
  p_meta->next = nullptr;
  return h_meta;
}

auto GFRegisterModule() -> int {
  GFModuleLogDebug("gnupg info gathering module registering");
  return 0;
}

auto GFActiveModule() -> int {
  GFModuleLogDebug("gnupg info gathering module activating");
  GFModuleListenEvent(GFGetModuleID(),
                      GFModuleStrDup("REQUEST_GATHERING_GNUPG_INFO"));
  return 0;
}

auto GFExecuteModule(GFModuleEvent *event) -> int {
  GFModuleLogDebug(
      fmt::format("gnupg info gathering module executing, event id: {}",
                  event->id)
          .c_str());

  GFModuleLogDebug("start to load extra info at module gnupginfogathering...");

  const auto *const gpgme_version = GFModuleRetrieveRTValueOrDefault(
      GFModuleStrDup("core"), GFModuleStrDup("gpgme.version"),
      GFModuleStrDup("0.0.0"));
  GFModuleLogDebug(
      fmt::format("got gpgme version from rt: {}", gpgme_version).c_str());

  const auto *const gpgconf_path = GFModuleRetrieveRTValueOrDefault(
      GFModuleStrDup("core"), GFModuleStrDup("gpgme.ctx.gpgconf_path"),
      GFModuleStrDup(""));
  GFModuleLogDebug(
      fmt::format("got gpgconf path from rt: {}", gpgconf_path).c_str());

  auto context = Context{gpgme_version, gpgconf_path};

  // get all components
  const char *argv[] = {GFModuleStrDup("--list-components")};
  GFExecuteCommandSync(gpgconf_path, 1, argv, GetGpgComponentInfos, &context);
  GFModuleLogDebug("load gnupg component info done.");

#ifdef QT5_BUILD
  QVector<GFCommandExecuteContext> exec_contexts;
#else
  QList<GFCommandExecuteContext> exec_contexts;
#endif

  const char **argv_0 =
      static_cast<const char **>(GFAllocateMemory(sizeof(const char *)));
  argv_0[0] = GFModuleStrDup("--list-dirs");

  exec_contexts.push_back(
      {gpgconf_path, 1, argv_0, GetGpgDirectoryInfos, nullptr});

  char **components_c_array;
  int ret = GFModuleListRTChildKeys(
      GFGetModuleID(), GFModuleStrDup("gnupg.components"), &components_c_array);
  if (components_c_array == nullptr || ret == 0) return -1;

  QStringList components;
  auto *p_a = components_c_array;
  for (int i = 0; i < ret; i++) components.append(QString::fromUtf8(p_a[i]));

  for (const auto &component : components) {
    const auto *component_info_json = GFModuleRetrieveRTValueOrDefault(
        GFGetModuleID(),
        GFModuleStrDup(QString("gnupg.components.%1").arg(component).toUtf8()),
        nullptr);

    if (component_info_json == nullptr) continue;

    auto jsonlized_component_info =
        QJsonDocument::fromJson(component_info_json);
    assert(jsonlized_component_info.isObject());

    auto component_info = GpgComponentInfo(jsonlized_component_info.object());
    GFModuleLogDebug(fmt::format("gpgconf check options ready, "
                                 "component: {}",
                                 component_info.name)
                         .c_str());

    if (component_info.name == "gpgme" || component_info.name == "gpgconf") {
      continue;
    }

    auto *context = new (GFAllocateMemory(sizeof(Context)))
        Context{gpgme_version, gpgconf_path, component_info};

    const char **argv_0 =
        static_cast<const char **>(GFAllocateMemory(sizeof(const char *) * 2));
    argv_0[0] = GFModuleStrDup("--list-options"),
    argv_0[1] = GFModuleStrDup(component_info.name.toUtf8());
    exec_contexts.push_back(
        {gpgconf_path, 2, argv_0, GetGpgOptionInfos, context});
  }

  GFExecuteCommandBatchSync(static_cast<int32_t>(exec_contexts.size()),
                            exec_contexts.constData());
  GFModuleUpsertRTValueBool(GFGetModuleID(),
                            GFModuleStrDup("gnupg.gathering_done"), 1);

  char **event_argv =
      static_cast<char **>(GFAllocateMemory(sizeof(char **) * 1));
  event_argv[0] = GFModuleStrDup("0");

  GFModuleTriggerModuleEventCallback(event, GFGetModuleID(), 1, event_argv);

  GFModuleLogDebug("gnupg external info gathering done");
  return 0;
}

auto GFDeactiveModule() -> int { return 0; }

auto GFUnregisterModule() -> int {
  GFModuleLogDebug("gnupg info gathering module unregistering");
  return 0;
}

auto CalculateBinaryChacksum(const QString &path) -> std::optional<QString> {
  // check file info and access rights
  QFileInfo info(path);
  if (!info.exists() || !info.isFile() || !info.isReadable()) {
    GFModuleLogError(fmt::format("get info for file {} error, exists: {}",
                                 info.filePath(), info.exists())
                         .c_str());
    return {};
  }

  // open and read file
  QFile f(info.filePath());
  if (!f.open(QIODevice::ReadOnly)) {
    GFModuleLogError(fmt::format("open {} to calculate checksum error: {}",
                                 path.toStdString(),
                                 f.errorString().toStdString())
                         .c_str());
    return {};
  }

  QCryptographicHash hash_sha(QCryptographicHash::Sha256);

  // read data by chunks
  const qint64 buffer_size = 8192;  // Define a suitable buffer size
  while (!f.atEnd()) {
    QByteArray const buffer = f.read(buffer_size);
    if (buffer.isEmpty()) {
      GFModuleLogError(fmt::format("error reading file {} during "
                                   "checksum calculation",
                                   path.toStdString())
                           .c_str());
      return {};
    }
    hash_sha.addData(buffer);
  }

  // close the file
  f.close();

  // return the first 6 characters of the SHA-256 hash
  // of the file
  return QString(hash_sha.result().toHex()).left(6);
}

void GetGpgComponentInfos(void *data, int exit_code, const char *out,
                          const char *err) {
  auto *context = reinterpret_cast<Context *>(data);
  auto p_out = QString::fromUtf8(out);
  auto p_err = QString::fromUtf8(err);

  GFModuleLogDebug(fmt::format("gpgconf components exit_code: {} "
                               "process stdout size: {}",
                               exit_code, p_out.size())
                       .c_str());

  if (exit_code != 0) {
    GFModuleLogError(fmt::format("gpgconf execute error, process "
                                 "stderr: {}, "
                                 "process stdout: {}",
                                 p_err, p_out)
                         .c_str());
    return;
  }

  std::vector<GpgComponentInfo> component_infos;
  GpgComponentInfo c_i_gpgme;
  c_i_gpgme.name = "gpgme";
  c_i_gpgme.desc = "GPG Made Easy";
  c_i_gpgme.version = context->gpgme_version;
  c_i_gpgme.path = "Embedded In";
  c_i_gpgme.binary_checksum = "/";

  GpgComponentInfo c_i_gpgconf;
  c_i_gpgconf.name = "gpgconf";
  c_i_gpgconf.desc = "GPG Configure";
  c_i_gpgconf.version = "/";
  c_i_gpgconf.path = context->gpgconf_path;
  auto gpgconf_binary_checksum = CalculateBinaryChacksum(context->gpgconf_path);
  c_i_gpgconf.binary_checksum =
      (gpgconf_binary_checksum.has_value() ? gpgconf_binary_checksum.value()
                                           : QString("/"));

  component_infos.push_back(c_i_gpgme);
  component_infos.push_back(c_i_gpgconf);

  auto const jsonlized_gpgme_component_info = c_i_gpgme.Json();
  auto const jsonlized_gpgconf_component_info = c_i_gpgconf.Json();
  GFModuleUpsertRTValue(
      GFGetModuleID(), GFModuleStrDup("gnupg.components.gpgme"),
      GFModuleStrDup(QJsonDocument(jsonlized_gpgme_component_info).toJson()));
  GFModuleUpsertRTValue(
      GFGetModuleID(), GFModuleStrDup("gnupg.components.gpgconf"),
      GFModuleStrDup(QJsonDocument(jsonlized_gpgconf_component_info).toJson()));

  auto line_split_list = p_out.split("\n");

  for (const auto &line : line_split_list) {
    auto info_split_list = line.split(":");

    if (info_split_list.size() != 3) continue;

    auto component_name = info_split_list[0].trimmed();
    auto component_desc = info_split_list[1].trimmed();
    auto component_path = info_split_list[2].trimmed();

#ifdef WINDOWS
    // replace some special substrings on windows
    // platform
    component_path.replace("%3a", ":");
#endif

    auto binary_checksum = CalculateBinaryChacksum(component_path);

    GFModuleLogDebug(
        fmt::format("gnupg component name: {} desc: "
                    "{} checksum: {} path: {} ",
                    component_name, component_desc,
                    binary_checksum.has_value() ? binary_checksum.value() : "/",
                    component_path)
            .c_str());

    QString version = "/";

    if (component_name == "gpg") {
      version = GFModuleRetrieveRTValueOrDefault(
          GFModuleStrDup("core"), GFModuleStrDup("gpgme.ctx.gnupg_version"),
          GFModuleStrDup("2.0.0"));
    }
    if (component_name == "gpg-agent") {
      GFModuleUpsertRTValue(GFGetModuleID(),
                            GFModuleStrDup("gnupg.gpg_agent_path"),
                            GFModuleStrDup(QString(component_path).toUtf8()));
    }
    if (component_name == "dirmngr") {
      GFModuleUpsertRTValue(GFGetModuleID(),
                            GFModuleStrDup("gnupg.dirmngr_path"),
                            GFModuleStrDup(QString(component_path).toUtf8()));
    }
    if (component_name == "keyboxd") {
      GFModuleUpsertRTValue(GFGetModuleID(),
                            GFModuleStrDup("gnupg.keyboxd_path"),
                            GFModuleStrDup(QString(component_path).toUtf8()));
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
      GFModuleUpsertRTValue(
          GFGetModuleID(),
          GFModuleStrDup(
              QString("gnupg.components.%1").arg(component_name).toUtf8()),
          GFModuleStrDup(QJsonDocument(jsonlized_component_info).toJson()));

      component_infos.push_back(c_i);
    }

    GFModuleLogDebug("load gnupg component info actually done.");
  }
}

void GetGpgDirectoryInfos(void *, int exit_code, const char *out,
                          const char *err) {
  if (exit_code != 0) return;

  auto p_out = QString::fromUtf8(out);
  auto p_err = QString::fromUtf8(err);
  auto line_split_list = p_out.split("\n");

  for (const auto &line : line_split_list) {
    auto info_split_list = line.split(":");
    GFModuleLogDebug(fmt::format("gpgconf direcrotries info line: "
                                 "{} info size: {}",
                                 line, info_split_list.size())
                         .c_str());

    if (info_split_list.size() != 2) continue;

    auto configuration_name = info_split_list[0].trimmed();
    auto configuration_value = info_split_list[1].trimmed();

#ifdef WINDOWS
    // replace some special substrings on windows
    // platform
    configuration_value.replace("%3a", ":");
#endif

    // record gnupg home path
    if (configuration_name == "homedir") {
      GFModuleUpsertRTValue(GFGetModuleID(), GFModuleStrDup("gnupg.home_path"),
                            GFModuleStrDup(configuration_value.toUtf8()));
    }

    GFModuleUpsertRTValue(
        GFGetModuleID(),
        GFModuleStrDup(
            QString("gnupg.dirs.%1").arg(configuration_name).toUtf8()),
        GFModuleStrDup(configuration_value.toUtf8()));
  }
}

void GetGpgOptionInfos(void *data, int exit_code, const char *out,
                       const char *err) {
  if (exit_code != 0) return;

  auto p_out = QString::fromUtf8(out);
  auto p_err = QString::fromUtf8(err);
  auto *context = reinterpret_cast<Context *>(data);
  auto component_name = context->component_info.name;

  GFModuleLogDebug(fmt::format("gpgconf {} avaliable options "
                               "exit_code: {} process stdout "
                               "size: {} ",
                               component_name, exit_code, p_out.size())
                       .c_str());

  std::vector<GpgOptionsInfo> options_infos;

  auto line_split_list = p_out.split("\n");

  for (const auto &line : line_split_list) {
    auto info_split_list = line.split(":");

    GFModuleLogDebug(fmt::format("component {} avaliable options "
                                 "line: {} info size: {}",
                                 component_name, line, info_split_list.size())
                         .c_str());

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
    GFModuleUpsertRTValue(
        GFGetModuleID(),
        GFModuleStrDup(QString("gnupg.components.%1.options.%2")
                           .arg(component_name)
                           .arg(option_name)
                           .toUtf8()),
        GFModuleStrDup(QJsonDocument(jsonlized_option_info).toJson()));
    options_infos.push_back(info);
  }

  context->~Context();
  GFFreeMemory(context);
}
