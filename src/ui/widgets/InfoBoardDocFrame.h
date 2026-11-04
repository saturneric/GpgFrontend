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

#include "core/utils/ReportSealPainter.h"
#include "ui/function/UIStyle.h"

namespace GpgFrontend::UI {

/**
 * @brief The report's colours, taken from the current palette.
 *
 * Every colour the report draws with comes from here, so it follows the
 * application's light or dark theme, and a theme switch only needs these
 * recomputed. Status colours are the widget's (InfoBoardWidget::StatusColor).
 */
struct ReportColors {
  QColor text;       ///< primary text
  QColor muted;      ///< labels, sources, footer: readable, quieter
  QColor technical;  ///< hashes, codes, identifiers: between the two
  QColor border;     ///< card and document outlines, hairlines
  QColor surface;    ///< a card, a step off the panel
  bool dark = false;

  static auto Blend(const QColor& a, const QColor& b, int wa, int wb)
      -> QColor {
    return BlendReportColor(a, b, wa, wb);
  }

  static auto From(const QPalette& palette) -> ReportColors {
    ReportColors c;
    const auto window = palette.color(QPalette::Window);
    c.text = palette.color(QPalette::WindowText);
    c.dark = IsDarkPalette(palette);
    c.muted = Blend(c.text, window, 3, 2);
    c.technical = Blend(c.text, window, 4, 1);
    c.border = BorderColor(palette);
    // Dark: a card is slightly lighter than the charcoal panel. Light: the
    // base colour, a clean sheet on the grey panel.
    c.surface = c.dark ? window.lighter(118) : palette.color(QPalette::Base);
    return c;
  }
};

/// What kind of text a label carries; its colour follows the theme.
enum class ReportTone : std::uint8_t {
  kTEXT,
  kMUTED,
  kTECHNICAL,
  kRULE,  ///< a separator line
};

/// Colour @p label for @p tone from @p palette, and remember the tone so a
/// theme switch can colour it again (see InfoBoardWidget::changeEvent).
inline void ApplyTone(QWidget* label, ReportTone tone,
                      const QPalette& palette) {
  label->setProperty("reportTone", static_cast<int>(tone));
  const auto colors = ReportColors::From(palette);
  const auto& color = tone == ReportTone::kMUTED       ? colors.muted
                      : tone == ReportTone::kTECHNICAL ? colors.technical
                      : tone == ReportTone::kRULE      ? colors.border
                                                       : colors.text;
  label->setStyleSheet(u"color: %1;"_s.arg(color.name()));
}

/**
 * @brief QFrame subclass that paints a faint logo watermark behind its
 * children. Used as the backdrop of the generated report document.
 */
class DocFrame final : public QFrame {
 public:
  using QFrame::QFrame;

  /// A report is shown: draw the watermark and the inner rule.
  void SetWatermark(const QString& /*text*/, const QColor& /*color*/) {
    watermark_active_ = true;
    if (watermark_.isNull()) {
      watermark_ = QPixmap(u":/icons/gpgfrontend_logo.png"_s);
    }
    update();
  }

  void ClearWatermark() {
    watermark_active_ = false;
    update();
  }

 protected:
  void paintEvent(QPaintEvent* ev) override {
    QFrame::paintEvent(ev);
    if (!watermark_active_) return;

    const auto colors = ReportColors::From(palette());
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::SmoothPixmapTransform);

    // The GF mark as the paper's watermark: one, large, faint, centred
    // behind everything. Painted, not laid out, so it takes no space; the
    // opaque cards keep it out from under the text they hold.
    if (!watermark_.isNull()) {
      constexpr int kMaxSide = 280;
      const int side = std::min({kMaxSide, static_cast<int>(width() * 0.55),
                                 static_cast<int>(height() * 0.8)});
      if (side > 32) {
        const QPixmap mark = watermark_.scaled(side, side, Qt::KeepAspectRatio,
                                               Qt::SmoothTransformation);
        p.setOpacity(colors.dark ? 0.06 : 0.045);
        p.drawPixmap((width() - mark.width()) / 2,
                     (height() - mark.height()) / 2, mark);
        p.setOpacity(1.0);
      }
    }

