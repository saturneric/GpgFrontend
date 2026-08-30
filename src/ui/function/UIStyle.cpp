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

#include "ui/function/UIStyle.h"

namespace GpgFrontend::UI {

namespace {

/// Whether the palette is a dark one. Shared by every colour below so they all
/// agree about which theme they are painting into.
auto IsDarkPalette(const QPalette& palette) -> bool {
  return palette.color(QPalette::Base).lightness() < 128;
}

/// The ordinary text colour mixed @p strength of the way towards the window
/// background. Opaque rather than translucent because a label drawing through
/// rich text drops an alpha channel.
auto MixTextTowardsWindow(const QPalette& palette, double strength) -> QColor {
  const auto text = palette.color(QPalette::WindowText);
  const auto back = palette.color(QPalette::Window);

  const auto mix = [strength](int a, int b) -> int {
    return static_cast<int>((a * strength) + (b * (1 - strength)));
  };
  return {mix(text.red(), back.red()), mix(text.green(), back.green()),
          mix(text.blue(), back.blue())};
}

}  // namespace

auto AccentColor(const QPalette& palette, bool positive) -> QColor {
  if (!positive) {
    auto color = palette.color(QPalette::Text);
    color.setAlpha(150);
    return color;
  }
  return IsDarkPalette(palette) ? QColor(102, 187, 106) : QColor(46, 125, 50);
}

auto MutedTextColor(const QPalette& palette) -> QColor {
  // How much of the real text colour survives in a caption or a sub-line.
  return MixTextTowardsWindow(palette, 0.72);
}

auto BorderColor(const QPalette& palette) -> QColor {
  return MixTextTowardsWindow(palette, 0.22);
}

auto WarningColor(const QPalette& palette) -> QColor {
  return IsDarkPalette(palette) ? QColor(227, 179, 65) : QColor(155, 100, 0);
}

auto DangerColor(const QPalette& palette) -> QColor {
  return IsDarkPalette(palette) ? QColor(239, 83, 80) : QColor(198, 40, 40);
}

void SetChip(QLabel* label, const QString& text, const QColor& color) {
  label->setText(QString("<span style=\"color:%1;\">%2</span>")
                     .arg(color.name(QColor::HexRgb), text.toHtmlEscaped()));
}

void SetLabelTextColor(QLabel* label, const QColor& color) {
  auto palette = label->palette();
  palette.setColor(QPalette::WindowText, color);
  label->setPalette(palette);
}

auto CreateCard(const QString& title, QWidget* content, QWidget* parent)
    -> QFrame* {
  auto* frame = new QFrame(parent);
  frame->setObjectName(QStringLiteral("GFCard"));
  frame->setFrameShape(QFrame::StyledPanel);

  // A hairline drawn from the palette rather than the platform's StyledPanel
  // groove, which is a bevel on some styles and nothing at all on others.
  frame->setStyleSheet(QStringLiteral("QFrame#GFCard {"
                                      "  border: 1px solid %1;"
                                      "  border-radius: 8px;"
                                      "}")
                           .arg(BorderColor(frame->palette()).name()));

  auto* title_label = new QLabel(QStringLiteral("<b>%1</b>").arg(title), frame);
  title_label->setTextFormat(Qt::RichText);
  title_label->setWordWrap(true);

  auto* layout = new QVBoxLayout(frame);
  layout->setContentsMargins(14, 12, 14, 12);
  layout->setSpacing(8);
  layout->addWidget(title_label);
  layout->addWidget(content, 1);

  // A card is as tall as what is in it. Left to grow, it takes a share of
  // whatever height its page has spare and pads its contents with it, which is
  // how a card of five readings ends up half a screen tall -- unless the thing
  // inside genuinely wants the room, which it says by being expandable.
  if (content->sizePolicy().verticalPolicy() != QSizePolicy::Expanding) {
    frame->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
  }

  return frame;
}

auto HumanSize(qint64 bytes) -> QString {
  return QLocale().formattedDataSize(bytes, 1,
                                     QLocale::DataSizeTraditionalFormat);
}

auto CreateDetailLabel(const QString& text, QWidget* parent) -> QLabel* {
  auto* label = new QLabel(text, parent);
  label->setWordWrap(true);
  label->setTextInteractionFlags(Qt::TextSelectableByMouse);

  auto font = label->font();
  font.setPointSizeF(font.pointSizeF() * 0.92);
  label->setFont(font);

  SetLabelTextColor(label, MutedTextColor(label->palette()));
  return label;
}

auto CreateDisclosure(const QString& title, QWidget* content, QWidget* parent)
    -> QWidget* {
  auto* holder = new QWidget(parent);
  auto* layout = new QVBoxLayout(holder);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);

  auto* toggle = new QToolButton(holder);
  toggle->setCheckable(true);
  toggle->setChecked(false);
  toggle->setText(title);
  toggle->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  toggle->setArrowType(Qt::RightArrow);
  toggle->setAutoRaise(true);

  content->setParent(holder);
  content->hide();

  QObject::connect(
      toggle, &QToolButton::toggled, holder, [toggle, content](bool open) {
        toggle->setArrowType(open ? Qt::DownArrow : Qt::RightArrow);
        content->setVisible(open);
      });

  auto* toggle_row = new QHBoxLayout();
  toggle_row->addWidget(toggle);
  toggle_row->addStretch(1);

  layout->addLayout(toggle_row);
  layout->addWidget(content);

  return holder;
}

auto CreateDialogHeader(const QString& icon, const QString& title,
                        const QString& subtitle, QWidget* parent) -> QLayout* {
  auto* icon_label = new QLabel(parent);
  icon_label->setPixmap(QPixmap(icon).scaled(40, 40, Qt::KeepAspectRatio,
                                             Qt::SmoothTransformation));
  icon_label->setFixedSize(40, 40);

  auto* title_label = new QLabel(title, parent);
  auto title_font = title_label->font();
  title_font.setBold(true);
  title_font.setPointSizeF(title_font.pointSizeF() * 1.25);
  title_label->setFont(title_font);

  // Icon and title share one line; the wrapped description spans the full width
  // beneath them. Keeping the subtitle out of the icon's column lets the
  // vertical layout honour its height-for-width, so no line ever clips -- a
  // wrapped QLabel nested inside a horizontal layout does not get that.
  auto* title_row = new QHBoxLayout();
  title_row->setSpacing(14);
  title_row->addWidget(icon_label, 0, Qt::AlignVCenter);
  title_row->addWidget(title_label, 1);

  auto* header = new QVBoxLayout();
  header->setSpacing(6);
  header->addLayout(title_row);

  if (!subtitle.isEmpty()) {
    auto* subtitle_label = new QLabel(subtitle, parent);
    subtitle_label->setWordWrap(true);
    SetLabelTextColor(subtitle_label,
                      MutedTextColor(subtitle_label->palette()));
    header->addWidget(subtitle_label);
  }

  // A native rule sets the identity apart from what follows without the weight
  // of a boxed group; the platform style draws it to match its own separators
  // on every OS and theme.
  auto* separator = new QFrame(parent);
  separator->setFrameShape(QFrame::HLine);
  separator->setFrameShadow(QFrame::Sunken);
  header->addSpacing(2);
  header->addWidget(separator);

  return header;
}

}  // namespace GpgFrontend::UI
