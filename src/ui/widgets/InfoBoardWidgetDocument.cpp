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

#include "ui/function/AppearanceFont.h"
#include "ui/widgets/InfoBoardDocFrame.h"
#include "ui/widgets/InfoBoardWidget.h"
#include "ui_InfoBoard.h"

namespace GpgFrontend::UI {

namespace {

// Card spacing, in logical pixels.
// Compact: the Status Panel is a small dock, and dense cards read better
// there than airy ones. Readability comes from the body size, not space.
constexpr int kCardPaddingH = 11;
constexpr int kCardPaddingV = 8;
constexpr int kCardAccent = 2;  ///< the status edge on a card's left
constexpr int kFieldSpacing = 5;
constexpr int kHeaderGap = 1;   ///< added to kFieldSpacing under the header
constexpr int kCardGap = 7;     ///< between cards
constexpr int kSectionGap = 8;  ///< between the document's sections
/// Added between the runs of a card's fields (what happened, why, next).
constexpr int kFieldGroupGap = 4;

// One body size for everything meant to be read; hierarchy comes from
// weight and colour. The outcome row is the one larger level. Only
// monospaced technical text (hashes, codes, identifiers) is a pixel step
// smaller, because the monospaced face runs wider.
auto PrimaryFont(bool bold) -> QFont { return ReportUiFont(1.25, bold); }
auto HeadingFont() -> QFont { return ReportUiFont(1.0, true); }
auto BodyFont() -> QFont { return ReportUiFont(); }
auto MutedFont(bool bold = false) -> QFont { return ReportUiFont(1.0, bold); }
auto TechnicalFont() -> QFont {
  const int body = ReportBodyPixelSize();
  return ReportMonoFont(static_cast<double>(std::max(body - 1, 1)) / body);
}

/// Width of the field label column, the same in every card whatever the
/// labels say (a longer one wraps inside it). In characters, so it follows
/// the font size.
constexpr int kFieldLabelChars = 15;
/// Prose and diagnostics stop at a comfortable reading width on a wide panel.
constexpr int kReadingWidthChars = 90;

/// A long unbroken identifier (a digest, a fingerprint) in groups of eight,
/// so it can wrap on a narrow panel. Shown only: the exact value is the
/// tooltip, and Copy and every export carry it as it is.
auto GroupedForDisplay(const QString& value) -> QString {
  constexpr int kGroup = 8;
  constexpr int kLongToken = 24;
  if (value.size() <= kLongToken ||
      std::any_of(value.cbegin(), value.cend(),
                  [](QChar c) { return c.isSpace(); })) {
    return value;
  }
  QStringList groups;
  for (qsizetype i = 0; i < value.size(); i += kGroup) {
    groups << value.mid(i, kGroup);
  }
  return groups.join(u' ');
}

auto FieldLabelWidth() -> int {
  return QFontMetrics(MutedFont()).averageCharWidth() * kFieldLabelChars;
}

auto ReadingWidth() -> int {
  return QFontMetrics(BodyFont()).averageCharWidth() * kReadingWidthChars;
}

/// Items side by side at their natural widths, left-aligned; stacked when
/// they do not fit on one line.
class FlowRow final : public StackingRow {
 public:
  FlowRow(const QList<QWidget*>& items, int spacing, QWidget* parent)
      : StackingRow(0, parent), spacing_(spacing) {
    Box()->setSpacing(spacing_);
    for (auto* item : items) {
      item->setParent(this);
      Box()->addWidget(item, 0, Qt::AlignBaseline);
    }
    Box()->addStretch(1);
  }

 protected:
  auto SideBySideSpacing() const -> int override { return spacing_; }

  auto ItemMinWidth(QWidget* item, bool stacked) const -> int override {
    if (stacked) return StackingRow::ItemMinWidth(item, stacked);
    // Side by side only while each label fits on one line; a wrapping
    // label's size hint is not its full width.
    if (auto* label = qobject_cast<QLabel*>(item)) {
      return label->fontMetrics().horizontalAdvance(label->text()) + 2;
    }
    return item->sizeHint().width();
  }

  void OnStackChanged(bool stacked) override {
    Box()->setSpacing(stacked ? 0 : spacing_);
  }

