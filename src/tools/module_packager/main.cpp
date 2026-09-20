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
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>

#include "core/module/ModuleDescriptor.h"
#include "core/module/ModuleDescriptorBuilder.h"
#include "core/module/ModuleEntryBinding.h"
#include "core/module/ModuleManifest.h"
#include "core/module/ModuleNamespace.h"
#include "core/module/ModulePreparedEntry.h"
#include "core/module/ModuleSetVerification.h"
#include "core/module/ModuleTrustRoot.h"

/**
 * @file main.cpp
 * @brief Build one `*.gfmodule` package. A host tool, invoked by CMake.
 *
 * Crypto and canonical JSON are not things to write in CMake language, so the
 * build system's whole involvement is calling this with arguments. Everything
 * it does lives in ModuleDescriptorBuilder, which is also what the tests drive
 * -- so the packaging path a developer's build takes is the packaging path the
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
      << "                         [--prepared-manifest FILE]\n"
      << "                         [--file ARCHIVE_PATH=SOURCE_FILE]...\n"
      << "\n"
      << "subcommands: verify-module-set, seal-prepared, reseal, "
         "binding-id, host-info\n";
}

/// Split `KEY=VALUE` at the FIRST `=`, so a value may contain one.
auto SplitPair(const QString& text, QString& key, QString& value) -> bool {
  const auto at = text.indexOf('=');
  if (at <= 0 || at == text.size() - 1) return false;
  key = text.left(at);
  value = text.mid(at + 1);
  return true;
}

/// Parse `--expect-count`'s value, refusing anything that is not a
/// non-negative decimal integer.
///
/// `QString::toInt()` answers 0 for an empty or malformed string, and 0 is a
/// meaningful count here -- so a caller that forgot to substitute a variable
/// would not be refused, it would be silently held to "expect nothing", and
/// the gate would report a count mismatch rather than the missing argument
/// that caused it. A count this gate cannot read is a broken invocation, not
/// an expectation of zero.
auto ParseExpectCount(const QString& text, int& out, QTextStream& err) -> bool {
  auto ok = false;
  const auto parsed = text.toInt(&ok);
  if (!ok || parsed < 0) {
    err << "gf_module_packager: --expect-count needs a non-negative integer, "
        << "got \"" << text << "\"\n";
    return false;
  }
  out = parsed;
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
  auto print_entries = false;
  auto print_bindings = false;

  for (auto i = 0; i < args.size(); ++i) {
    const auto& flag = args.at(i);
    const auto value = [&]() -> QString {
      if (i + 1 >= args.size()) return {};
      return args.at(++i);
    };

    if (flag == "--namespace-root") {
      root = value();
    } else if (flag == "--expect-count") {
      if (!ParseExpectCount(value(), expected, err)) return 2;
    } else if (flag == "--print-entries") {
      // For the deployment audit, which has to tell an entry from a private
      // helper and must not guess it from a filename.
      // No value to consume: the loop's own ++i is the whole advance. An
      // extra decrement here would cancel it out and spin forever.
      print_entries = true;
    } else if (flag == "--print-bindings") {
      // For the build record: what each module's entry is bound by, and to
      // what. Separate from --print-entries because they answer different
      // questions and one line trying to answer both is a line every consumer
      // has to re-parse when either changes.
      print_bindings = true;
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
        << "                         [--assert-no-native-outside TREE]\n"
        << "                         [--print-entries] [--print-bindings]\n";
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

  if (print_entries) {
    // Machine-readable, on stdout, after everything else: `<key> <path>` per
    // line. The audit script reads it rather than inferring the entry from a
    // name.
    for (auto it = result.entries.constBegin(); it != result.entries.constEnd();
         ++it) {
      out << "entry " << it.key() << " " << it.value() << "\n";
    }
  }

  if (print_bindings) {
    for (auto it = result.bindings.constBegin();
         it != result.bindings.constEnd(); ++it) {
      out << "binding " << it.key() << " " << it.value() << "\n";
    }
  }

  out << "  " << result.verified.size() << " module(s) verified\n";
  return 0;
}

/// `seal-prepared`: record what each entry native binds to, after preparation.
///
/// Runs between the platform deployment step and the descriptor finalize step.
/// The descriptors sitting in the tree at this point were written by the build
/// against the freshly linked natives; `patchelf`, `linuxdeployqt` and
/// `install_name_tool` have since rewritten those natives, so those
/// descriptors are stale by construction and their recorded values are
/// deliberately NOT what is written here. What is written is what the file
/// binds to now.
///
/// Finalize then regenerates every descriptor with `--prepared-manifest`, and
/// the build refuses if anything moved in between. See ModulePreparedEntry.h
/// for why this is content-based rather than build-graph-based.
auto SealPreparedCommand(const QStringList& args, QTextStream& err) -> int {
  QString root;
  auto expected = -1;

  for (auto i = 0; i < args.size(); ++i) {
    const auto& flag = args.at(i);
    const auto value = [&]() -> QString {
      if (i + 1 >= args.size()) return {};
      return args.at(++i);
    };

    if (flag == "--namespace-root") {
      root = value();
    } else if (flag == "--expect-count") {
      if (!ParseExpectCount(value(), expected, err)) return 2;
    } else {
      err << "gf_module_packager: unknown argument: " << flag << "\n";
      return 2;
    }
  }

  if (root.isEmpty()) {
    err << "usage: gf_module_packager seal-prepared --namespace-root DIR\n"
        << "                         [--expect-count N]\n";
    return 2;
  }

  const QDir dir(root);
  if (!dir.exists()) {
    err << "gf_module_packager: " << root << ": this directory does not "
        << "exist\n";
    return 1;
  }

  QTextStream out(stdout);
  out << "sealing " << root << "\n";

  auto sealed = 0;
  auto failed = false;

  for (const auto& ns :
       dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name)) {
    const auto descriptor = ns.absoluteFilePath() + "/" +
                            GpgFrontend::Module::kModuleDescriptorFileName;
    if (!QFileInfo(descriptor).isFile()) continue;

    // Verified, not merely parsed: the id and the entry name about to be
    // sealed have to come from bytes this build signed, or the seal records
    // whatever an unsigned file claimed.
    const auto verdict =
        GpgFrontend::Module::VerifyModuleDescriptor(descriptor);
    if (!verdict.ok) {
      err << "  FAIL  " << ns.fileName()
          << ": its descriptor was refused: " << verdict.reason << "\n";
      failed = true;
      continue;
    }

    const auto& manifest = verdict.manifest;
    const auto native_root =
        GpgFrontend::Module::ModuleNativeRootFor(descriptor);
    const auto native_path =
        native_root + "/" +
        GpgFrontend::Module::ModuleNativeFileName(manifest.entry_native.name);

    if (!QFileInfo(native_path).isFile()) {
      err << "  FAIL  " << ns.fileName()
          << ": its entry native is not there: " << native_path << "\n";
      failed = true;
      continue;
    }

    GpgFrontend::Module::PreparedEntrySeal seal;
    seal.module_id = manifest.id;
    seal.build_id = manifest.build_id;
    seal.entry_native_name = manifest.entry_native.name;
    seal.mode = manifest.entry_native.mode;

    QString why;
    const GpgFrontend::Module::ModuleEntryBindingContext context{
        manifest.id, manifest.build_id, manifest.sdk_abi};
    if (!GpgFrontend::Module::ComputeEntryVerificationValue(
            seal.mode, native_path, context, seal.value, why)) {
      err << "  FAIL  " << ns.fileName() << ": " << why << "\n";
      failed = true;
      continue;
    }

    if (seal.mode ==
        GpgFrontend::Module::ModuleEntryVerificationMode::kFILE_SHA256) {
      seal.size = QFileInfo(native_path).size();
    }

    const auto seal_path =
        native_root + "/" +
        QString::fromUtf8(GpgFrontend::Module::kPreparedEntrySealFileName);
    if (!GpgFrontend::Module::WritePreparedEntrySeal(seal_path, seal, why)) {
      err << "  FAIL  " << ns.fileName()
          << ": its seal could not be written: " << why << "\n";
      failed = true;
      continue;
    }

    // Worth printing even when it matches: a reader comparing this against the
    // finalize step's output is exactly the audit this mechanism supports.
    out << "  seal  " << manifest.id << " "
        << GpgFrontend::Module::ModuleEntryVerificationModeKey(seal.mode) << " "
        << seal.value << "\n";
    ++sealed;
  }

  if (expected >= 0 && sealed != expected) {
    err << "  FAIL  " << root << ": " << sealed << " sealed, and " << expected
        << " were expected\n";
    failed = true;
  }

  if (failed) {
    err << "gf_module_packager: nothing downstream of this should run\n";
    return 1;
  }

  out << "  " << sealed << " module(s) sealed\n";
  return 0;
}

/// `reseal`: regenerate every descriptor in a tree against the natives as they
/// stand now.
///
/// ## Why this exists instead of re-running the build
///
/// The obvious way to refresh a descriptor is to delete it and let CMake make
/// another. That does not work on a deployed tree, and the reason is worth
/// writing down because it is invisible until it bites: the module natives ARE
/// build outputs, and `linuxdeployqt`, `patchelf` and `install_name_tool` have
/// just rewritten them. Ninja records the mtime of every output it produces, so
/// the next `cmake --build` sees they changed underneath it and RELINKS them --
/// throwing away the rpath work the deployment step just did, and then failing
/// the seal comparison for good measure.
///
/// So the descriptors are regenerated without the build graph. Each one is
/// verified first, so its metadata is this build's own signed statement rather
/// than whatever a file on disk claimed; only the entry binding is recomputed;
/// resources are carried over from the original; and the result is re-signed
/// with the same build key.
///
/// ## What it does not do
///
/// It does not make a rewritten native trustworthy -- it describes what is
/// there. The guarantee that matters comes afterwards, from `verify-module-set`
/// over the finished tree, and from nothing touching the natives in between.
auto ResealCommand(const QStringList& args, QTextStream& err) -> int {
  QString root;
  QString seed_path;
  auto expected = -1;

  for (auto i = 0; i < args.size(); ++i) {
    const auto& flag = args.at(i);
    const auto value = [&]() -> QString {
      if (i + 1 >= args.size()) return {};
      return args.at(++i);
    };

    if (flag == "--namespace-root") {
      root = value();
    } else if (flag == "--signing-seed") {
      seed_path = value();
    } else if (flag == "--expect-count") {
      if (!ParseExpectCount(value(), expected, err)) return 2;
    } else {
      err << "gf_module_packager: unknown argument: " << flag << "\n";
      return 2;
    }
  }

  if (root.isEmpty() || seed_path.isEmpty()) {
    err << "usage: gf_module_packager reseal --namespace-root DIR\n"
        << "                         --signing-seed FILE [--expect-count N]\n";
    return 2;
  }

  QFile seed_file(seed_path);
  if (!seed_file.open(QIODevice::ReadOnly)) {
    err << "gf_module_packager: the signing seed could not be read: "
        << seed_path << "\n";
    return 2;
  }
  const auto seed = seed_file.readAll();
  seed_file.close();

  const QDir dir(root);
  if (!dir.exists()) {
    err << "gf_module_packager: " << root
        << ": this directory does not exist\n";
    return 1;
  }

  QTextStream out(stdout);
  out << "resealing " << root << "\n";

  auto resealed = 0;
  auto failed = false;

  for (const auto& ns :
       dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name)) {
    const auto descriptor = ns.absoluteFilePath() + "/" +
                            GpgFrontend::Module::kModuleDescriptorFileName;
    if (!QFileInfo(descriptor).isFile()) continue;

    // Verified, not merely parsed. Everything carried into the new descriptor
    // comes from bytes this build signed; the only thing taken from the
    // filesystem is the entry binding, which is the one thing that changed.
    const auto verdict =
        GpgFrontend::Module::VerifyModuleDescriptor(descriptor);
    if (!verdict.ok) {
      err << "  FAIL  " << ns.fileName()
          << ": its descriptor was refused: " << verdict.reason << "\n";
      failed = true;
      continue;
    }

    const auto& manifest = verdict.manifest;

    if (ns.fileName() != GpgFrontend::Module::ModuleDirectoryKey(manifest.id)) {
      err << "  FAIL  " << ns.fileName() << ": it declares " << manifest.id
          << ", whose namespace is "
          << GpgFrontend::Module::ModuleDirectoryKey(manifest.id) << "\n";
      failed = true;
      continue;
    }

    const GpgFrontend::Module::ModuleNativeRoot native_root{
        GpgFrontend::Module::ModuleNativeRootFor(descriptor)};
    const auto resolved =
        GpgFrontend::Module::ResolveNativeEntry(manifest, native_root);
    if (!resolved.ok) {
      err << "  FAIL  " << ns.fileName()
          << ": its entry native was refused: " << resolved.reason << "\n";
      failed = true;
      continue;
    }
    const auto native_path = resolved.path;

    QMap<QString, QByteArray> resources;
    QString why;
    if (!GpgFrontend::Module::ReadModuleDescriptorResources(descriptor,
                                                            resources, why)) {
      err << "  FAIL  " << ns.fileName()
          << ": its resources could not be read: " << why << "\n";
      failed = true;
      continue;
    }

    GpgFrontend::Module::ModuleDescriptorBuildSpec spec;
    spec.module_id = manifest.id;
    spec.version = manifest.version;
    spec.sdk_abi = manifest.sdk_abi;
    spec.min_host_version = manifest.min_host_version;
    spec.security_epoch = manifest.security_epoch;
    spec.capabilities = manifest.capabilities;
    spec.events = manifest.events;
    spec.translation_context = manifest.translation_context;
    spec.metadata = manifest.metadata;
    spec.build_id = manifest.build_id;
    spec.build_timestamp = manifest.build_timestamp;
    spec.build_source_commit = manifest.build_source_commit;
    spec.platform_os = manifest.platform_os;
    spec.platform_arch = manifest.platform_arch;
    spec.platform_qt = manifest.platform_qt;
    spec.entry_native_name = manifest.entry_native.name;
    spec.entry_native_file = native_path;
    spec.signing_seed = seed;
    spec.output_path = descriptor;

    for (auto it = resources.constBegin(); it != resources.constEnd(); ++it) {
      spec.resources.append({it.key(), {}, it.value()});
    }

    const auto built = GpgFrontend::Module::BuildModuleDescriptor(spec);
    if (!built.ok) {
      err << "  FAIL  " << ns.fileName() << ": " << built.reason << "\n";
      failed = true;
      continue;
    }

    out << "  reseal " << manifest.id << " " << resources.size()
        << " resource(s)\n";
    ++resealed;
  }

  if (expected >= 0 && resealed != expected) {
    err << "  FAIL  " << root << ": " << resealed << " resealed, and "
        << expected << " were expected\n";
    failed = true;
  }

  if (failed) {
    err << "gf_module_packager: this tree was not fully resealed\n";
    return 1;
  }

  out << "  " << resealed << " descriptor(s) resealed\n";
  return 0;
}

/// `host-info`: the canonical os and architecture names this build uses.
///
/// So a shell script does not have to reimplement NormalizeManifestArch().
/// `uname -m` says `aarch64` where Qt says `arm64`, and a build record that
/// spelled the architecture differently from the descriptors beside it would
/// be describing a machine nothing else recognises. One implementation,
/// printed on request.
auto HostInfoCommand(const QStringList& args, QTextStream& err) -> int {
  if (!args.isEmpty()) {
    err << "usage: gf_module_packager host-info\n";
    return 2;
  }
  QTextStream out(stdout);
  out << "os " << GpgFrontend::Module::ManifestHostOsName() << "\n";
  out << "arch " << GpgFrontend::Module::ManifestHostArchName() << "\n";
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

  GpgFrontend::Module::ModuleDescriptorBuildSpec spec;
  GpgFrontend::Module::PreparedEntrySeal prepared;
  auto have_prepared = false;
  auto args = QCoreApplication::arguments();

  // Subcommands are dispatched first; the packaging flags stay the default so
  // every existing caller is unchanged.
  if (args.size() > 1 && args.at(1) == "verify-module-set") {
    return VerifyModuleSetCommand(args.mid(2), err);
  }
  if (args.size() > 1 && args.at(1) == "seal-prepared") {
    return SealPreparedCommand(args.mid(2), err);
  }
  if (args.size() > 1 && args.at(1) == "reseal") {
    return ResealCommand(args.mid(2), err);
  }
  if (args.size() > 1 && args.at(1) == "host-info") {
    return HostInfoCommand(args.mid(2), err);
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
    } else if (flag == "--prepared-manifest") {
      // Read here and reduced to one expected value, so the builder's check is
      // a string comparison it can be tested on rather than a file format it
      // has to know.
      const auto path = value();
      GpgFrontend::Module::PreparedEntrySeal seal;
      QString why;
      if (!GpgFrontend::Module::ReadPreparedEntrySeal(path, seal, why)) {
        err << "gf_module_packager: " << path << ": " << why << "\n";
        return 2;
      }
      prepared = seal;
      have_prepared = true;
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

  if (have_prepared) {
    // The seal is for one module and one entry. Pointing the wrong one at it
    // would otherwise pass whenever the two natives happened to bind alike,
    // which for `apple-binding-id` is not even unlikely -- it is derived from
    // identity, so two descriptors of one module in one build share it.
    if (prepared.module_id != spec.module_id) {
      err << "gf_module_packager: the prepared manifest is for \""
          << prepared.module_id << "\", not \"" << spec.module_id << "\"\n";
      return 2;
    }
    if (prepared.entry_native_name != spec.entry_native_name) {
      err << "gf_module_packager: the prepared manifest binds \""
          << prepared.entry_native_name << "\", not \""
          << spec.entry_native_name << "\"\n";
      return 2;
    }
    if (prepared.build_id != spec.build_id) {
      err << "gf_module_packager: the prepared manifest is from build \""
          << prepared.build_id << "\", not \"" << spec.build_id << "\"\n";
      return 2;
    }
    spec.expected_entry_value = prepared.value;
  }

  const auto result = GpgFrontend::Module::BuildModuleDescriptor(spec);
  if (!result.ok) {
    err << "gf_module_packager: " << result.reason << "\n";
    return 1;
  }

  QTextStream(stdout) << QFileInfo(spec.output_path).fileName() << ": bound "
                      << spec.entry_native_name << ", signed "
                      << spec.resources.size() << " resource(s)\n";
  return 0;
}
