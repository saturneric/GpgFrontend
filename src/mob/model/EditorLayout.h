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
#include <QPointer>
#include <QTextOption>

class QTextDocument;

namespace GpgFrontend::Mobile {

/**
 * @brief Which way the lines of a text editor run, and which side they start
 * at: what a TextArea cannot be told itself.
 *
 * A Qt Quick TextEdit lays its document out in a single direction, the input
 * method's, and offers no per-paragraph direction. Here the document's own
 * text option is set instead: "auto" lays each paragraph out by its first
 * strong letters (the Unicode bidirectional algorithm), so a Hebrew line and
 * an English one each run their way; "ltr" and "rtl" fix it.
 *
 * Alignment "auto" starts each line where its paragraph runs from; "left" and
 * "right" are absolute, whatever the direction. Armored text in "auto" runs
 * and starts left to right, so OpenPGP blocks always read the same.
 *
 * The editor writes its own text option whenever its text, wrap mode or
 * alignment change: one direction for the whole text, from its first
 * paragraph with letters that have one. This puts the chosen one back after
 * it, and only when it differs in effect: text that runs one way throughout,
 * as most does, is left as the editor laid it out, so typing never lays it
 * out twice. The editor's alignment is given by leadingRight, never left to
 * follow the cursor, so moving it lays nothing out again.
 */
class EditorLayout : public QObject {
  Q_OBJECT
  QML_ELEMENT

  /// The TextArea or TextEdit whose document is laid out.
  Q_PROPERTY(QObject* target READ Target WRITE SetTarget NOTIFY SignalChanged)
  /// "auto", "ltr" or "rtl".
  Q_PROPERTY(
      QString direction READ Direction WRITE SetDirection NOTIFY SignalChanged)
  /// "auto", "left" or "right".
  Q_PROPERTY(
      QString alignment READ Alignment WRITE SetAlignment NOTIFY SignalChanged)
  /// The text is OpenPGP armor: "auto" means left to right.
  Q_PROPERTY(bool armored READ Armored WRITE SetArmored NOTIFY SignalChanged)
  /// The text's first paragraph with a direction runs right to left: the
  /// editor's leading side, which its horizontalAlignment names in "auto".
  Q_PROPERTY(bool leadingRight READ LeadingRight NOTIFY SignalLeadingChanged)
  /// How often the editor's own layout had to be put right: each time lays
  /// the whole text out again. Text that runs one way needs none.
  Q_PROPERTY(int overrides READ Overrides CONSTANT)

 public:
  explicit EditorLayout(QObject* parent = nullptr);

  [[nodiscard]] auto Target() const -> QObject*;
  void SetTarget(QObject* target);
  [[nodiscard]] auto Direction() const -> QString;
  void SetDirection(const QString& direction);
  [[nodiscard]] auto Alignment() const -> QString;
  void SetAlignment(const QString& alignment);
  [[nodiscard]] auto Armored() const -> bool;
  void SetArmored(bool armored);
  [[nodiscard]] auto LeadingRight() const -> bool;
  [[nodiscard]] auto Overrides() const -> int;

  /// Which way @p text runs by its first letters that have a direction;
  /// Qt::LayoutDirectionAuto when none has.
  [[nodiscard]] static auto FirstStrong(QStringView text)
      -> Qt::LayoutDirection;

  /// Put the chosen direction and alignment back if the editor changed
  /// them.
  Q_INVOKABLE void apply();

 signals:
  void SignalChanged();
  void SignalLeadingChanged();

 private:
  QPointer<QObject> target_;
  QPointer<QTextDocument> document_;
  QString direction_ = QStringLiteral("auto");
  QString alignment_ = QStringLiteral("auto");
  bool armored_ = false;
  bool leading_right_ = false;
  int overrides_ = 0;

  /// The direction the text is laid out in: a concrete one when every
  /// paragraph runs the same way, else Qt::LayoutDirectionAuto (each its
  /// own); @p fallback when no paragraph has a direction.
  [[nodiscard]] auto wanted_direction(Qt::LayoutDirection fallback)
      -> Qt::LayoutDirection;
};

}  // namespace GpgFrontend::Mobile