    // A document's inner rule: a hairline a few pixels inside the frame,
    // the quiet second line of a certificate border.
    QColor rule = colors.border;
    rule.setAlpha(140);
    p.setPen(QPen(rule, 1.0));
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(QRectF(rect()).adjusted(3.5, 3.5, -3.5, -3.5), 1.5, 1.5);
  }

 private:
  bool watermark_active_ = false;
  QPixmap watermark_;
};

/**
 * @brief The source shown at the right of a report card's header.
 *
 * Shows as much of the source as fits, cut in the middle, and never more than
 * kMaxShare of the card, so the title keeps the rest. The full text is the
 * tooltip; text() is what is on screen.
 */
class CardSourceLabel final : public QLabel {
 public:
  CardSourceLabel(const QString& full, QWidget* parent)
      : QLabel(parent), full_(full) {
    setObjectName(u"InfoBoardCardSource"_s);
    setTextFormat(Qt::PlainText);
    setToolTip(full_);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    setText(full_);
  }

  /// The width the whole source needs.
  [[nodiscard]] auto FullWidth() const -> int {
    return fontMetrics().horizontalAdvance(full_) + 2;
  }

  [[nodiscard]] auto sizeHint() const -> QSize override {
    return {FullWidth(), QLabel::sizeHint().height()};
  }

  [[nodiscard]] auto minimumSizeHint() const -> QSize override {
    return {fontMetrics().horizontalAdvance(u"…"_s) * 3,
            QLabel::minimumSizeHint().height()};
  }

 protected:
  void resizeEvent(QResizeEvent* event) override {
    QLabel::resizeEvent(event);
    // sizeHint() reports the full text, so replacing the shown text here
    // never feeds back into the layout.
    const auto shown =
        fontMetrics().elidedText(full_, Qt::ElideMiddle, event->size().width());
    if (shown != text()) setText(shown);
  }

 private:
  QString full_;
};

/**
 * @brief A row that becomes a column when it gets narrow.
 *
 * Side by side while there is room; below kStackBelow its items stack, each
 * taking the full width, rather than squeezing. The switch depends only on
 * the row's width, which stacking does not change, so it cannot oscillate.
 */
class StackingRow : public QWidget {
 public:
  StackingRow(int stack_below, QWidget* parent)
      : QWidget(parent),
        stack_below_(stack_below),
        box_(new QBoxLayout(QBoxLayout::LeftToRight, this)) {
    box_->setContentsMargins(0, 0, 0, 0);
    // The row reports its own minimum (the stacked one); the layout must
    // not pin the widget to the side-by-side minimum, or it could never get
    // narrow enough to stack.
    box_->setSizeConstraint(QLayout::SetNoConstraint);
  }

  [[nodiscard]] auto Box() const -> QBoxLayout* { return box_; }
  [[nodiscard]] auto IsStacked() const -> bool { return stacked_; }

  /// Stacked, the row is as narrow as its widest item.
  [[nodiscard]] auto minimumSizeHint() const -> QSize override {
    int width = 0;
    for (auto* w : Items()) width = std::max(width, ItemMinWidth(w, true));
    return {width, QWidget::minimumSizeHint().height()};
  }

 protected:
  /// Called whenever the row switches; subclasses adjust their items.
  virtual void OnStackChanged(bool /*stacked*/) {}

  /// The spacing between items when side by side.
  virtual auto SideBySideSpacing() const -> int { return 8; }

  /// An item's minimum width; @p stacked asks for the one it has stacked.
  virtual auto ItemMinWidth(QWidget* item, bool /*stacked*/) const -> int {
    return std::max(item->minimumSizeHint().width(), item->minimumWidth());
  }

  void resizeEvent(QResizeEvent* event) override {
    QWidget::resizeEvent(event);
    // Side by side only while every item fits at its own minimum.
    int needed = 0;
    const auto items = Items();
    for (auto* w : items) needed += ItemMinWidth(w, false);
    needed += SideBySideSpacing() * std::max<int>(0, items.size() - 1);

    const bool stacked = event->size().width() < std::max(stack_below_, needed);
    if (stacked == stacked_) return;
    stacked_ = stacked;
    box_->setDirection(stacked ? QBoxLayout::TopToBottom
                               : QBoxLayout::LeftToRight);
    OnStackChanged(stacked);
    updateGeometry();
  }

