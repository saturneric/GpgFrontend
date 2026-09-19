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

#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>

#include "core/module/ModuleEntryBinding.h"
#include "core/module/ModulePackageBuilder.h"
#include "core/module/ModuleSetVerification.h"
#include "core/module/ModuleTrustRoot.h"

/**
 * @file main.cpp
 * @brief Build one `*.gfmodule` package. A host tool, invoked by CMake.
 *
 * Crypto and canonical JSON are not things to write in CMake language, so the
 * build system's whole involvement is calling this with arguments. Everything
 * it does lives in ModulePackageBuilder, which is also what the tests drive --
 * so the packaging path a developer's build takes is the packaging path the
 * tests cover, rather than a second implementation that agrees with it today.
 *
 * It deliberately touches no core runtime state: no singletons, no task
 * runners, no settings, no profile or key database. Treat "would this still
 * work if gf_core were a thin archive-plus-hash-plus-sodium library?" as the
 * design test -- it is what keeps a cross-compiled build from needing a
 * natively-built copy of the whole application.
 */

namespace {

void PrintUsage(QTextStream& err) {
  err << "usage: gf_module_packager --output FILE --id ID --version V\n"
      << "                         --sdk-abi N --min-host-version V\n"
      << "                         --os OS --arch ARCH --qt V\n"
      << "                         [--build-id S] [--timestamp S] "
         "[--commit S]\n"
      << "                         [--security-epoch N]\n"
      << "                         [--capability NAME]...\n"
      << "                         [--event EVENT_ID]...\n"
      << "                         [--translation-context NAME]\n"
      << "                         [--meta KEY=VALUE]...\n"
      << "                         --entry-native name=NAME,file=PATH\n"
      << "                         [--file ARCHIVE_PATH=SOURCE_FILE]...\n";
}

/// Split `KEY=VALUE` at the FIRST `=`, so a value may contain one.
auto SplitPair(const QString& text, QString& key, QString& value) -> bool {
  const auto at = text.indexOf('=');
  if (at <= 0 || at == text.size() - 1) return false;
  key = text.left(at);
  value = text.mid(at + 1);
  return true;
}

}  // namespace

/// `verify-module-set`: the release gate, run over a finished tree.
///
/// It verifies against THIS binary's own embedded trust root, which is the
/// same one gf_core compiled into the Host beside it. That is why it needs no
/// key argument and cannot be pointed at the wrong key: the tool and the
/// application are built from one trust root, so "the tool says yes" and "the
/// Host will load these" are the same statement.
auto VerifyModuleSetCommand(const QStringList& args, QTextStream& err) -> int {
  QString root;
  auto expected = -1;
  QString outside_root;

  for (auto i = 0; i < args.size(); ++i) {
    const auto& flag = args.at(i);
    const auto value = [&]() -> QString {
      if (i + 1 >= args.size()) return {};
      return args.at(++i);
    };

    if (flag == "--namespace-root") {
      root = value();
    } else if (flag == "--expect-count") {
      expected = value().toInt();
    } else if (flag == "--assert-no-native-outside") {
      // A SHIPPING tree, not a build tree. A build tree legitimately holds
      // module libraries outside any namespace -- test fixtures, intermediate
      // outputs -- and pointing this at one reports them all, correctly and
      // uselessly. It belongs in the staging step, after the tree has been
      // assembled from what is meant to ship.
      outside_root = value();
    } else {
      err << "gf_module_packager: unknown argument: " << flag << "\n";
      return 2;
    }
  }

  if (root.isEmpty()) {
    err << "usage: gf_module_packager verify-module-set --namespace-root DIR\n"
        << "                         [--expect-count N]\n"
        << "                         [--assert-no-native-outside TREE]\n";
    return 2;
  }

  QTextStream out(stdout);
  out << "verifying " << root << "\n"
      << "  build: " << GpgFrontend::Module::ModuleBuildId() << "\n";

  const auto result = GpgFrontend::Module::VerifyModuleSet(root, expected);

  for (auto it = result.verified.constBegin(); it != result.verified.constEnd();
       ++it) {
    out << "  ok    " << it.key() << "\n";
  }
  for (const auto& warning : result.warnings) {
    out << "  warn  " << warning.where << ": " << warning.reason << "\n";
  }
  for (const auto& problem : result.problems) {
    err << "  FAIL  " << problem.where << ": " << problem.reason << "\n";
  }

  auto failed = !result.ok;

  if (!outside_root.isEmpty()) {
    const auto leaked = GpgFrontend::Module::FindNativeModuleBinariesOutside(
        outside_root, root);
    for (const auto& path : leaked) {
      err << "  FAIL  " << path
          << ": a module library outside any module namespace\n";
      failed = true;
    }
  }

  if (failed) {
    err << "gf_module_packager: this tree is not shippable\n";
    return 1;
  }

  out << "  " << result.verified.size() << " module(s) verified\n";
  return 0;
}