 private:
  int spacing_;
};

/// The footer: reference, hash and issue time side by side, stacked and
/// left-aligned when there is no room for one line.
class FooterRow final : public StackingRow {
 public:
  static constexpr int kStackBelow = 0;

  FooterRow(const QList<QPair<QLabel*, Qt::Alignment>>& items, QWidget* parent)
      : StackingRow(kStackBelow, parent), items_(items) {
    setObjectName(u"InfoBoardFooter"_s);
    Box()->setSpacing(8);
    for (const auto& [label, alignment] : items_) {
      label->setParent(this);
      label->setAlignment(alignment | Qt::AlignVCenter);
      Box()->addWidget(label, 1);
    }
  }

 protected:
  void OnStackChanged(bool stacked) override {
    Box()->setSpacing(stacked ? 2 : 8);
    for (const auto& [label, alignment] : items_) {
      label->setAlignment((stacked ? Qt::AlignLeft : alignment) |
                          Qt::AlignVCenter);
    }
  }

 private:
  QList<QPair<QLabel*, Qt::Alignment>> items_;
};

}  // namespace

auto InfoBoardWidget::build_card_stylesheet(const QColor& color) const
    -> QString {
  // A neutral card with a fine border and small corners; the status shows
  // only as a narrow, muted accent on the left.
  const auto colors = ReportColors::From(palette());
  const auto accent = ReportColors::Blend(color, colors.border, 2, 1);
  return u"#DocResultRow { background-color: %1; border: 1px solid %2;"
         " border-left: %4px solid %3; border-radius: 3px; }"_s
             .arg(colors.surface.name(), colors.border.name(), accent.name())
             .arg(kCardAccent);
}

auto InfoBoardWidget::create_card(QWidget* parent, InfoBoardStatus status) const
    -> QFrame* {
  const QColor ic = StatusColor(status);
  auto* card = new QFrame(parent);
  card->setObjectName(u"DocResultRow"_s);
  card->setStyleSheet(build_card_stylesheet(ic));
  auto* cl = new QVBoxLayout(card);
  cl->setContentsMargins(kCardPaddingH, kCardPaddingV, kCardPaddingH,
                         kCardPaddingV);
  cl->setSpacing(kFieldSpacing);
  return card;
}

void InfoBoardWidget::setup_status_page_layout(QVBoxLayout* page_layout) {
  page_layout->setContentsMargins(0, 0, 0, 0);
  page_layout->setSpacing(8);

  placeholder_label_ = new QLabel(
      tr("No operation yet.\nResults will appear here as a summary document."),
      ui_->page_1);
  placeholder_label_->setAlignment(Qt::AlignCenter);
  placeholder_label_->setWordWrap(true);
  placeholder_label_->setStyleSheet(
      u"color: palette(windowText); font-style: italic;"_s);

  doc_frame_ = new DocFrame(ui_->page_1);
  doc_frame_->setObjectName(u"DocFrame"_s);

  // Text meant to be read is set in the application's own face, on the
  // report's pixel scale; hashes, codes and identifiers use the bundled
  // monospaced face (ReportFont), where columns and lookalike characters
  // matter.
  //
  // The size is pinned rather than inherited, and pinned in pixels. Inheriting
  // it meant the report took the platform's default UI point size (13pt on
  // macOS, 9pt on Windows, 10-11pt on most Linux desktops) and every label
  // stepped off that; on top of which Qt reports 72 logical DPI on macOS
  // against 96 elsewhere, so a "pt" is not even the same unit. Every label
  // below asks for its own step instead of deriving one from this font.
  const QFont doc_font = ReportUiFont();
  doc_frame_->setFont(doc_font);

  // QFontInfo reports what Qt actually resolved; QFont::family() only echoes
  // back what was asked for. A silent substitution -- CoreText falling back to
  // Menlo because the bundled family never registered, say -- looks like a
  // styling quirk on screen but is baked into every exported PNG, so say it out
  // loud once at startup.
  const QFontInfo doc_font_info(doc_font);
  LOG_D() << "report document font: requested" << doc_font.family()
          << "resolved" << doc_font_info.family() << "at"
          << doc_font_info.pixelSize() << "px";
}

void InfoBoardWidget::create_field_rows(QWidget* parent,
                                        QVBoxLayout* fields_layout) {
  fields_layout->setContentsMargins(0, 2, 0, 2);
  fields_layout->setSpacing(5);

  // The summary: one plain paragraph saying what happened.
  summary_label_ = new QLabel(parent);
  summary_label_->setObjectName(u"InfoBoardSummary"_s);
  summary_label_->setWordWrap(true);
  summary_label_->setTextFormat(Qt::PlainText);
  summary_label_->setTextInteractionFlags(Qt::TextSelectableByMouse);
  summary_label_->setFont(BodyFont());
  ApplyTone(summary_label_, ReportTone::kTEXT, palette());
  summary_label_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
  summary_label_->setMaximumWidth(ReadingWidth());
  fields_layout->addWidget(summary_label_);

  // The report's signers or recipients: the same label column as every
  // card field, with the identities as pills (sources are never pills).
  key_details_ = new QLabel(tr("DETAILS"), parent);
  key_details_->setTextFormat(Qt::PlainText);
  key_details_->setWordWrap(true);
  key_details_->setAlignment(Qt::AlignLeft | Qt::AlignTop);
  key_details_->setFont(MutedFont());
  ApplyTone(key_details_, ReportTone::kMUTED, palette());

  details_container_ = new QWidget(parent);
  details_container_->setMaximumWidth(ReadingWidth());
  auto* dc_layout = new QVBoxLayout(details_container_);
  dc_layout->setContentsMargins(0, 0, 0, 0);
  dc_layout->setSpacing(4);

  row_details_ = new CardFieldRow(key_details_, details_container_,
                                  FieldLabelWidth(), parent);
  // Indented by a card's edge and padding, so its label and value line up
  // with the fields inside the cards below.
  row_details_->setContentsMargins(kCardAccent + kCardPaddingH, 0, 0, 0);
  row_details_->setVisible(false);
  fields_layout->addWidget(row_details_);
}

void InfoBoardWidget::create_footer(QVBoxLayout* doc_layout) {
  auto* sep_bot = new QFrame(doc_frame_);
  sep_bot->setFrameShape(QFrame::HLine);
  sep_bot->setFrameShadow(QFrame::Plain);
  ApplyTone(sep_bot, ReportTone::kRULE, palette());

  // The reference and the hash are identifiers: monospaced. The time is
  // read.

  id_label_ = new QLabel(doc_frame_);
  id_label_->setTextFormat(Qt::PlainText);
  id_label_->setFont(TechnicalFont());
  ApplyTone(id_label_, ReportTone::kMUTED, palette());
  id_label_->setTextInteractionFlags(Qt::TextSelectableByMouse);

  // Shortened on screen; the tooltip, Copy and the PNG carry it in full.
  hash_label_ = new QLabel(doc_frame_);
  hash_label_->setTextFormat(Qt::PlainText);
  hash_label_->setFont(TechnicalFont());
  ApplyTone(hash_label_, ReportTone::kMUTED, palette());

  time_label_ = new QLabel(doc_frame_);
  time_label_->setTextFormat(Qt::PlainText);
  time_label_->setFont(MutedFont());
  ApplyTone(time_label_, ReportTone::kMUTED, palette());

  auto* footer_row = new FooterRow({{id_label_, Qt::AlignLeft},
                                    {hash_label_, Qt::AlignHCenter},
                                    {time_label_, Qt::AlignRight}},
                                   doc_frame_);

  doc_layout->addWidget(sep_bot);
  doc_layout->addWidget(footer_row);
}

void InfoBoardWidget::init_status_page() {
  auto* page_layout = qobject_cast<QVBoxLayout*>(ui_->page_1->layout());
  if (page_layout == nullptr) return;

  setup_status_page_layout(page_layout);

  auto* doc_layout = new QVBoxLayout(doc_frame_);
  doc_layout->setContentsMargins(kCardPaddingH, kCardPaddingV, kCardPaddingH,
                                 kCardPaddingV);
  doc_layout->setSpacing(kSectionGap);

  // Header: a certificate's heading (brand, title, a fine rule), then what
  // was requested and how it ended; the seal sits to the right of it all.
  auto* header = new QVBoxLayout();
  header->setContentsMargins(0, 0, 0, 0);
  header->setSpacing(2);

  const auto make_header_label = [this](const QFont& font) -> QLabel* {
    auto* label = new QLabel(doc_frame_);
    label->setFont(font);
    label->setWordWrap(true);
    label->setTextFormat(Qt::PlainText);
    label->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    return label;
  };

  // The identity block: the wordmark over the document's title. Formal
  // through alignment and rules, not size; the GF mark itself is the
  // paper's watermark.

  // The brand is a name, not a phrase: never translated.
  QFont brand_font = MutedFont(true);
  brand_font.setPixelSize(std::max(8, brand_font.pixelSize() - 2));
  brand_font.setLetterSpacing(QFont::AbsoluteSpacing, 1.6);
  auto* brand = make_header_label(brand_font);
  brand->setObjectName(u"InfoBoardBrand"_s);
  brand->setText(u"GPGFRONTEND"_s);
  ApplyTone(brand, ReportTone::kMUTED, palette());

  QFont title_font = BodyFont();
  title_font.setWeight(QFont::DemiBold);
  title_font.setLetterSpacing(QFont::AbsoluteSpacing, 0.6);
  header_label_ = make_header_label(title_font);
  header_label_->setObjectName(u"InfoBoardReportName"_s);
  header_label_->setText(tr("SECURITY OPERATION REPORT"));
  ApplyTone(header_label_, ReportTone::kTEXT, palette());

  auto* identity = new QVBoxLayout();
  identity->setContentsMargins(0, 0, 0, 0);
  identity->setSpacing(0);
  identity->addWidget(brand);
  identity->addWidget(header_label_);

  auto* heading_rule = new DoubleRule(doc_frame_);

  // What was requested and how it ended, on one row where it fits.
  // The operation in bold primary text, the outcome beside it in its own
  // colour; both wrap rather than shrink on a narrow panel.
  operation_label_ = make_header_label(PrimaryFont(true));
  operation_label_->setObjectName(u"InfoBoardOperation"_s);
  ApplyTone(operation_label_, ReportTone::kTEXT, palette());
  status_label_ = make_header_label(PrimaryFont(true));
  status_label_->setObjectName(u"InfoBoardStatus"_s);
  auto* outcome_row =
      new FlowRow({operation_label_, status_label_}, 12, doc_frame_);

  meta_label_ = make_header_label(MutedFont());
  meta_label_->setObjectName(u"InfoBoardMeta"_s);
  ApplyTone(meta_label_, ReportTone::kMUTED, palette());

  header->addLayout(identity);
  header->addSpacing(5);
  header->addWidget(heading_rule);
  header->addSpacing(5);
  header->addWidget(outcome_row);
  header->addWidget(meta_label_);

  seal_ = new ReportSeal(doc_frame_, doc_frame_);
  auto* header_row = new QHBoxLayout();
  header_row->setContentsMargins(0, 0, 0, 0);
  header_row->setSpacing(12);
  header_row->addLayout(header, 1);
  header_row->addWidget(seal_, 0, Qt::AlignTop | Qt::AlignRight);

  sep_top_ = new QFrame(doc_frame_);
  sep_top_->setFrameShape(QFrame::HLine);
  sep_top_->setFrameShadow(QFrame::Plain);
  ApplyTone(sep_top_, ReportTone::kRULE, palette());

  // Field rows
  auto* fields_widget = new QWidget(doc_frame_);
  auto* fields_layout = new QVBoxLayout(fields_widget);
  create_field_rows(fields_widget, fields_layout);

  extra_sep_ = new QFrame(doc_frame_);
  extra_sep_->setFrameShape(QFrame::HLine);
  extra_sep_->setFrameShadow(QFrame::Plain);
  ApplyTone(extra_sep_, ReportTone::kRULE, palette());
  extra_sep_->setVisible(false);

  extra_widget_ = new QWidget(doc_frame_);
  auto* extra_layout = new QVBoxLayout(extra_widget_);
  extra_layout->setContentsMargins(0, 0, 0, 0);
  extra_layout->setSpacing(kCardGap);
  extra_widget_->setVisible(false);

  doc_layout->addLayout(header_row);
  doc_layout->addWidget(sep_top_);
  doc_layout->addWidget(fields_widget);
  doc_layout->addWidget(extra_sep_);
  doc_layout->addWidget(extra_widget_);

  create_footer(doc_layout);

  // Wrap the report card in a top-aligned container so that, when the content
  // is shorter than the viewport, the leftover space falls outside the bordered
  // DocFrame instead of stretching it.
  auto* scroll_content = new QWidget(ui_->page_1);
  auto* scroll_layout = new QVBoxLayout(scroll_content);
  scroll_layout->setContentsMargins(0, 0, 0, 0);
  scroll_layout->setSpacing(0);
  scroll_layout->addWidget(doc_frame_);
  scroll_layout->addStretch(1);

  doc_scroll_ = new QScrollArea(ui_->page_1);
  doc_scroll_->setWidgetResizable(true);
  doc_scroll_->setFrameShape(QFrame::NoFrame);
  doc_scroll_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  doc_scroll_->setWidget(scroll_content);
  doc_scroll_->setVisible(false);

  page_layout->addWidget(placeholder_label_, 1, Qt::AlignCenter);
  page_layout->addWidget(doc_scroll_, 1);
}

void InfoBoardWidget::delete_widgets_in_layout(QLayout* layout,
                                               int start_index) {
  if (layout == nullptr) return;

  while (layout->count() > start_index) {
    QLayoutItem* item = layout->takeAt(start_index);
    if (item == nullptr) break;

    if (auto* child_layout = item->layout()) {
      delete_widgets_in_layout(child_layout, 0);
      delete child_layout;
    }

    if (auto* widget = item->widget()) {
      widget->deleteLater();
    }

    delete item;
  }
}

void InfoBoardWidget::render_card(QVBoxLayout* layout, QWidget* parent,
                                  const InfoBoardCard& card_data) {
  if (layout == nullptr || parent == nullptr) return;

  auto* card = create_card(parent, card_data.status);
  auto* cl = qobject_cast<QVBoxLayout*>(card->layout());

  // One header row for every card: what it is on the left, what it is about
  // on the right.
  // The status shows as the accent and a small icon; the title itself is
  // ordinary text, so the report's own status stays the loudest thing.
  auto* heading = new QWidget(card);
  auto* hl = new QHBoxLayout(heading);
  hl->setContentsMargins(0, 0, 0, 0);
  hl->setSpacing(6);
  auto* icon = new QLabel(heading);
  icon->setFixedSize(14, 14);
  icon->setPixmap(
      QPixmap(StatusIconPath(card_data.status))
          .scaled(12, 12, Qt::KeepAspectRatio, Qt::SmoothTransformation));
  auto* title = new QLabel(card_data.title, heading);
  title->setObjectName(u"InfoBoardCardTitle"_s);
  title->setTextFormat(Qt::PlainText);
  title->setWordWrap(true);
  // Weight through the font rather than the stylesheet: every other label in
  // the card uses QFont::setBold, and a QSS rule carrying a font property
  // resolves down a different path into the font database -- and can drop the
  // pixel size the document set. The stylesheet keeps only the colour.
  title->setFont(HeadingFont());
  ApplyTone(title, ReportTone::kTEXT, palette());
  hl->addWidget(icon, 0, Qt::AlignVCenter);
  hl->addWidget(title, 1);

  CardSourceLabel* source = nullptr;
  if (!card_data.source.IsEmpty()) {
    source = new CardSourceLabel(card_data.source.full.isEmpty()
                                     ? card_data.source.label
                                     : card_data.source.full,
                                 card);
    source->setFont(MutedFont());
    ApplyTone(source, ReportTone::kMUTED, palette());
  }
  cl->addWidget(new CardHeaderRow(heading, source, card));
  cl->addSpacing(kHeaderGap);

  // Fields: one shared two-column layout. The label column has a fixed
  // width; values take the rest and wrap without limit. Runs of fields
  // (what happened, why, what next) are set slightly apart.
  std::optional<InfoBoardFieldGroup> previous_group;
  for (qsizetype i = 0; i < card_data.fields.size(); ++i) {
    const auto& field = card_data.fields.at(i);
    // Skip fields with no value so the card stays compact, and those the
    // summary or headline already says (the text exports keep them).
    if (field.second.isEmpty()) continue;
    const auto meta = card_data.MetaAt(i);
    if (meta.summarized) continue;

    if (previous_group.has_value() && *previous_group != meta.group) {
      cl->addSpacing(kFieldGroupGap);
    }
    previous_group = meta.group;

    auto* kl = new QLabel(field.first, card);
    kl->setTextFormat(Qt::PlainText);
    // Labels are a step quieter than the values they name.
    kl->setFont(MutedFont());
    ApplyTone(kl, ReportTone::kMUTED, palette());
    kl->setWordWrap(true);
    kl->setAlignment(Qt::AlignLeft | Qt::AlignTop);

    auto* value_box = new QWidget(card);
    // A comfortable measure on a wide panel; it never limits the height.
    value_box->setMaximumWidth(ReadingWidth());
    auto* vbl = new QVBoxLayout(value_box);
    vbl->setContentsMargins(0, 0, 0, 0);
    vbl->setSpacing(1);

    // Values are never elided: an engine's message must be readable in full,
    // and selectable to search for.
    auto* vl = new QLabel(
        meta.technical ? GroupedForDisplay(field.second) : field.second,
        value_box);
    if (meta.technical && vl->text() != field.second) {
      vl->setToolTip(field.second);
    }
    vl->setProperty("fieldValue", field.second);
    vl->setObjectName(u"InfoBoardCardValue"_s);
    vl->setProperty("fieldKey", field.first);
    vl->setTextFormat(Qt::PlainText);
    vl->setWordWrap(true);
    vl->setTextInteractionFlags(Qt::TextSelectableByMouse);
    vl->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    // Technical identifiers stay compact and quiet; prose is body text.
    vl->setFont(meta.technical ? TechnicalFont() : BodyFont());
    ApplyTone(vl, meta.technical ? ReportTone::kTECHNICAL : ReportTone::kTEXT,
              palette());
    vbl->addWidget(vl);

    if (!meta.note.isEmpty()) {
      // What the value belongs to (a stage, a code, a path): secondary.
      auto* nl = new QLabel(meta.note, value_box);
      nl->setObjectName(u"InfoBoardCardNote"_s);
      nl->setTextFormat(Qt::PlainText);
      nl->setWordWrap(true);
      nl->setTextInteractionFlags(Qt::TextSelectableByMouse);
      nl->setFont(TechnicalFont());
      ApplyTone(nl, ReportTone::kMUTED, palette());
      vbl->addWidget(nl);
    }

    cl->addWidget(new CardFieldRow(kl, value_box, FieldLabelWidth(), card));
  }

  layout->addWidget(card);
}

void InfoBoardWidget::add_expander(QVBoxLayout* layout, QWidget* parent,
                                   const QString& text,
                                   const QString& action_text,
                                   const std::function<void()>& on_click) {
  // A text line of the document, with its action beside it: it reads the
  // same in an exported image as on screen.
  auto* row = new QWidget(parent);
  row->setObjectName(u"InfoBoardExpander"_s);
  auto* rl = new QHBoxLayout(row);
  rl->setContentsMargins(0, 0, 0, 0);
  rl->setSpacing(8);

  auto* label = new QLabel(text, row);
  label->setTextFormat(Qt::PlainText);
  label->setWordWrap(true);
  label->setFont(MutedFont());
  ApplyTone(label, ReportTone::kMUTED, palette());

  auto* button = new QToolButton(row);
  button->setText(action_text);
  button->setAutoRaise(true);
  button->setCursor(Qt::PointingHandCursor);
  button->setFont(MutedFont(true));
  // Queued: the click re-renders the very layout this button lives in.
  connect(button, &QToolButton::clicked, this, [this, on_click]() {
    QMetaObject::invokeMethod(this, on_click, Qt::QueuedConnection);
  });

  rl->addWidget(label, 1);
  rl->addWidget(button, 0, Qt::AlignVCenter);
  layout->addWidget(row);
}

}  // namespace GpgFrontend::UI
