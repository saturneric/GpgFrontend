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

#include "CommonUtils.h"

#include <sodium.h>

#include "core/GFConstants.h"
#include "core/module/ModuleManager.h"
#include "plat/common/PlatformEnv.h"

namespace GpgFrontend {

auto BeautifyFingerprint(QString fingerprint) -> QString {
  auto len = fingerprint.size();
  QString buffer;
  QTextStream out(&buffer);
  decltype(len) count = 0;
  while (count < len) {
    if ((count != 0U) && ((count % 5) == 0)) out << " ";
    out << fingerprint[count];
    count++;
  }
  return out.readAll();
}

auto GFCompareSoftwareVersion(const QString& a, const QString& b) -> int {
  auto remove_prefix = [](const QString& version) {
    return version.startsWith('v') ? version.mid(1) : version;
  };

  auto real_version_a = remove_prefix(a);
  auto real_version_b = remove_prefix(b);

  QStringList split_a = real_version_a.split('.');
  QStringList split_b = real_version_b.split('.');

  const auto min_depth = std::min(split_a.size(), split_b.size());

  for (int i = 0; i < min_depth; ++i) {
    int const num_a = split_a[i].toInt();
    int const num_b = split_b[i].toInt();

    if (num_a != num_b) {
      return (num_a > num_b) ? 1 : -1;
    }
  }

  if (split_a.size() != split_b.size()) {
    return (split_a.size() > split_b.size()) ? 1 : -1;
  }

  return 0;
}

auto GFSoftwareVersionGreaterThan(const QString& a, const QString& b) -> bool {
  if (a.isEmpty()) return false;
  return GFCompareSoftwareVersion(a, b) > 0;
}

auto GFStrDup(const QString& s) -> char* {
  if (s.isEmpty()) return nullptr;

  auto u_s = s.toUtf8();
  auto* c_s = static_cast<char*>(SMAMalloc((u_s.size() + 1) * sizeof(char)));

  memcpy(c_s, u_s.constData(), u_s.size());
  c_s[u_s.size()] = '\0';
  return c_s;
}

auto GFBufferDup(const GFBuffer& s) -> char* {
  if (s.Empty()) return nullptr;

  const auto& u_s = s;
  auto* c_s = static_cast<char*>(SMASecMalloc((u_s.Size() + 1) * sizeof(char)));

  memcpy(c_s, u_s.Data(), u_s.Size());
  c_s[u_s.Size()] = '\0';
  return c_s;
}

auto GFUnStrDup(char* s) -> QString {
  auto q_s = QString::fromUtf8(s == nullptr ? "" : s);
  if (s != nullptr) SMAFree(static_cast<void*>(s));
  return q_s;
}

auto GFUnStrDup(const char* s) -> QString {
  return GFUnStrDup(const_cast<char*>(s));
}

auto IsFlatpakENV() -> bool {
  return QString::fromLocal8Bit(qgetenv("container")) == "flatpak";
}

auto IsAppImageENV() -> bool {
  return !QString::fromLocal8Bit(qgetenv("APPIMAGE")).isEmpty();
}

auto RelaunchProgram(const QString& argv0) -> QString {
  if (!IsAppImageENV()) return argv0;
  const QFileInfo image(qEnvironmentVariable("APPIMAGE"));
  return image.isFile() && image.isAbsolute() ? image.absoluteFilePath()
                                              : argv0;
}

namespace {

#ifdef Q_OS_LINUX
/**
 * @brief The directory the .AppImage file itself sits in.
 *
 * Empty when this is not an AppImage run, and also when $APPIMAGE does not
 * resolve to an existing file -- the callers then fall back to the ordinary
 * application directory rather than to a path that only looks right.
 */
auto AppImageDirPath() -> QString {
  if (!IsAppImageENV()) return {};

  const auto dir = QFileInfo(qEnvironmentVariable("APPIMAGE")).canonicalPath();

  // canonicalPath() answers "." -- not an empty string -- when $APPIMAGE names
  // a file that is not there. Taken at face value that is a relative data
  // root, so the profile would follow the process's working directory.
  return QDir::isAbsolutePath(dir) ? dir : QString{};
}
#endif

}  // namespace

auto ResolveApplicationDirPath() -> QString {
#ifdef Q_OS_LINUX
  // The application directory of an AppImage points inside its read-only
  // mount, which vanishes when the process exits.
  auto image_dir = AppImageDirPath();
  if (!image_dir.isEmpty()) return image_dir;
#endif
  return QCoreApplication::applicationDirPath();
}

auto ResolvePortableDataPath() -> QString {
#ifdef Q_OS_LINUX
  // An AppImage is a single file, not an executable inside a bin/ of a
  // deployment tree, so there is no bin/ level to climb out of: the directory
  // holding it already is the deployment root. Climbing anyway would put the
  // profile -- the user's keys -- one directory above the AppImage, outside
  // the folder they copied onto the stick.
  auto image_dir = AppImageDirPath();
  if (!image_dir.isEmpty()) return image_dir;
#endif

  const auto app_path = ResolveApplicationDirPath();
  const auto canonical = QDir(app_path + "/../").canonicalPath();
  // canonicalPath() is empty when the parent does not resolve, which would
  // silently turn every derived path into a relative one
  return canonical.isEmpty() ? QDir::cleanPath(app_path + "/..") : canonical;
}

auto ParseHexEncodedVersionTuple(const QString& s) -> int {
  // s is a hex-encoded, unsigned int-packed version tuple,
  // i.e. each byte represents one part of the version tuple
  bool ok;
  const auto version = s.toUtf8().toUInt(&ok, 16);
  return ok ? static_cast<int>(version) : -1;
}

const auto kReEmail = QRegularExpression{
    R"((?:[a-z0-9!#$%&'*+/=?^_`{|}~-]+(?:\.[a-z0-9!#$%&'*+/=?^_`{|}~-]+)*|"(?:[\x01-\x08\x0b\x0c\x0e-\x1f\x21\x23-\x5b\x5d-\x7f]|\\[\x01-\x09\x0b\x0c\x0e-\x7f])*")@(?:(?:[a-z0-9](?:[a-z0-9-]*[a-z0-9])?\.)+[a-z0-9](?:[a-z0-9-]*[a-z0-9])?|\[(?:(?:(2(5[0-5]|[0-4][0-9])|1[0-9][0-9]|[1-9]?[0-9]))\.){3}(?:(2(5[0-5]|[0-4][0-9])|1[0-9][0-9]|[1-9]?[0-9])|[a-z0-9-]*[a-z0-9]:(?:[\x01-\x08\x0b\x0c\x0e-\x1f\x21-\x5a\x53-\x7f]|\\[\x01-\x09\x0b\x0c\x0e-\x7f])+)\]))"};

auto IsEmailAddress(const QString& str) -> bool {
  // Anchor the match: the whole string must be the address. Otherwise a value
  // like "a b@c.com >x" would pass and, once wrapped in '<' '>' by
  // AssembleUserId, produce a malformed user id.
  static const QRegularExpression kReEmailAnchored(
      QRegularExpression::anchoredPattern(kReEmail.pattern()));
  return kReEmailAnchored.match(str).hasMatch();
}

namespace {

/// Reads text a line at a time and decides whether anything lies outside one
/// signed or message armor block; see HasTextOutsideSignedArmor().
class ArmorEnvelopeCheck {
 public:
  void Line(QByteArrayView raw) {
    // Inside cleartext every line starting with '-' is dash-escaped, and
    // base64 has no dashes, so armor lines can only be the envelope itself.
    if (raw.startsWith(kArmorBegin)) ++begins_;
    if (raw.startsWith(kArmorEnd)) ++ends_;
    if (raw.contains(kArmorBegin)) any_armor_ = true;

    const auto line = raw.trimmed();
    if (line.isEmpty()) return;
    if (first_.isEmpty()) first_ = line.toByteArray();
    last_ = line.toByteArray();
  }

