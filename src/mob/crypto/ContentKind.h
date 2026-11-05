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
 * "message" (encrypted or signed data), "signature", "key", or empty.
 * Nothing is decrypted, and like ClassifyFileName() it is a label, never a
 * gate.
 */
auto ClassifyFileHead(const QString& name, const QByteArray& head) -> QString;

/// ClassifyFileHead() of the file at @p path (a local path or a content
/// URI), reading no more than its first bytes.
auto ClassifyFile(const QString& name, const QString& path) -> QString;

/// ClassifyText() for QML.
class ContentKind : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON

 public:
  using QObject::QObject;

  [[nodiscard]] Q_INVOKABLE QString textKind(const QString& text) const;
};

}  // namespace GpgFrontend::Mobile
