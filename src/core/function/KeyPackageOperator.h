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
 * Saturneric <eric@bktus.com) starting on May 12, 2021.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#pragma once

#include "core/function/GFBufferFactory.h"
#include "core/model/GFBuffer.h"
#include "core/model/GpgImportInformation.h"
#include "core/typedef/GpgTypedef.h"

namespace GpgFrontend {

/**
 * @brief One key found inside an opened key package.
 */
struct KeyPackageKey {
  QString fpr;
  QString key_id;
  QString name;   ///< the primary user ID's name
  QString email;  ///< the primary user ID's email
  bool has_secret = false;
};

/**
 * @brief What protects a key package.
 *
 * Version 2 knows only the PIN. Version 3 adds a plain key, for a package
 * that goes to a tool such as gfpipe.
 */
enum class KeyPackageProtection : std::uint8_t {
  kPIN_FILE,  ///< a PIN, with the wrapped key in a separate key file
  kPLAIN,     ///< a random secret the user keeps as text
};

/**
 * @brief What a key package's plaintext header says, read without opening it.
 */
struct KeyPackageInfo {
  int version = 0;  ///< 2 or 3; 0 when the file is not a key package
  KeyPackageProtection protection = KeyPackageProtection::kPIN_FILE;
  /// Some secret key in it has no passphrase of its own (version 3 only).
  bool secrets_unprotected = false;
  QString package_id;       ///< version 3 only
  qint64 generated_at = 0;  ///< Unix seconds; version 3 only
  int key_count = 0;        ///< version 3 only
  /// Made by a newer GpgFrontend than this one can read; nothing else in the
  /// info is filled in.
  bool too_new = false;
};

/**
 * @brief What to write as a version 3 key package.
 */
struct KeyPackageV3Request {
  QString package_path;
  QString key_path;  ///< the separate key file; PIN only
  GpgAbstractKeyPtrList keys;
  bool secret = false;  ///< include the secret keys of keys that have one
  /// Write the secret keys without their passphrase; the package's own
  /// protection then guards them. The key database is not changed.
  bool remove_passphrase = false;
  KeyPackageProtection protection = KeyPackageProtection::kPIN_FILE;
  GFBuffer pin;  ///< PIN only
  /// A gfpipe program to set up beside the package, ready to run: it is
  /// copied into the package's folder, and a plain key is saved there as
  /// `<name>.gfkey`, where gfpipe looks for it. Empty for none.
  QString gfpipe_program;
};

/**
 * @brief What writing a version 3 key package produced.
 */
struct KeyPackageV3Written {
  QString error;  ///< empty on success
  /// The key as text for the user to keep (plain only), shown once.
  GFBuffer plain_key;
  QString package_id;
  QString gfpipe_path;    ///< the copied gfpipe, when one was asked for
  QString key_text_path;  ///< the saved plain key, when set up for gfpipe
};

/**
 * @brief Singleton operator for creating and importing encrypted key packages.
 *
 * A key package bundles exported OpenPGP key data encrypted with a randomly
 * generated passphrase. That passphrase is itself encrypted with a caller-
 * supplied PIN and written to a separate key file, so the two files must be
 * kept together to restore the keys.
 */
class GF_CORE_EXPORT KeyPackageOperator
    : public SingletonFunctionObject<KeyPackageOperator> {
 public:
  /**
   * @brief Construct the operator for the given singleton channel.
   *
   * @param channel singleton channel identifier
   */
  explicit KeyPackageOperator(
      int channel = SingletonFunctionObject::GetDefaultChannel());

  /**
   * @brief Generate a unique key package file name.
   *
   * The name has the form "KeyPackage_XXXXXXXX" where the suffix is derived
   * from a cryptographically random z-base-32 string.
   *
   * @return generated file name string
   */
  auto GenerateKeyPackageName() -> QString;

  /**
   * @brief Export and encrypt a set of keys into a key package file pair.
   *
   * Exports @p keys from GPG, encrypts the key data with a freshly generated
   * random passphrase, then encrypts that passphrase with @p pin. The
   * encrypted key data is written to @p key_package_path and the encrypted
   * passphrase is written to @p key_path. Results are delivered via @p cb.
   *
   * @param key_package_path destination path for the encrypted key data file
   * @param key_path destination path for the encrypted passphrase file
   * @param keys list of keys to export
   * @param pin PIN used to encrypt the package passphrase
   * @param secret if true, include secret key material in the export
   * @param cb callback invoked with the operation result (0 on success, -1 on
   * failure)
   */
  void GenerateKeyPackage(const QString &key_package_path,
                          const QString &key_path,
                          const GpgAbstractKeyPtrList &keys,
                          const GFBuffer &pin, bool secret,
                          const OperationCallback &cb);

  /**
   * @brief Decrypt and import a key package.
   *
   * Reads the encrypted key data from @p key_package_path and the encrypted
   * passphrase from @p key_path, decrypts both using @p pin, then imports
   * the recovered key data into GPG.
   *
   * @param key_package_path path to the encrypted key data file
   * @param key_path path to the encrypted passphrase file
   * @param pin PIN used to decrypt the package passphrase
   * @return tuple of (error message string, import information); error string
   * is empty on success
   */
  auto ImportKeyPackage(const QString &key_package_path,
                        const QString &key_path, const GFBuffer &pin)
      -> std::tuple<QString, QSharedPointer<GpgImportInformation>>;

  /**
   * @brief Decrypt a key package without importing anything.
   *
   * The first half of ImportKeyPackage(): read both files, open the key file
   * with @p pin, then open the package with what it held.
   *
   * @param key_package_path path to the encrypted key data file
   * @param key_path path to the encrypted passphrase file
   * @param pin PIN used to decrypt the package passphrase
   * @return tuple of (error message string, key data); the error is empty on
   * success
   */
  auto OpenKeyPackage(const QString &key_package_path, const QString &key_path,
                      const GFBuffer &pin) -> std::tuple<QString, GFBuffer>;

  /**
   * @brief List the keys in opened key data, one entry per fingerprint.
   *
   * A package written with private keys carries each key twice, once in its
   * public section and once in its secret one; they are listed once.
   *
   * @param key_data what OpenKeyPackage() returned
   * @return the keys, in the order they first appear
   */
  static auto ListKeyPackageKeys(const GFBuffer &key_data)
      -> QList<KeyPackageKey>;

  /**
   * @brief Import some of the keys in opened key data.
   *
   * Only the keys whose fingerprint is in @p fprs reach the key database,
   * with every part the package holds of them.
   *
   * @param key_data what OpenKeyPackage() returned
   * @param fprs fingerprints of the keys to import
   * @return tuple of (error message string, import information); the error
   * is empty on success
   */
  auto ImportKeyPackageKeys(const GFBuffer &key_data, const QStringList &fprs)
      -> std::tuple<QString, QSharedPointer<GpgImportInformation>>;

  /**
   * @brief Write a version 3 key package, blocking.
   *
   * Exports each key of @p request on this channel, removes the passphrases
   * when asked (prompting for each key once), seals the package and writes its
   * files.
   *
   * @param request what to write
   * @return the outcome
   */
  auto GenerateKeyPackageV3Sync(const KeyPackageV3Request &request)
      -> KeyPackageV3Written;

  /**
   * @brief GenerateKeyPackageV3Sync() on a task runner.
   *
   * @param request what to write
   * @param cb receives the outcome on the calling thread
   */
  void GenerateKeyPackageV3(const KeyPackageV3Request &request,
                            const std::function<void(KeyPackageV3Written)> &cb);

  /**
   * @brief Read what a key package's header says, without opening it.
   *
   * @param key_package_path the package file
   * @return the header, with version 0 when the file is not a key package
   */
  static auto InspectKeyPackage(const QString &key_package_path)
      -> KeyPackageInfo;

  /**
   * @brief The gfpipe this installation carries: beside the application,
   * or in Contents/Helpers of a macOS bundle.
   *
   * @return its path; empty when this installation has none
   */
  static auto BundledGfPipe() -> QString;

  /**
   * @brief Open a plain version 3 package.
   *
   * @param key_package_path the package file
   * @param secret the plain key text
   * @return tuple of (error message, key data), as OpenKeyPackage()
   */
  static auto OpenKeyPackageWithSecret(const QString &key_package_path,
                                       const GFBuffer &secret)
      -> std::tuple<QString, GFBuffer>;

 private:
  GFBufferFactory &gb_fac_ = GFBufferFactory::GetInstance(
      GetChannel());  ///< Factory used for random passphrase generation and
                      ///< encryption

  /**
   * @brief Generate the unique key package file name (implementation).
   *
   * @return generated file name string
   */
  auto generate_key_package_name() -> QString;
};
}  // namespace GpgFrontend
