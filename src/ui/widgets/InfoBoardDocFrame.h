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
    const int w = wa + wb;
    return {(a.red() * wa + b.red() * wb) / w,
            (a.green() * wa + b.green() * wb) / w,
            (a.blue() * wa + b.blue() * wb) / w};
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
enum class ReportSealKind : std::uint8_t {
  kSUCCESS,
  kWARNING,
  kCANCELLED,
  kFAILURE,
  kINFORMATION,
};

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
    constexpr double kPi = 3.14159265358979323846;
    // Lettering and ornament need real pixels; below this the seal keeps
    // only its contours and symbol.
    constexpr double kDetailFromPx = 70.0;
    // Stamped, not placed: a small fixed counterclockwise tilt, the same in
    // every render. The whole seal turns as one; a circle keeps its bounds,
    // so nothing clips.
    constexpr double kTiltDegrees = -8.0;

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    // The seal is the right-hand square; the waves fill what is left of it.
    const double side = height();
    const QRectF seal_rect(width() - side, 0.0, side, side);
    p.translate(seal_rect.center());
    p.rotate(kTiltDegrees);
    p.translate(-seal_rect.center());

    const auto colors = ReportColors::From(palette());
    const QRectF box = seal_rect.adjusted(2.0, 2.0, -2.0, -2.0);
    const QPointF c = box.center();
    const double r = box.width() / 2.0;
    // The device scale, unaffected by the tilt (m11 alone would be cos-scaled).
    const auto& t = p.deviceTransform();
    const double scale = std::hypot(t.m11(), t.m12());
    const bool detail = side * scale >= kDetailFromPx;

    // The ink: the outcome's colour, a little muted; the symbol stronger.
    const QColor ink = kind_ == ReportSealKind::kINFORMATION
                           ? colors.muted
                           : ReportColors::Blend(accent_, colors.muted, 4, 1);
    const QColor symbol_ink = symbol_color(colors);

    // Shallow relief: a pale edge up-left, a faint shade down-right, the
    // stroke itself graded slightly within its own colour.
    const QColor lift =
        colors.dark ? QColor(255, 255, 255, 30) : QColor(255, 255, 255, 200);
    const QColor shade =
        colors.dark ? QColor(0, 0, 0, 90)
                    : QColor(ink.darker(170).red(), ink.darker(170).green(),
                             ink.darker(170).blue(), 55);
    const auto graded = [&](const QColor& base) -> QBrush {
      QLinearGradient g(box.topLeft(), box.bottomRight());
      g.setColorAt(0.0, base.lighter(colors.dark ? 118 : 112));
      g.setColorAt(1.0, base.darker(colors.dark ? 104 : 112));
      return {g};
    };
    const auto relief = [&](const auto& draw, double width, const QColor& base,
                            Qt::PenCapStyle cap) {
      const double d = std::max(0.4, width * 0.28);
      QPen pen(shade, width, Qt::SolidLine, cap, Qt::RoundJoin);
      p.setPen(pen);
      draw(QPointF(d, d));
      pen.setColor(lift);
      p.setPen(pen);
      draw(QPointF(-d * 0.8, -d * 0.8));
      pen.setBrush(graded(base));
      p.setPen(pen);
      draw(QPointF(0, 0));
    };
    const auto ring = [&](double radius, double width) {
      p.setBrush(Qt::NoBrush);
      relief([&](const QPointF& o) { p.drawEllipse(c + o, radius, radius); },
             width, ink, Qt::FlatCap);
    };

    // Cancellation waves: five short parallel lines running left from the
    // seal and slipping a little under its edge, as a postmark's would.
    // Drawn first, so the seal's rings lie over them; thin and muted, so
    // the seal stays the subject.
    {
      const double start = 3.0;  // the widget's own left, inside its bounds
      const double end = c.x() - r * 0.72;  // a slight overlap
      const double amplitude = std::max(0.8, r * 0.055);
      const double wavelength = std::max(8.0, r * 0.52);
      QColor wave_ink = ink;
      // Translucent, so always lighter than the seal's solid outer contour.
      wave_ink.setAlpha(colors.dark ? 185 : 170);
      QPen wave_pen(wave_ink, std::max(1.2, r * 0.042), Qt::SolidLine,
                    Qt::RoundCap, Qt::RoundJoin);
      p.setPen(wave_pen);
      p.setBrush(Qt::NoBrush);
      // Evenly spaced and centred on the seal's horizontal axis.
      constexpr int kWaves = 5;
      const double spacing = r * 0.20;
      for (int line = 0; line < kWaves; ++line) {
        const double dy = (line - (kWaves - 1) / 2.0) * spacing;
        QPainterPath wave;
        constexpr int kSteps = 48;
        for (int i = 0; i <= kSteps; ++i) {
          const double x = start + (end - start) * i / kSteps;
          const double y =
              c.y() + dy +
              amplitude * std::sin(2.0 * kPi * (x - start) / wavelength);
          if (i == 0) {
            wave.moveTo(x, y);
          } else {
            wave.lineTo(x, y);
          }
        }
        p.drawPath(wave);
      }
    }

    // Nearly transparent: the seal belongs to the paper.
    QColor wash = ink;
    wash.setAlpha(colors.dark ? 16 : 10);
    p.setPen(Qt::NoPen);
    p.setBrush(wash);
    p.drawEllipse(c, r, r);

    // Contours: a strong outer line with a fine one just inside it, and the
    // band's fine inner edge.
    const double outer_w = std::max(1.6, r * 0.065);
    const double fine_w = std::max(0.7, r * 0.018);
    const double outer_r = r - outer_w / 2.0;
    const double fine_r = outer_r - outer_w / 2.0 - std::max(1.6, r * 0.05);
    const double band_inner = detail ? r * 0.64 : r * 0.80;
    ring(outer_r, outer_w);
    ring(fine_r, fine_w);
    ring(band_inner, fine_w);

    if (detail) {
      // The band: the brand along the top, the exact outcome along the
      // bottom, a small diamond where they meet on either side.
      QFont font = this->font();
      font.setPixelSize(std::max(6, static_cast<int>(std::lround(r * 0.16))));
      font.setBold(true);
      font.setLetterSpacing(QFont::AbsoluteSpacing, 0.5);
      p.setFont(font);
      const QFontMetricsF fm(font);
      const double text_r = (fine_r - fine_w + band_inner + fine_w) / 2.0;
      p.setPen(ink);
      const auto arc = [&](const QString& text, bool bottom) {
        const double total = fm.horizontalAdvance(text);
        const double span = total / text_r * 180.0 / kPi;
        double angle = bottom ? 90.0 + span / 2.0 : -90.0 - span / 2.0;
        for (const QChar ch : text) {
          const double adv = fm.horizontalAdvance(ch);
          const double step = adv / text_r * 180.0 / kPi;
          const double mid = bottom ? angle - step / 2.0 : angle + step / 2.0;
          p.save();
          p.translate(c);
          if (bottom) {
            p.rotate(mid - 90.0);
            p.drawText(QPointF(-adv / 2.0, text_r + fm.capHeight() / 2.0), ch);
          } else {
            p.rotate(mid + 90.0);
            p.drawText(QPointF(-adv / 2.0, -text_r + fm.capHeight() / 2.0), ch);
          }
          p.restore();
          angle += bottom ? -step : step;
        }
      };
      arc(u"GPGFRONTEND"_s, false);
      arc(inscription_, true);

      p.setPen(Qt::NoPen);
      p.setBrush(ink);
      const double dia = std::max(1.2, r * 0.045);
      for (const double side : {-1.0, 1.0}) {
        const QPointF at = c + QPointF(side * text_r, 0.0);
        QPolygonF diamond{at + QPointF(0, -dia), at + QPointF(dia, 0),
                          at + QPointF(0, dia), at + QPointF(-dia, 0)};
        p.drawPolygon(diamond);
      }

      // Fine radial ticks just inside the band, clear of the symbol.
      QColor tick = ink;
      tick.setAlpha(150);
      p.setPen(
          QPen(tick, std::max(0.6, r * 0.012), Qt::SolidLine, Qt::FlatCap));
      constexpr int kTicks = 48;
      const double t0 = band_inner - fine_w;
      const double t1 = t0 - std::max(1.5, r * 0.045);
      for (int i = 0; i < kTicks; ++i) {
        const double a = 2.0 * kPi * i / kTicks;
        const QPointF dir(std::cos(a), std::sin(a));
        p.drawLine(c + dir * t0, c + dir * t1);
      }
    }

    // The symbol: bold, optically centred, well clear of the rings.
    const double k = band_inner * (detail ? 0.46 : 0.50);
    const double sw = std::max(2.2, band_inner * 0.17);
    QPainterPath symbol;
    QPointF dot;
    bool has_dot = false;
    switch (kind_) {
      case ReportSealKind::kSUCCESS:
        // A check sits low-left; nudged right and up to look centred.
        symbol.moveTo(c + QPointF(-0.74 * k, 0.04 * k));
        symbol.lineTo(c + QPointF(-0.20 * k, 0.56 * k));
        symbol.lineTo(c + QPointF(0.80 * k, -0.50 * k));
        break;
      case ReportSealKind::kFAILURE:
        symbol.moveTo(c + QPointF(-0.58 * k, -0.58 * k));
        symbol.lineTo(c + QPointF(0.58 * k, 0.58 * k));
        symbol.moveTo(c + QPointF(0.58 * k, -0.58 * k));
        symbol.lineTo(c + QPointF(-0.58 * k, 0.58 * k));
        break;
      case ReportSealKind::kINFORMATION:
        // Neutral: an "i", a dot over a stem.
        symbol.moveTo(c + QPointF(0, -0.15 * k));
        symbol.lineTo(c + QPointF(0, 0.75 * k));
        dot = c + QPointF(0, -0.68 * k);
        has_dot = true;
        break;
      default:
        // Warning, cancellation, an unsupported operation: "!".
        symbol.moveTo(c + QPointF(0, -0.78 * k));
        symbol.lineTo(c + QPointF(0, 0.30 * k));
        dot = c + QPointF(0, 0.80 * k);
        has_dot = true;
        break;
    }
    p.setBrush(Qt::NoBrush);
    relief([&](const QPointF& o) { p.drawPath(symbol.translated(o)); }, sw,
           symbol_ink, Qt::RoundCap);
    if (has_dot) {
      const double dr = sw * 0.62;
      const auto draw_dot = [&](const QColor& color, const QPointF& o) {
        p.setPen(Qt::NoPen);
        p.setBrush(color);
        p.drawEllipse(dot + o, dr, dr);
      };
      const double d = std::max(0.4, sw * 0.28);
      draw_dot(shade, QPointF(d, d));
      draw_dot(lift, QPointF(-d * 0.8, -d * 0.8));
      draw_dot(symbol_ink, QPointF(0, 0));
    }
  }

 private:
  /// The symbol in the outcome's own colour, stronger than the rings.
  [[nodiscard]] auto symbol_color(const ReportColors& colors) const -> QColor {
    return kind_ == ReportSealKind::kINFORMATION
               ? colors.muted
               : ReportColors::Blend(accent_, colors.text, 6, 1);
  }

  QWidget* document_;
  ReportSealKind kind_ = ReportSealKind::kINFORMATION;
  QColor accent_;
  QString inscription_;
};

}  // namespace GpgFrontend::UI
