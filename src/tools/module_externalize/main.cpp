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

#include <sodium.h>

#include <QCoreApplication>
#include <QTextStream>

#include "core/module/ModuleExternalize.h"
#include "core/module/ModulePublisherKey.h"

/**
 * @file main.cpp
 * @brief Turn a verified integrated module into a publisher-signed external
 * one. A release tool, run by a person, never by the build.
 *
 * Three tools, three trust roles:
 *
 * ```
 * gf_module_keygen      = integrated build identity (ephemeral, per build)
 * gf_module_packager    = integrated construction and verification
 * gf_module_externalize = verified integrated -> publisher-attested external
 * ```
 *
 * All of the work lives in ModuleExternalize and ModulePublisherKey, which
 * the tests drive; this file is argument parsing. Like the packager it links
 * gf_core only for the descriptor, binding and archive code and the compiled
 * integrated trust root, and touches no runtime state: no settings, no
 * profile, no trust store, no module runtime. It is a deterministic artifact
 * transformation, and must stay one.
 *
 * The publisher key is only ever read from a file. A key on an argv is a key
 * in every process listing.
 */

namespace {

void PrintUsage(QTextStream& err) {
  err << "usage: gf_module_externalize --input NAMESPACE_DIR|module.gfmodule\n"
      << "                             --publisher-key FILE\n"
      << "                             --output-root DIR\n"
      << "                             [--publisher-name NAME]\n"
      << "                             [--publisher-url URL]\n"
      << "       gf_module_externalize new-key --out FILE\n"
      << "       gf_module_externalize fingerprint --key FILE\n"
      << "\n"
      << "The input must be an integrated module this build verifies, with "
         "its\n"
      << "entry native bound. Externalization is the last release step: do "
         "not\n"
      << "rewrite the output's native afterwards.\n";
}

void PrintKey(QTextStream& out, const QByteArray& public_key) {
  out << "publisher key  "
      << GpgFrontend::Module::ModulePublisherKeyText(public_key) << "\n"
      << "fingerprint    "
      << GpgFrontend::Module::ModulePublisherKeyFingerprint(public_key) << "\n";
}

/// `new-key`: a fresh publisher identity, as FILE (secret) and FILE.pub.
auto NewKeyCommand(const QStringList& args, QTextStream& err) -> int {
  QString path;
  for (auto i = 0; i < args.size(); ++i) {
    if (args.at(i) == "--out" && i + 1 < args.size()) {
      path = args.at(++i);
    } else {
      err << "gf_module_externalize: unknown argument: " << args.at(i) << "\n";
      return 2;
    }
  }
  if (path.isEmpty()) {
    err << "usage: gf_module_externalize new-key --out FILE\n";
    return 2;
  }

  QByteArray public_key;
  QString reason;
  if (!GpgFrontend::Module::GenerateModulePublisherKeyFiles(
          path, path + ".pub", public_key, reason)) {
    err << "gf_module_externalize: " << reason << "\n";
    return 1;
  }

  QTextStream out(stdout);
  out << "secret key     " << path << "  (keep it; it is your identity)\n"
      << "public key     " << path << ".pub  (publish it)\n";
  PrintKey(out, public_key);
  return 0;
}

/// `fingerprint`: the identity a key file names, secret or public.
auto FingerprintCommand(const QStringList& args, QTextStream& err) -> int {
  QString path;
  for (auto i = 0; i < args.size(); ++i) {
    if (args.at(i) == "--key" && i + 1 < args.size()) {
      path = args.at(++i);
    } else {
      err << "gf_module_externalize: unknown argument: " << args.at(i) << "\n";
      return 2;
    }
  }
  if (path.isEmpty()) {
    err << "usage: gf_module_externalize fingerprint --key FILE\n";
    return 2;
  }

  QByteArray public_key;
  QString reason;
  if (!GpgFrontend::Module::ReadModulePublisherPublicKey(path, public_key,
                                                         reason)) {
    err << "gf_module_externalize: " << reason << "\n";
    return 1;
  }
  QTextStream out(stdout);
  PrintKey(out, public_key);
  return 0;
}

}  // namespace

auto main(int argc, char** argv) -> int {
  QCoreApplication app(argc, argv);
  QTextStream err(stderr);

  const auto args = QCoreApplication::arguments();
  if (args.size() > 1 && args.at(1) == "new-key") {
    return NewKeyCommand(args.mid(2), err);
  }
  if (args.size() > 1 && args.at(1) == "fingerprint") {
    return FingerprintCommand(args.mid(2), err);
  }

  GpgFrontend::Module::ModuleExternalizeSpec spec;
  QString key_path;

  for (auto i = 1; i < args.size(); ++i) {
    const auto& flag = args.at(i);
    const auto value = [&]() -> QString {
      if (i + 1 >= args.size()) return {};
      return args.at(++i);
    };

    if (flag == "--input") {
      spec.input = value();
    } else if (flag == "--publisher-key") {
      key_path = value();
    } else if (flag == "--output-root") {
      spec.output_root = value();
    } else if (flag == "--publisher-name") {
      spec.publisher_name = value();
    } else if (flag == "--publisher-url") {
      spec.publisher_url = value();
    } else if (flag == "-h" || flag == "--help") {
      PrintUsage(err);
      return 0;
    } else {
      err << "gf_module_externalize: unknown argument: " << flag << "\n";
      PrintUsage(err);
      return 2;
    }
  }

  if (spec.input.isEmpty() || key_path.isEmpty() ||
      spec.output_root.isEmpty()) {
    PrintUsage(err);
    return 2;
  }

  QString reason;
  if (!GpgFrontend::Module::ReadModulePublisherSecretKey(
          key_path, spec.publisher_seed, reason)) {
    err << "gf_module_externalize: " << reason << "\n";
    return 1;
  }
  if (GpgFrontend::Module::IsModulePublisherSecretKeyExposed(key_path)) {
    err << "gf_module_externalize: warning: " << key_path
        << " can be read by other users; it should be owner-only\n";
  }

  const auto result = GpgFrontend::Module::ExternalizeModule(spec);

  spec.publisher_seed.detach();
  sodium_memzero(spec.publisher_seed.data(),
                 static_cast<size_t>(spec.publisher_seed.size()));

  if (!result.ok) {
    err << "gf_module_externalize: " << result.reason << "\n";
    return 1;
  }

  QTextStream out(stdout);
  out << "externalized   " << result.manifest.id << " "
      << result.manifest.version << "\n"
      << "namespace      " << result.output_namespace << "\n";
  PrintKey(out, result.publisher_key);
  return 0;
}