 private:
  int stack_below_;
  QBoxLayout* box_;
  bool stacked_ = false;

  [[nodiscard]] auto Items() const -> QList<QWidget*> {
    QList<QWidget*> items;
    for (int i = 0; i < box_->count(); ++i) {
      if (auto* w = box_->itemAt(i)->widget(); w != nullptr) items << w;
    }
    return items;
  }
};

/**
 * @brief One labelled field of a report card.
 *
 * The label column has a fixed width, whatever the label says; a longer
 * label wraps inside it. Narrow, the label sits above its value.
 */
class CardFieldRow final : public StackingRow {
 public:
  static constexpr int kStackBelow = 360;
  static constexpr int kSpacing = 10;

  CardFieldRow(QLabel* key, QWidget* value, int key_width, QWidget* parent)
      : StackingRow(kStackBelow, parent), key_(key), key_width_(key_width) {
    setObjectName(u"InfoBoardCardField"_s);
    Box()->setSpacing(kSpacing);
    key_->setParent(this);
    value->setParent(this);
    key_->setFixedWidth(key_width_);
    Box()->addWidget(key_, 0, Qt::AlignTop);
    Box()->addWidget(value, 100);
    // Whatever a value capped at its reading width leaves goes here, so the
    // pair stays at the left instead of drifting apart.
    Box()->addStretch(1);
  }

 protected:
  auto SideBySideSpacing() const -> int override { return kSpacing; }

  auto ItemMinWidth(QWidget* item, bool stacked) const -> int override {
    // Side by side the label takes its column; stacked, only its words.
    if (item == key_) {
      return stacked ? key_->fontMetrics().averageCharWidth() * 8 : key_width_;
    }
    return StackingRow::ItemMinWidth(item, stacked);
  }

  void OnStackChanged(bool stacked) override {
    Box()->setSpacing(stacked ? 1 : kSpacing);
    if (stacked) {
      key_->setMinimumWidth(0);
      key_->setMaximumWidth(QWIDGETSIZE_MAX);
    } else {
      key_->setFixedWidth(key_width_);
    }
  }

 private:
  QLabel* key_;
  int key_width_;
};

/**
 * @brief A report card's header: icon and title on the left, the source on
 * the right.
 *
 * The source gets at most kMaxShare of the row, middle-elided beyond that.
 * When the title would not keep its own width beside even that much, the
 * source moves under the title, shown in full, instead of being cut to
 * nothing.
 */
class CardHeaderRow final : public StackingRow {
 public:
  static constexpr double kMaxShare = 0.4;
  static constexpr int kSpacing = 12;

  CardHeaderRow(QWidget* heading, CardSourceLabel* source, QWidget* parent)
      : StackingRow(0, parent), heading_(heading), source_(source) {
    setObjectName(u"InfoBoardCardHeader"_s);
    Box()->setSpacing(kSpacing);
    heading_->setParent(this);
    Box()->addWidget(heading_, 1);
    if (source_ != nullptr) {
      source_->setParent(this);
      source_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
      Box()->addWidget(source_, 0);
    }
  }

 protected:
  auto SideBySideSpacing() const -> int override { return kSpacing; }

  auto ItemMinWidth(QWidget* item, bool stacked) const -> int override {
    if (stacked) return StackingRow::ItemMinWidth(item, stacked);
    // Side by side the title keeps its natural width, and the source needs
    // as much of itself as its share allows.
    if (item == heading_) return heading_->sizeHint().width();
    if (item == source_) {
      return std::min(source_->FullWidth(),
                      static_cast<int>(width() * kMaxShare));
    }
    return StackingRow::ItemMinWidth(item, stacked);
  }

  void resizeEvent(QResizeEvent* event) override {
    if (source_ != nullptr && !IsStacked()) {
      source_->setMaximumWidth(
          static_cast<int>(event->size().width() * kMaxShare));
    }
    StackingRow::resizeEvent(event);
  }

  void OnStackChanged(bool stacked) override {
    Box()->setSpacing(stacked ? 2 : kSpacing);
    if (source_ == nullptr) return;
    source_->setAlignment((stacked ? Qt::AlignLeft : Qt::AlignRight) |
                          Qt::AlignVCenter);
    if (stacked) source_->setMaximumWidth(QWIDGETSIZE_MAX);
  }

