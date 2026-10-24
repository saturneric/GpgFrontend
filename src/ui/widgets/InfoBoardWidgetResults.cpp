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

#include "core/utils/CommonUtils.h"
#include "ui/function/AppearanceFont.h"
#include "ui/function/SecureWipe.h"
#include "ui/widgets/InfoBoardDocFrame.h"
#include "ui/widgets/InfoBoardWidget.h"
#include "ui_InfoBoard.h"

// Every posting ends here: the setters normalize what they were given into an
// InfoBoardReport, and this file renders that one model. There is no other
// rendering path, so the same result always looks the same.

namespace GpgFrontend::UI {

void InfoBoardWidget::SetInfoBoardWithOpInfo(const QString& text,
                                             InfoBoardStatus /*status*/,
                                             const GpgOpResultInfo& info) {
  // The result's own facts decide its status; the caller's summary of them
  // is not a second opinion.
  GpgOperaResult result(info);
  result.input_index = 0;
  auto report = BuildReportFromResults({result});
  if (!text.trimmed().isEmpty() && report.objects.size() == 1 &&
      report.objects.first().raw_report.trimmed().isEmpty()) {
    report.objects.first().raw_report = text.trimmed();
  }
  // An op-info record is itself a result, whatever the text says.
  post_report(std::move(report), true);
}

void InfoBoardWidget::SetInfoBoardFromResults(
    const QString& text, InfoBoardStatus overall_status,
    const QList<GpgOperaResult>& results) {
  if (results.isEmpty()) {
    SetInfoBoard(text, overall_status);
    return;
  }
  post_report(BuildReportFromResults(results), true);
}

void InfoBoardWidget::SetInfoBoardReport(InfoBoardReport report) {
  const bool meaningful = !report.IsEmpty();
  post_report(std::move(report), meaningful);
}

void InfoBoardWidget::post_report(InfoBoardReport report, bool meaningful) {
  FinalizeReport(report, CurrentDateTime());
  // Normalization never logs; the one place a posting is accepted does,
  // once per posting.
  for (const auto& note : report.notes) {
    LOG_W() << "status report:" << note;
  }

  WipeString(current_copy_text_);
  current_report_ = std::move(report);
  show_all_groups_ = false;
  expanded_groups_.clear();

  set_info_board_text(RenderRawDetails(current_report_),
                      current_report_.severity);
  render_report();
  UpdateActionButtons();

  if (meaningful) emit SignalResultPosted(current_report_.severity);
}

auto InfoBoardWidget::IsReportCollapsed() const -> bool {
  const auto& objects = current_report_.objects;
  if (!show_all_groups_ && objects.size() > StyleConstants::kMaxShownGroups) {
    return true;
  }
  const auto shown = show_all_groups_
                         ? objects.size()
                         : std::min<qsizetype>(objects.size(),
                                               StyleConstants::kMaxShownGroups);
  for (qsizetype i = 0; i < shown; ++i) {
    const auto& object = objects[i];
    if (object.cards.size() - 1 > StyleConstants::kMaxShownDomainCards &&
        !expanded_groups_.contains(object.source.id)) {
      return true;
    }
  }
  return false;
}

void InfoBoardWidget::render_report() {
  if (doc_frame_ == nullptr) return;

  const auto& report = current_report_;
  if (report.IsEmpty()) {
    reset_document_view();
    return;
  }

  const auto status = report.severity;
  const QColor color = StatusColor(status);
  const QString accent = color.name();

  // The document: a fine neutral frame with small corners (its inner rule
  // is painted by the frame), on a plain paper or charcoal surface. Status
  // colour is kept for the outcome, the seal and small accents.
  const auto colors = ReportColors::From(palette());
  const QColor paper = colors.dark
                           ? palette().color(QPalette::Window).lighter(108)
                           : palette().color(QPalette::Base).darker(101);
  doc_frame_->setStyleSheet(
      u"#DocFrame { border: 1px solid %1; border-radius: 3px;"
      " background-color: %2; }"_s.arg(colors.border.name(), paper.name()));

  // The seal: GF branding with the outcome's mark. Never an approval for
  // anything that did not succeed, and never a claim of a signature.
  const auto seal_kind = [&]() -> ReportSealKind {
    switch (report.headline) {
      case InfoBoardHeadline::kSUCCEEDED:
        return ReportSealKind::kSUCCESS;
      case InfoBoardHeadline::kCANCELLED:
        return ReportSealKind::kCANCELLED;
      case InfoBoardHeadline::kFAILED:
      case InfoBoardHeadline::kPARTIALLY_FAILED:
        return ReportSealKind::kFAILURE;
      case InfoBoardHeadline::kINFORMATION:
        return ReportSealKind::kINFORMATION;
      default:
        return ReportSealKind::kWARNING;
    }
  }();
  // Green only for a success, red only for a failure; everything in between
  // (warnings, cancellations, an unsupported operation) is amber.
  const QColor seal_color = seal_kind == ReportSealKind::kSUCCESS
                                ? StatusColor(kINFO_ERROR_OK)
                            : seal_kind == ReportSealKind::kFAILURE
                                ? StatusColor(kINFO_ERROR_CRITICAL)
                                : StatusColor(kINFO_ERROR_WARN);
  seal_->SetSeal(seal_kind, seal_color, HeadlineText(report.headline).toUpper(),
                 tr("Report status: %1. This seal is GpgFrontend's report "
                    "branding; it is not a signature.")
                     .arg(HeadlineText(report.headline)));

  // Header: what was requested, then how it ended, then the quiet details.
  operation_label_->setText(report.operation_label);
  operation_label_->setVisible(!report.operation_label.isEmpty());
  status_label_->setText(
      build_status_symbol(status, HeadlineText(report.headline)));
  status_label_->setStyleSheet(u"color: %1;"_s.arg(accent));
  QStringList meta;
  for (const auto& part : {report.count_summary, report.engine}) {
    if (!part.isEmpty()) meta << part;
  }
  meta_label_->setText(meta.join(u"  ·  "_s));
  meta_label_->setVisible(!meta.isEmpty());

  summary_label_->setText(report.summary);
  // A plain success's summary only repeats the status: off screen, kept in
  // the text outputs.
  summary_label_->setVisible(!report.summary.isEmpty() &&
                             !report.summary_restates_headline);

  // Chips: the report's signers or recipients.
  if (auto* l = qobject_cast<QVBoxLayout*>(details_container_->layout())) {
    delete_widgets_in_layout(l, 0);
    for (const auto& chip : report.chips) {
      // The identity as text, aligned like a field value: it wraps where it
      // must and is never squeezed into a pill.
      auto* identity = new QLabel(chip, details_container_);
      identity->setTextFormat(Qt::PlainText);
      identity->setWordWrap(true);
      identity->setTextInteractionFlags(Qt::TextSelectableByMouse);
      identity->setFont(ReportUiFont());
      ApplyTone(identity, ReportTone::kTEXT, palette());
      l->addWidget(identity);
    }
  }
  key_details_->setText(report.chips_title);
  row_details_->setVisible(!report.chips.isEmpty());
  // No rules between the header, summary and cards: spacing separates them.
  sep_top_->setVisible(false);

  render_objects();

  time_label_->setText(
      tr("Issued:  %1")
          .arg(QLocale().toString(report.issued_at.toLocalTime(),
                                  QLocale::ShortFormat)));
  id_label_->setText(report.ref_id);
  // The footer shows enough of the hash to recognise it; the tooltip,
  // Copy and the PNG's metadata carry all of it.
  constexpr int kShownHashChars = 16;
  hash_label_->setText(
      tr("Report hash: %1")
          .arg(report.report_hash.size() > kShownHashChars
                   ? report.report_hash.left(kShownHashChars) + u"…"_s
                   : report.report_hash));
  hash_label_->setToolTip(report.report_hash);

  static_cast<DocFrame*>(doc_frame_)
      ->SetWatermark(HeadlineText(report.headline).toUpper(), color);

  ui_->saveToolButton->setToolTip(tr("Export certificate as PNG image"));

  current_copy_text_ = RenderReportText(report);

  placeholder_label_->setVisible(false);
  doc_scroll_->setVisible(true);
}

void InfoBoardWidget::render_objects() {
  auto* layout = qobject_cast<QVBoxLayout*>(extra_widget_->layout());
  if (layout == nullptr) return;
  delete_widgets_in_layout(layout, 0);

  const auto& objects = current_report_.objects;
  const auto shown_groups =
      show_all_groups_ ? objects.size()
                       : std::min<qsizetype>(objects.size(),
                                             StyleConstants::kMaxShownGroups);

  for (qsizetype i = 0; i < shown_groups; ++i) {
    const auto& object = objects[i];
    const bool expanded = expanded_groups_.contains(object.source.id);

    // The Outcome card is never collapsed: every error lives there.
    qsizetype domain_shown = 0;
    qsizetype domain_hidden = 0;
    QHash<QString, int> hidden_by_title;
    for (const auto& card : object.cards) {
      if (card.kind == InfoBoardCardKind::kOUTCOME) {
        render_card(layout, extra_widget_, card);
        continue;
      }
      if (expanded || domain_shown < StyleConstants::kMaxShownDomainCards) {
        render_card(layout, extra_widget_, card);
        ++domain_shown;
      } else {
        ++domain_hidden;
        ++hidden_by_title[card.title];
      }
    }

    if (domain_hidden > 0) {
      const int hidden_cards = static_cast<int>(domain_hidden);
      QStringList what;
      for (auto it = hidden_by_title.cbegin(); it != hidden_by_title.cend();
           ++it) {
        what << u"%1 × %2"_s.arg(it.value()).arg(it.key());
      }
      what.sort();
      const auto id = object.source.id;
      add_expander(
          layout, extra_widget_,
          tr("… %n more card(s) for this item (%1)", nullptr, hidden_cards)
              .arg(what.join(u", "_s)),
          tr("Show"), [this, id]() {
            expanded_groups_.insert(id);
            render_objects();
          });
    }
  }

  if (shown_groups < objects.size()) {
    InfoBoardOutcomeCounts hidden;
    for (auto i = shown_groups; i < objects.size(); ++i) {
      hidden.Add(objects[i].task_outcome);
    }
    const int hidden_items = static_cast<int>(objects.size() - shown_groups);
    add_expander(layout, extra_widget_,
                 tr("… %n more item(s): %1", nullptr, hidden_items)
                     .arg(CountSummaryText(hidden)),
                 tr("Show all"), [this]() {
                   show_all_groups_ = true;
                   render_objects();
                 });
  }

  const bool has_any = layout->count() > 0;
  extra_sep_->setVisible(false);
  extra_widget_->setVisible(has_any);
  UpdateActionButtons();
}

}  // namespace GpgFrontend::UI