/// `binding-id`: write the macOS binding id for a module into a file.
///
/// A subcommand rather than a CMake function, and for a reason worth stating:
/// the derivation hashes NUL-separated fields, and a CMake string cannot
/// contain a NUL. Computing it there would mean hashing something else and
/// calling it the same name -- a second implementation that could never agree
/// with this one. So there is only this one, and CMake calls it.
auto BindingIdCommand(const QStringList& args, QTextStream& err) -> int {
  GpgFrontend::Module::ModuleEntryBindingContext context;
  QString out_path;

  for (auto i = 0; i < args.size(); ++i) {
    const auto& flag = args.at(i);
    const auto value = [&]() -> QString {
      if (i + 1 >= args.size()) return {};
      return args.at(++i);
    };

    if (flag == "--id") {
      context.module_id = value();
    } else if (flag == "--build-id") {
      context.build_id = value();
    } else if (flag == "--sdk-abi") {
      context.sdk_abi = value().toInt();
    } else if (flag == "--output") {
      out_path = value();
    } else {
      err << "gf_module_packager: unknown argument: " << flag << "\n";
      return 2;
    }
  }

  if (context.module_id.isEmpty() || context.build_id.isEmpty() ||
      context.sdk_abi <= 0 || out_path.isEmpty()) {
    err << "usage: gf_module_packager binding-id --id ID --build-id ID "
           "--sdk-abi N --output FILE\n";
    return 2;
  }

  const auto binding = GpgFrontend::Module::ModuleEntryBindingId(context);

  QFile out(out_path);
  if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    err << "gf_module_packager: could not write " << out_path << "\n";
    return 1;
  }
  // Exactly 64 characters and no newline: this becomes the whole content of a
  // Mach-O section, and a trailing byte would make it the wrong length.
  out.write(binding.toLatin1());
  out.close();
  return 0;
}

auto main(int argc, char** argv) -> int {
  QCoreApplication app(argc, argv);
  QTextStream err(stderr);

  GpgFrontend::Module::ModulePackageBuildSpec spec;
  auto args = QCoreApplication::arguments();

  // One subcommand so far, and the packaging flags stay the default so every
  // existing caller is unchanged.
  if (args.size() > 1 && args.at(1) == "verify-module-set") {
    return VerifyModuleSetCommand(args.mid(2), err);
  }
  if (args.size() > 1 && args.at(1) == "binding-id") {
    return BindingIdCommand(args.mid(2), err);
  }

  for (auto i = 1; i < args.size(); ++i) {
    const auto& flag = args.at(i);

    const auto value = [&]() -> QString {
      if (i + 1 >= args.size()) return {};
      return args.at(++i);
    };

    if (flag == "--output") {
      spec.output_path = value();
    } else if (flag == "--id") {
      spec.module_id = value();
    } else if (flag == "--version") {
      spec.version = value();
    } else if (flag == "--sdk-abi") {
      spec.sdk_abi = value().toInt();
    } else if (flag == "--min-host-version") {
      spec.min_host_version = value();
    } else if (flag == "--security-epoch") {
      spec.security_epoch = value().toInt();
    } else if (flag == "--os") {
      spec.platform_os = value();
    } else if (flag == "--arch") {
      spec.platform_arch = value();
    } else if (flag == "--qt") {
      spec.platform_qt = value();
    } else if (flag == "--build-id") {
      spec.build_id = value();
    } else if (flag == "--timestamp") {
      spec.build_timestamp = value();
    } else if (flag == "--commit") {
      spec.build_source_commit = value();
    } else if (flag == "--capability") {
      spec.capabilities.append(value());
    } else if (flag == "--event") {
      spec.events.append(value());
    } else if (flag == "--translation-context") {
      spec.translation_context = value();
    } else if (flag == "--meta") {
      QString key;
      QString text;
      if (!SplitPair(value(), key, text)) {
        err << "gf_module_packager: --meta wants KEY=VALUE\n";
        return 2;
      }
      spec.metadata.insert(key, text);
    } else if (flag == "--signing-seed") {
      // Read here rather than taken as hex on a command line: a key on an
      // argv is a key in every process listing on the machine.
      const auto path = value();
      QFile seed(path);
      if (!seed.open(QIODevice::ReadOnly)) {
        err << "gf_module_packager: the signing seed could not be read: "
            << path << "\n";
        return 2;
      }
      spec.signing_seed = seed.readAll();
      seed.close();
    } else if (flag == "--entry-native") {
      // name=<logical>,file=<path>. The logical name is what the descriptor
      // records; the file is read to compute the binding value and is NOT
      // packaged, because executable code never travels inside a descriptor.
      const auto text = value();
      for (const auto& part : text.split(u',', Qt::SkipEmptyParts)) {
        QString key;
        QString val;
        if (!SplitPair(part, key, val)) {
          err << "gf_module_packager: --entry-native wants "
                 "name=NAME,file=PATH\n";
          return 2;
        }
        if (key == "name") {
          spec.entry_native_name = val;
        } else if (key == "file") {
          spec.entry_native_file = val;
        } else {
          err << "gf_module_packager: unknown --entry-native key: " << key
              << "\n";
          return 2;
        }
      }
    } else if (flag == "--file") {
      QString archive_path;
      QString source_file;
      if (!SplitPair(value(), archive_path, source_file)) {
        err << "gf_module_packager: --file wants ARCHIVE_PATH=SOURCE_FILE\n";
        return 2;
      }
      spec.resources.append({archive_path, source_file, {}});
    } else if (flag == "--help" || flag == "-h") {
      QTextStream(stdout) << "";
      PrintUsage(err);
      return 0;
    } else {
      err << "gf_module_packager: unknown argument: " << flag << "\n";
      PrintUsage(err);
      return 2;
    }
  }

  // Defaults that are only defaults because leaving them empty would produce a
  // manifest the parser refuses for a missing field, which is a confusing way
  // to learn that a build recipe forgot to pass something optional.
  if (spec.build_id.isEmpty()) spec.build_id = "local";
  if (spec.build_timestamp.isEmpty()) {
    spec.build_timestamp =
        QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
  }
  if (spec.build_source_commit.isEmpty()) spec.build_source_commit = "unknown";

  const auto result = GpgFrontend::Module::BuildModulePackage(spec);
  if (!result.ok) {
    err << "gf_module_packager: " << result.reason << "\n";
    return 1;
  }

  QTextStream(stdout) << QFileInfo(spec.output_path).fileName() << ": bound "
                      << spec.entry_native_name << ", signed "
                      << spec.resources.size() << " resource(s)\n";
  return 0;
}