 private:
  QWidget* heading_;
  CardSourceLabel* source_;
};

/// How a report's seal looks: what it says about the outcome, never more.
// What a seal says happened, and how it is painted: the core's, which the
// phone's report shares.
using GpgFrontend::ReportSealKind;

/// A document heading's double rule: two hairlines, close together.
class DoubleRule final : public QWidget {
 public:
  explicit DoubleRule(QWidget* parent) : QWidget(parent) {
    setObjectName(u"InfoBoardDoubleRule"_s);
    setFixedHeight(4);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  }

 protected:
  void paintEvent(QPaintEvent* /*event*/) override {
    const auto colors = ReportColors::From(palette());
    QPainter p(this);
    p.setPen(QPen(colors.border, 1));
    p.drawLine(0, 0, width(), 0);
    QColor second = colors.border;
    second.setAlpha(150);
    p.setPen(QPen(second, 1));
    p.drawLine(0, 3, width(), 3);
  }
};

/**
 * @brief The report's seal: GF branding and the report's outcome.
 *
 * Continuous concentric rings in a muted shade of the outcome's colour, the
 * result's symbol at the centre (a check, a cross, or an exclamation mark
 * for anything in between), "GPGFRONTEND" along the top of the band and the
 * exact outcome (e.g. CANCELLED) along the bottom. It is the report's own
 * branding: no accreditation, no signature. The lettering is drawn only
 * where it has the pixels to be legible; the outcome is always written out
 * beside the seal as well.
 *
 * It sizes itself from the document's width: full, smaller without
 * lettering, or hidden on a narrow panel, before any text would shrink.
 */
class ReportSeal final : public QWidget {
 public:
  static constexpr int kFull = 78;
  static constexpr int kCompact = 56;
  /// The cancellation waves left of the seal, at full and compact size.
  static constexpr int kFullWaves = 58;
  static constexpr int kCompactWaves = 30;
  static constexpr int kFullFrom = 560;     ///< document width
  static constexpr int kCompactFrom = 400;  ///< document width

  ReportSeal(QWidget* document, QWidget* parent)
      : QWidget(parent), document_(document) {
    setObjectName(u"InfoBoardSeal"_s);
    setFixedSize(kFull + kFullWaves, kFull);
    document_->installEventFilter(this);
  }

  void SetSeal(ReportSealKind kind, const QColor& accent,
               const QString& inscription, const QString& tooltip) {
    kind_ = kind;
    accent_ = accent;
    inscription_ = inscription;
    setToolTip(tooltip);
    update();
  }

  [[nodiscard]] auto Kind() const -> ReportSealKind { return kind_; }
  [[nodiscard]] auto Inscription() const -> QString { return inscription_; }

 protected:
  auto eventFilter(QObject* watched, QEvent* event) -> bool override {
    if (watched == document_ && event->type() == QEvent::Resize) {
      const int w = document_->width();
      const int d = w >= kFullFrom ? kFull : w >= kCompactFrom ? kCompact : 0;
      setVisible(d > 0);
      // The waves shorten with the seal, and go with it.
      const int waves = d == kFull ? kFullWaves : kCompactWaves;
      if (d > 0) setFixedSize(d + waves, d);
    }
    return QWidget::eventFilter(watched, event);
  }

  void paintEvent(QPaintEvent* /*event*/) override {
    // Lettering and ornament need real pixels; below this the seal keeps
    // only its contours and symbol.
    constexpr double kDetailFromPx = 70.0;

    QPainter p(this);
    // The device scale (the seal's tilt does not change it).
    const auto& t = p.deviceTransform();
    const double scale = std::hypot(t.m11(), t.m12());
    const auto colors = ReportColors::From(palette());
    PaintReportSeal(p, rect(), kind_, inscription_,
                    {accent_, colors.text, colors.muted, colors.dark, font()},
                    height() * scale >= kDetailFromPx);
  }

 private:
  QWidget* document_;
  ReportSealKind kind_ = ReportSealKind::kINFORMATION;
  QColor accent_;
  QString inscription_;
};

}  // namespace GpgFrontend::UI