  [[nodiscard]] auto Outside() const -> bool {
    if (!any_armor_) return false;
    const auto cleartext = first_.startsWith(kPgpSignedBegin);
    if (!cleartext && !first_.startsWith(kPgpMessageBegin)) return true;
    if (!last_.endsWith(cleartext
                            ? QByteArrayView(kPgpSignatureEnd)
                            : QByteArrayView("-----END PGP MESSAGE-----"))) {
      return true;
    }
    // two openings (message, signature) for cleartext, one otherwise
    return begins_ != (cleartext ? 2 : 1) || ends_ != 1;
  }

 private:
  static constexpr auto kArmorBegin = QByteArrayView("-----BEGIN PGP ");
  static constexpr auto kArmorEnd = QByteArrayView("-----END PGP ");

  QByteArray first_;
  QByteArray last_;
  int begins_ = 0;
  int ends_ = 0;
  bool any_armor_ = false;
};

}  // namespace

auto HasTextOutsideSignedArmor(const QByteArray& data) -> bool {
  ArmorEnvelopeCheck check;
  for (const auto& line : data.split('\n')) check.Line(line);
  return check.Outside();
}

auto HasTextOutsideSignedArmorInFile(const QString& path) -> bool {
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly)) return false;
  // A line at a time: a signed file can be large, and only its armor lines
  // and its first and last lines matter.
  ArmorEnvelopeCheck check;
  constexpr qint64 kMaxLine = 64 * 1024;
  while (!file.atEnd()) {
    const auto line = file.readLine(kMaxLine);
    if (line.isEmpty() && file.error() != QFileDevice::NoError) return false;
    check.Line(line);
  }
  return check.Outside();
}

auto CurrentDateTime() -> QDateTime {
  const auto fixed = Module::RetrieveRTValueTypedOrDefault<qint64>(
      "core", kFixedClockRTKey, -1);
  if (fixed < 0) return QDateTime::currentDateTime();
  return QDateTime::fromSecsSinceEpoch(fixed);
}

auto EnsureSodiumInit() -> bool {
  static const int kRc = sodium_init();
  if (kRc < 0) {
    LOG_E() << "sodium_init failed";
    return false;
  }
  return true;
}

auto SecureLevelFromApp() -> int {
  if (qApp == nullptr) return 0;
  return qApp->property("GFSecureLevel").toInt();
}

auto ResolveLayeredValue(const QVariant& env_value, const QVariant& user_value,
                         const QVariant& fallback) -> QVariant {
  if (env_value.isValid()) return env_value;
  if (user_value.isValid()) return user_value;
  return fallback;
}

auto IsRunningInAppSandbox() -> bool {
  return Platform::IsRunningInAppSandbox();
}

auto IsRunningInSandBox() -> bool {
  return IsFlatpakENV() || IsRunningInAppSandbox();
}
}  // namespace GpgFrontend