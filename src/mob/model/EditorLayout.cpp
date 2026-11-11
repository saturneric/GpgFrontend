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

#include "model/EditorLayout.h"

#include <QGuiApplication>
#include <QInputMethod>
#include <QMetaMethod>
#include <QQuickTextDocument>
#include <QTextDocument>

#include "shell/present/TextDirection.h"

namespace GpgFrontend::Mobile {

EditorLayout::EditorLayout(QObject* parent) : QObject(parent) {
  // The editor follows the keyboard's language in its own option.
  connect(QGuiApplication::inputMethod(), &QInputMethod::inputDirectionChanged,
          this, &EditorLayout::apply, Qt::QueuedConnection);
}

auto EditorLayout::Target() const -> QObject* { return target_; }

void EditorLayout::SetTarget(QObject* target) {
  if (target_ == target) return;
  if (target_ != nullptr) disconnect(target_, nullptr, this, nullptr);
  if (document_ != nullptr) disconnect(document_, nullptr, this, nullptr);
  target_ = target;
  document_ = nullptr;
  if (target != nullptr) {
    if (auto* quick =
            target->property("textDocument").value<QQuickTextDocument*>();
        quick != nullptr) {
      document_ = quick->textDocument();
    }
    // What makes the editor write its own option: put ours back after it.
    static const QList<QByteArray> kRewrites{
        "textChanged", "wrapModeChanged", "horizontalAlignmentChanged",
        "effectiveHorizontalAlignmentChanged", "verticalAlignmentChanged"};
    const auto* meta = target->metaObject();
    const auto slot =
        metaObject()->method(metaObject()->indexOfMethod("apply()"));
    for (int i = 0; i < meta->methodCount(); ++i) {
      const auto method = meta->method(i);
      if (method.methodType() == QMetaMethod::Signal &&
          kRewrites.contains(method.name())) {
        connect(target, method, this, slot);
      }
    }
  }
  apply();
  emit SignalChanged();
}

auto EditorLayout::Direction() const -> QString { return direction_; }

void EditorLayout::SetDirection(const QString& direction) {
  if (direction_ == direction) return;
  direction_ = direction;
  apply();
  emit SignalChanged();
}

auto EditorLayout::Alignment() const -> QString { return alignment_; }

void EditorLayout::SetAlignment(const QString& alignment) {
  if (alignment_ == alignment) return;
  alignment_ = alignment;
  apply();
  emit SignalChanged();
}

auto EditorLayout::Armored() const -> bool { return armored_; }

void EditorLayout::SetArmored(bool armored) {
  if (armored_ == armored) return;
  armored_ = armored;
  apply();
  emit SignalChanged();
}

auto EditorLayout::LeadingRight() const -> bool { return leading_right_; }

auto EditorLayout::Overrides() const -> int { return overrides_; }

auto EditorLayout::FirstStrong(QStringView text) -> Qt::LayoutDirection {
  // The desktop's rule, by code point (a script above the BMP counts).
  return Shell::FirstStrongDirection(text).value_or(Qt::LayoutDirectionAuto);
}

auto EditorLayout::wanted_direction(Qt::LayoutDirection fallback)
    -> Qt::LayoutDirection {
  auto first = Qt::LayoutDirectionAuto;
  bool mixed = false;
  for (auto block = document_->begin(); block.isValid(); block = block.next()) {
    const auto dir = FirstStrong(block.text());
    if (dir == Qt::LayoutDirectionAuto) continue;
    if (first == Qt::LayoutDirectionAuto) {
      first = dir;
    } else if (dir != first) {
      mixed = true;
      break;
    }
  }
  const bool right = !armored_ && first == Qt::RightToLeft;
  if (leading_right_ != right) {
    leading_right_ = right;
    // After this pass: the editor's alignment follows, and writes its
    // option again, which comes back here.
    QMetaObject::invokeMethod(
        this, [this]() { emit SignalLeadingChanged(); }, Qt::QueuedConnection);
  }

  if (direction_ == QLatin1String("ltr")) return Qt::LeftToRight;
  if (direction_ == QLatin1String("rtl")) return Qt::RightToLeft;
  // Armor is ASCII and runs left to right whatever is around it.
  if (armored_) return Qt::LeftToRight;
  if (mixed) return Qt::LayoutDirectionAuto;
  return first == Qt::LayoutDirectionAuto ? fallback : first;
}

namespace {

/// The side a line starts at, as laid out in @p direction.
auto Visual(Qt::LayoutDirection direction, Qt::Alignment alignment)
    -> Qt::Alignment {
  const auto horizontal = alignment & Qt::AlignHorizontal_Mask;
  if ((alignment & Qt::AlignAbsolute) != 0 || direction != Qt::RightToLeft) {
    return horizontal & ~Qt::AlignAbsolute;
  }
  if ((horizontal & Qt::AlignLeft) != 0) return Qt::AlignRight;
  if ((horizontal & Qt::AlignRight) != 0) return Qt::AlignLeft;
  return horizontal;
}

}  // namespace

void EditorLayout::apply() {
  if (document_ == nullptr) return;
  auto option = document_->defaultTextOption();
  const auto direction = wanted_direction(option.textDirection());
  const auto vertical = option.alignment() & Qt::AlignVertical_Mask;

  auto alignment = option.alignment();
  if (alignment_ == QLatin1String("left") ||
      alignment_ == QLatin1String("right")) {
    const auto side =
        alignment_ == QLatin1String("left") ? Qt::AlignLeft : Qt::AlignRight;
    // Laid out as one direction, the editor's own may already start lines
    // there; each paragraph its own way, only an absolute side holds.
    if (direction == Qt::LayoutDirectionAuto ||
        Visual(direction, alignment) != side) {
      alignment = vertical | side | Qt::AlignAbsolute;
    }
  } else {
    // Each paragraph from where it runs: Qt reads a left that is not
    // absolute as the leading side.
    alignment = vertical | Qt::AlignLeft;
  }

  if (option.textDirection() == direction && option.alignment() == alignment) {
    return;
  }
  option.setTextDirection(direction);
  option.setAlignment(alignment);
  ++overrides_;
  document_->setDefaultTextOption(option);
}

}  // namespace GpgFrontend::Mobile
