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

#pragma once

#include <QtQml/qqmlregistration.h>

#include <QObject>
#include <QString>

namespace GpgFrontend::Mobile {

/**
 * @brief What a text holds, by its first OpenPGP armor header: "empty",
 * "plain", "message" (encrypted), "signed" (signed in the clear),
 * "signature" (a detached one), "public_key", "private_key".
 *
 * A label and a suggestion of what to do, never a gate: every operation
 * stays offered whatever the text looks like. Text before the armor (a
 * quoted mail, say) does not change what the armor is.
 */
auto ClassifyText(const QString& text) -> QString;

/**
 * @brief What a file is, by its name alone, as the desktop's file panel
 * tells: "message" for .gpg, .pgp and .asc (encrypted or signed data),
 * "signature" for .sig (a detached signature), empty for anything else.
 *
 * Nothing is read: a label and a suggestion, never a gate, so every
 * operation stays offered whatever the name says.
 */
auto ClassifyFileName(const QString& name) -> QString;

/**
 * @brief What a file holds, from its name and its first bytes @p head: the
 * name is a hint, the content confirms or refutes it.
 *
 * Armored text says what it is under any name (ClassifyText()). Binary data
 * named as OpenPGP (.gpg, .pgp, .sig) is believed only when its first packet,
 * as the rPGP inspector frames it, is one such a file starts with; a file
 * named so that is not OpenPGP data is nothing in particular. Binary data
 * under another name is not looked into: a picture's first byte can pass for
 * a packet. Without the inspector (or with nothing read) the name decides.
 *
 * Nothing is decrypted, and like ClassifyFileName() it is a label, never a
 * gate.
 */
/// What a file holds, and how that is known.
struct FileKind {
  /// "message" (encrypted or signed data), "signature", "key", or empty.
  QString kind;
  /// Its bytes said so (an armor header, a first packet), not its name.
  bool confirmed = false;
  /// Named as OpenPGP data, and its bytes say it is not.
  bool refuted = false;
};

/// What the file named @p name holds, by its first bytes @p head, and
/// whether the bytes or the name decided (see above).
auto ClassifyFileHeadOf(const QString& name, const QByteArray& head)
    -> FileKind;

/// ClassifyFileHeadOf() of the file at @p path (a local path or a content
/// URI), reading no more than its first bytes.
auto ClassifyFileOf(const QString& name, const QString& path) -> FileKind;

/**
 * @brief Whether the operation @p op ("encrypt", "decrypt", "sign",
 * "verify", "encrypt_sign", "decrypt_verify", "symmetric") can run on text
 * of the kind @p kind (ClassifyText()).
 *
 * Only an encrypted message is decrypted, and only a message or a
 * cleartext signed text is verified (the text page has no detached
 * signature to go with plain text); anything is encrypted or signed, an
 * armored block too. As the Files page decides it for files
 * (FileWorkspace::allowsEntries()).
 */
auto TextOperationAllowed(const QString& op, const QString& kind) -> bool;

/// ClassifyText() for QML.
class ContentKind : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON

 public:
  using QObject::QObject;

  [[nodiscard]] Q_INVOKABLE QString textKind(const QString& text) const;
  /// TextOperationAllowed() for QML.
  [[nodiscard]] Q_INVOKABLE bool allowsText(const QString& op,
                                            const QString& kind) const;
};

}  // namespace GpgFrontend::Mobile
