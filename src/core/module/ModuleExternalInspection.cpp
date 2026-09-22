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

#include "core/module/ModuleExternalInspection.h"

#include <QDir>
#include <QFileInfo>
#include <QJsonObject>

#include "core/module/ModuleNamespace.h"
#include "core/module/ModulePublisherKey.h"

namespace GpgFrontend::Module {

auto InspectExternalModule(const QString& path) -> ExternalModuleReport {
  ExternalModuleReport report;

  const QFileInfo info(path);
  report.descriptor_path =
      info.isDir() ? QDir::cleanPath(info.absoluteFilePath()) + "/" +
                         QString::fromLatin1(kModuleDescriptorFileName)
                   : info.absoluteFilePath();

  const auto verdict = VerifyExternalModuleDescriptor(report.descriptor_path);
  report.descriptor_status = verdict.status;
  report.descriptor_reason = verdict.reason;
  report.signature_valid = verdict.authenticated;
  if (!report.signature_valid) return report;

  // Everything below is about a package whose signer is known. None of it is
  // read from the package's claims about itself except what was signed.
  report.manifest = verdict.manifest;
  report.publisher_key = verdict.signer_public_key;
  report.fingerprint = ModulePublisherKeyFingerprint(report.publisher_key);

  report.publisher_trusted = IsModulePublisherKeyTrusted(report.publisher_key);
  report.module_enabled =
      IsExternalModuleEnabled(report.manifest.id, report.publisher_key);
  report.authorization =
      ExternalModuleAuthorization(report.manifest.id, report.publisher_key);

  // Admission is the only stage left once authentication passed, and it is
  // exactly platform, ABI and minimum host version for an external module.
  report.compatible = verdict.ok;

  const QFileInfo descriptor(report.descriptor_path);
  report.namespace_valid = descriptor.absoluteDir().dirName() ==
                           ModuleDirectoryKey(report.manifest.id);

  const auto entry = ResolveAndVerifyNativeEntry(
      report.manifest,
      ModuleNativeRoot{ModuleNativeRootFor(report.descriptor_path)},
      {ModuleOrigin::kEXTERNAL, ModuleBindingRequirement::kREQUIRED});
  report.native_binding_valid = entry.ok;
  report.native_status = entry.status;
  report.native_reason = entry.reason;

  return report;
}

auto ExternalModuleReportToJson(const ExternalModuleReport& report)
    -> QJsonObject {
  QJsonObject json{
      {"signature_valid", report.signature_valid},
      {"descriptor_status", QString::fromUtf8(ModuleDescriptorStatusToString(
                                report.descriptor_status))},
      {"publisher_trusted", report.publisher_trusted},
      {"module_enabled", report.module_enabled},
      {"compatible", report.compatible},
      {"namespace_valid", report.namespace_valid},
      {"native_binding_valid", report.native_binding_valid},
      {"loadable", report.Loadable()},
  };
  if (!report.descriptor_reason.isEmpty()) {
    json.insert("descriptor_reason", report.descriptor_reason);
  }
  if (report.signature_valid) {
    json.insert("publisher_fingerprint", report.fingerprint);
    json.insert("id", report.manifest.id);
    json.insert("version", report.manifest.version);
    json.insert(
        "native_status",
        QString::fromUtf8(ModuleEntryStatusToString(report.native_status)));
    if (!report.native_reason.isEmpty()) {
      json.insert("native_reason", report.native_reason);
    }
  }
  return json;
}

}  // namespace GpgFrontend::Module
