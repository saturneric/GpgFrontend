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

#include "core/function/result_analyse/GpgOpResultInfo.h"
#include "ui/function/InfoBoardCardConverter.h"
#include "ui/widgets/InfoBoardDocFrame.h"
#include "ui/widgets/InfoBoardWidget.h"
#include "ui_InfoBoard.h"

namespace GpgFrontend::UI {

void InfoBoardWidget::update_doc_header(InfoBoardStatus status,
                                        const QString& operation,
                                        const QString& engine,
                                        const QString& count_summary) {
  if (header_label_ == nullptr) return;

  const QColor color = StatusColor(status);
  const QString accent = color.name();

  QStringList meta;
  if (!operation.isEmpty()) meta << operation.toHtmlEscaped();
  if (!count_summary.isEmpty()) meta << count_summary.toHtmlEscaped();
  if (!engine.isEmpty()) meta << engine.toHtmlEscaped();
  const QString subtitle = meta.join(QStringLiteral("  ·  "));

  header_label_->setText(build_header_html(status, subtitle, accent));

  const QString symbol = build_status_symbol(status);
  if (val_status_ != nullptr) {
    val_status_->setText(symbol);
    val_status_->setStyleSheet(
        QStringLiteral("color: %1; font-weight: bold;").arg(accent));
  }
  if (val_operation_ != nullptr) val_operation_->setText(operation);
  if (val_engine_ != nullptr) val_engine_->setText(engine);

  if (row_status_ != nullptr) row_status_->setVisible(false);
  if (row_operation_ != nullptr) row_operation_->setVisible(false);
  if (row_engine_ != nullptr) row_engine_->setVisible(false);
}

void InfoBoardWidget::update_status_page(
    const QString& text, InfoBoardStatus status, const QString& content_hash,
    const QString& operation, const QString& description,
    const QContainer<InfoBoardCard>& cards, const QString& details_title,
    const QStringList& details_items) {
  if (doc_frame_ == nullptr) return;

  const QColor color = StatusColor(status);
  const QString icon_path = StatusIconPath(status);

  stamp_label_->setPixmap(
      make_stamp_pixmap(color, icon_path, StyleConstants::kStampSize));

  const QString accent = color.name();
  const QString border_subtle = QStringLiteral("rgba(%1,%2,%3,50)")
                                    .arg(color.red())
                                    .arg(color.green())
                                    .arg(color.blue());
  doc_frame_->setStyleSheet(
      QStringLiteral(
          "#DocFrame { border: 1px solid %1; border-left: 4px solid %2;"
          " border-radius: 6px; }")
          .arg(border_subtle, accent));

  QString resolved_operation = operation;
  QString resolved_description = description;
  if (resolved_description.isEmpty() && !resolved_operation.isEmpty()) {
    resolved_description = StatusDescription(status, resolved_operation);
  }
  QContainer<InfoBoardCard> resolved_cards = cards;
  const QString& resolved_details_title = details_title;
  const QStringList& resolved_details_items = details_items;

  if (resolved_cards.isEmpty() && resolved_details_items.isEmpty()) {
    const QString hash_title = tr("File Hash Information");
    if (text.contains(hash_title)) {
      resolved_operation = hash_title;
      struct HashField {
        QString key;
        QString value;
      };
      QList<HashField> fields;
      const QStringList lines = text.split(QLatin1Char('\n'));
      for (const QString& raw_line : lines) {
        const QString line = raw_line.trimmed();
        if (!line.startsWith(QLatin1String("- "))) continue;
        const QString body = line.mid(2);
        const int colon_pos = body.indexOf(QLatin1String(": "));
        if (colon_pos < 0) continue;
        fields.append({body.left(colon_pos), body.mid(colon_pos + 2)});
      }
      if (!fields.isEmpty()) {
        InfoBoardCard card;
        card.title = hash_title;
        card.status = status;
        for (const auto& f : fields) {
          card.fields.append({f.key, f.value});
        }
        resolved_cards.append(card);
      }
    }
  }

  update_doc_header(status, resolved_operation, {}, {});

  if (auto* l = qobject_cast<QVBoxLayout*>(details_container_->layout())) {
    delete_widgets_in_layout(l, 0);
  }
  key_details_->setText(tr("DETAILS"));
  row_details_->setVisible(false);

  if (!resolved_details_items.isEmpty() || !resolved_description.isEmpty()) {
    populate_details_section_generic(resolved_details_title,
                                     resolved_details_items, status,
                                     resolved_description);
  }

  const bool has_details = row_details_ != nullptr && row_details_->isVisible();
  if (sep_top_ != nullptr) sep_top_->setVisible(has_details);

  if (auto* l = qobject_cast<QVBoxLayout*>(extra_widget_->layout())) {
    delete_widgets_in_layout(l, 0);
  }

  bool has_extra = false;
  if (!resolved_cards.isEmpty()) {
    auto* extra_layout = qobject_cast<QVBoxLayout*>(extra_widget_->layout());
    if (extra_layout != nullptr) {
      delete_widgets_in_layout(extra_layout, 0);
      render_cards(extra_layout, extra_widget_, resolved_cards);
      has_extra = true;
    }
  }

  if (has_extra) {
    if (extra_sep_ != nullptr) extra_sep_->setVisible(true);
    extra_widget_->setVisible(true);
  } else {
    extra_widget_->setVisible(false);
    if (extra_sep_ != nullptr) extra_sep_->setVisible(false);
  }

  const auto now = QDateTime::currentDateTime();
  time_label_->setText(
      tr("Issued:  %1").arg(QLocale().toString(now, QLocale::ShortFormat)));

  // Build content string for hashing (all visible content)
  QString content_for_hash;
  content_for_hash += text;
  content_for_hash += resolved_operation;
  for (const auto& card : resolved_cards) {
    content_for_hash += card.title;
    for (const auto& field : card.fields) {
      content_for_hash += field.first + field.second;
    }
  }
  content_for_hash += resolved_details_title;
  content_for_hash += resolved_details_items.join(QString());
  content_for_hash += now.toString(Qt::ISODate);

  // Compute SHA-256 hash of document content
  const QByteArray hash_bytes = QCryptographicHash::hash(
      content_for_hash.toUtf8(), QCryptographicHash::Sha256);
  const QString doc_hash = QString::fromLatin1(hash_bytes.toHex());

  const QString op_abbr = [&status]() -> QString {
    switch (status) {
      case kINFO_ERROR_OK:
        return QStringLiteral("OK");
      case kINFO_ERROR_WARN:
        return QStringLiteral("WN");
      case kINFO_ERROR_CRITICAL:
        return QStringLiteral("FL");
      default:
        return QStringLiteral("GP");
    }
  }();

  // Use first 8 hex chars of document hash for REF ID
  const QString content_id = doc_hash.left(8).toUpper();

  current_id_ = QStringLiteral("REF-%1-%2-%3")
                    .arg(op_abbr)
                    .arg(now.toString(QStringLiteral("yyyyMMdd")))
                    .arg(content_id);
  id_label_->setText(current_id_);

  // Display full hash in footer
  hash_label_->setText(tr("Hash: %1").arg(doc_hash.toUpper()));

  static_cast<DocFrame*>(doc_frame_)
      ->SetWatermark(StatusTitle(status).toUpper(), StatusColor(status));

  ui_->saveToolButton->setToolTip(tr("Export certificate as PNG image"));

  placeholder_label_->setVisible(false);
  doc_scroll_->setVisible(true);

  // Build plain-text copy of document content
  {
    QStringList lines;
    if (!current_id_.isEmpty()) {
      lines << current_id_;
      lines << QString(current_id_.length(), QLatin1Char('-'));
    }
    if (!resolved_operation.isEmpty()) {
      lines << tr("Operation: %1").arg(resolved_operation);
    }
    lines << tr("Status:    %1").arg(build_status_symbol(status));
    if (!content_hash.isEmpty()) lines << tr("SHA-256:   %1").arg(content_hash);
    if (!resolved_description.isEmpty()) lines << resolved_description;
    if (!resolved_details_items.isEmpty()) {
      const QString key = resolved_details_title.isEmpty()
                              ? tr("DETAILS")
                              : resolved_details_title;
      lines << QStringLiteral("%1:  %2").arg(
          key, resolved_details_items.join(QStringLiteral(", ")));
    }
    for (const auto& card : resolved_cards) {
      lines << QString();
      lines << card.title;
      for (const auto& f : card.fields) {
        lines << QStringLiteral("  %1: %2").arg(f.first, f.second);
      }
    }
    lines << QString() << time_label_->text();
    current_copy_text_ = lines.join(QLatin1Char('\n'));
  }
}

void InfoBoardWidget::populate_details_section(
    const GpgFrontend::GpgOpResultInfo& info, InfoBoardStatus status) {
  if (info.details.isEmpty()) return;

  if (info.operation.contains(tr("Decrypt"))) {
    key_details_->setText(tr("RECIPIENT"));
  } else if (info.operation.contains(tr("Sign")) ||
             info.operation.contains(tr("Verify"))) {
    key_details_->setText(tr("SIGNER"));
  } else {
    key_details_->setText(tr("DETAILS"));
  }

  auto* dc_layout = qobject_cast<QVBoxLayout*>(details_container_->layout());
  if (dc_layout == nullptr) return;

  delete_widgets_in_layout(dc_layout, 0);

  for (const auto& detail : info.details) {
    dc_layout->addWidget(
        create_detail_chip(details_container_, detail, status));
  }
  row_details_->setVisible(true);
}

void InfoBoardWidget::populate_extra_section(
    const GpgFrontend::GpgOpResultInfo& info, InfoBoardStatus /*status*/) {
  if (extra_widget_ == nullptr) return;

  auto* l = qobject_cast<QVBoxLayout*>(extra_widget_->layout());
  if (l == nullptr) return;

  delete_widgets_in_layout(l, 0);

  const QString& desc = info.description;
  if (!desc.isEmpty()) {
    auto* desc_label = new QLabel(extra_widget_);
    desc_label->setTextFormat(Qt::RichText);
    desc_label->setText(
        QStringLiteral("<span style='font-size:%1pt;'>%2</span>")
            .arg(font().pointSize())
            .arg(desc));
    desc_label->setWordWrap(true);
    desc_label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    desc_label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    desc_label->setStyleSheet(
        QStringLiteral("padding: 2px 0 8px 0;"
                       " border-bottom: 1px solid palette(mid);"));
    l->addWidget(desc_label);
  }

  bool has_cards = populate_extra_from_op_info(l, extra_widget_, info);
  const bool has_any = has_cards || !desc.isEmpty();

  if (extra_sep_ != nullptr) extra_sep_->setVisible(has_any);
  extra_widget_->setVisible(has_any);
}

void InfoBoardWidget::populate_details_section_generic(
    const QString& title, const QStringList& items, InfoBoardStatus status,
    const QString& description) {
  if (items.isEmpty() && description.isEmpty()) return;

  auto* dc_layout = qobject_cast<QVBoxLayout*>(details_container_->layout());
  if (dc_layout == nullptr) return;

  delete_widgets_in_layout(dc_layout, 0);

  key_details_->setText(title.isEmpty() ? tr("DETAILS") : title);

  // Add description at the top if provided
  if (!description.isEmpty()) {
    auto* desc_label = new QLabel(details_container_);
    desc_label->setTextFormat(Qt::RichText);
    desc_label->setText(
        QStringLiteral(
            "<span style='font-size:%1pt; font-style:italic;'>%2</span>")
            .arg(font().pointSize())
            .arg(description));
    desc_label->setWordWrap(true);
    desc_label->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    desc_label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    desc_label->setStyleSheet(
        QStringLiteral("color: palette(text); padding: 4px "
                       "0; border-bottom: 1px solid palette(mid);"));
    dc_layout->addWidget(desc_label);
  }

  for (const auto& item : items) {
    dc_layout->addWidget(create_detail_chip(details_container_, item, status));
  }
  row_details_->setVisible(true);
}

void InfoBoardWidget::SetInfoBoardWithOpInfo(
    const QString& text, InfoBoardStatus status,
    const GpgFrontend::GpgOpResultInfo& info) {
  SetInfoBoard(text, status, info.inputHash);
  if (doc_frame_ == nullptr) return;

  update_doc_header(status, info.operation, info.engine, {});
  populate_details_section(info, status);

  const bool has_details = row_details_ != nullptr && row_details_->isVisible();
  if (sep_top_ != nullptr) sep_top_->setVisible(has_details);

  populate_extra_section(info, status);

  // Rebuild copy text now that full op_info content is populated
  {
    QStringList lines;
    if (!current_id_.isEmpty()) {
      lines << current_id_;
      lines << QString(current_id_.length(), QLatin1Char('-'));
    }
    if (!info.operation.isEmpty()) {
      lines << tr("Operation: %1").arg(info.operation);
    }
    lines << tr("Status:    %1").arg(build_status_symbol(status));
    if (!info.engine.isEmpty()) lines << tr("Engine:    %1").arg(info.engine);
    if (!info.inputHash.isEmpty()) {
      lines << tr("SHA-256:   %1").arg(info.inputHash);
    }
    if (!info.description.isEmpty()) lines << info.description;
    if (!info.details.isEmpty()) {
      lines << tr("Details:   %1").arg(info.details.join(QStringLiteral(", ")));
    }
    const QStringList card_lines = build_op_info_copy_lines(info);
    if (!card_lines.isEmpty()) {
      lines << QString();
      lines.append(card_lines);
    }
    if (time_label_ != nullptr && !time_label_->text().isEmpty()) {
      lines << QString() << time_label_->text();
    }
    current_copy_text_ = lines.join(QLatin1Char('\n'));
  }
}

void InfoBoardWidget::SetInfoBoardFromResults(
    const QString& text, InfoBoardStatus overall_status,
    const QContainer<GpgOperaResult>& results) {
  // Use the first result's input hash for content-dependent REF generation
  QString content_hash;
  if (!results.isEmpty()) {
    content_hash = results.first().op_info.inputHash;
  }
  SetInfoBoard(text, overall_status, content_hash);
  if (doc_frame_ != nullptr && !results.isEmpty()) {
    update_status_page_from_results(overall_status, results);
  }
}

void InfoBoardWidget::update_status_page_from_results(
    InfoBoardStatus status, const QContainer<GpgOperaResult>& results) {
  if (extra_widget_ == nullptr) return;

  auto* extra_layout = qobject_cast<QVBoxLayout*>(extra_widget_->layout());
  if (extra_layout == nullptr) return;

  delete_widgets_in_layout(extra_layout, 0);

  // Aggregate op_info from all results
  QString agg_operation;
  QString agg_engine;
  QString agg_description;
  QStringList agg_details;
  for (const auto& r : results) {
    if (!r.op_info.operation.isEmpty() && agg_operation.isEmpty()) {
      agg_operation = r.op_info.operation;
    }
    if (!r.op_info.engine.isEmpty() && agg_engine.isEmpty()) {
      agg_engine = r.op_info.engine;
    }
    if (!r.op_info.description.isEmpty() && agg_description.isEmpty()) {
      agg_description = r.op_info.description;
    }
    for (const auto& d : r.op_info.details) {
      if (!agg_details.contains(d)) agg_details << d;
    }
  }

  // Details row — chips only, no description
  if (!agg_details.isEmpty()) {
    if (agg_operation == tr("Decrypt")) {
      key_details_->setText(tr("RECIPIENT"));
    } else if (agg_operation == tr("Sign") || agg_operation == tr("Verify")) {
      key_details_->setText(tr("SIGNER"));
    } else {
      key_details_->setText(tr("DETAILS"));
    }

    if (auto* dc_layout =
            qobject_cast<QVBoxLayout*>(details_container_->layout())) {
      delete_widgets_in_layout(dc_layout, 0);

      const QString chip_bg = QStringLiteral("rgba(100,100,100,10)");
      const QString chip_border = QStringLiteral("rgba(100,100,100,30)");

      for (const auto& detail : agg_details) {
        auto* chip = new QLabel(detail, details_container_);
        chip->setWordWrap(true);
        chip->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        chip->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
        chip->setStyleSheet(
            QStringLiteral("padding: 3px 8px; border-radius: 4px;"
                           " background-color: %1; border: 1px solid %2;")
                .arg(chip_bg, chip_border));
        dc_layout->addWidget(chip);
      }
    }
    row_details_->setVisible(true);
  } else {
    row_details_->setVisible(false);
  }

  const bool has_details = row_details_ != nullptr && row_details_->isVisible();
  if (sep_top_ != nullptr) sep_top_->setVisible(has_details);

  // Build count summary
  const bool has_tags =
      std::any_of(results.begin(), results.end(),
                  [](const GpgOperaResult& r) { return !r.tag.isEmpty(); });
  QString count_summary;
  if (has_tags) {
    int ok_count = 0;
    int warn_count = 0;
    int fail_count = 0;
    for (const auto& r : results) {
      if (r.op_info.status > 0) {
        ok_count++;
      } else if (r.op_info.status == 0) {
        warn_count++;
      } else {
        fail_count++;
      }
    }
    QStringList parts;
    if (ok_count > 0) {
      parts << QStringLiteral("%1 %2").arg(ok_count).arg(tr("OK"));
    }
    if (warn_count > 0) {
      parts << QStringLiteral("%1 %2").arg(warn_count).arg(tr("Warning"));
    }
    if (fail_count > 0) {
      parts << QStringLiteral("%1 %2").arg(fail_count).arg(tr("Failed"));
    }
    count_summary = parts.join(QStringLiteral(", "));
  }

  update_doc_header(status, agg_operation, agg_engine, count_summary);

  const QString agg_desc = agg_description.isEmpty()
                               ? StatusDescription(status, agg_operation)
                               : agg_description;
  if (!agg_desc.isEmpty()) {
    auto* desc_label = new QLabel(extra_widget_);
    desc_label->setTextFormat(Qt::RichText);
    desc_label->setText(
        QStringLiteral("<span style='font-size:%1pt;'>%2</span>")
            .arg(font().pointSize())
            .arg(agg_desc));
    desc_label->setWordWrap(true);
    desc_label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    desc_label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    desc_label->setStyleSheet(
        QStringLiteral("padding: 2px 0 8px 0;"
                       " border-bottom: 1px solid palette(mid);"));
    extra_layout->addWidget(desc_label);
  }

  // Per-result rows
  bool has_cards = false;
  int shown = 0;
  for (const auto& r : results) {
    if (shown >= StyleConstants::kMaxResultCards) {
      auto* more = new QLabel(tr("… and %1 more").arg(results.size() - shown),
                              extra_widget_);
      more->setAlignment(Qt::AlignCenter);
      QFont f = more->font();
      f.setPointSize(std::max(7, f.pointSize() - 1));
      more->setFont(f);
      more->setStyleSheet(QStringLiteral("color: palette(text);"));
      extra_layout->addWidget(more);
      break;
    }

    InfoBoardStatus item_status = kINFO_ERROR_CRITICAL;
    if (r.op_info.status > 0) {
      item_status = kINFO_ERROR_OK;
    } else if (r.op_info.status == 0) {
      item_status = kINFO_ERROR_WARN;
    }

    if (!r.tag.isEmpty()) {
      const QColor ic = StatusColor(item_status);
      const QString icon_path = StatusIconPath(item_status);

      auto* row = new QFrame(extra_widget_);
      row->setObjectName(QStringLiteral("DocResultRow"));
      row->setStyleSheet(build_card_stylesheet(ic));

      auto* rl = new QHBoxLayout(row);
      rl->setContentsMargins(10, 5, 10, 5);
      rl->setSpacing(8);

      auto* icon_lbl = new QLabel(row);
      icon_lbl->setFixedSize(18, 18);
      icon_lbl->setPixmap(QPixmap(icon_path).scaled(14, 14, Qt::KeepAspectRatio,
                                                    Qt::SmoothTransformation));
      rl->addWidget(icon_lbl, 0, Qt::AlignVCenter);

      auto* tag_lbl = new QLabel(r.tag, row);
      tag_lbl->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
      tag_lbl->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
      QFont tf = tag_lbl->font();
      tf.setPointSize(std::max(8, tf.pointSize() - 1));
      tag_lbl->setFont(tf);
      rl->addWidget(tag_lbl, 1);

      extra_layout->addWidget(row);
    }
    if (populate_extra_from_op_info(extra_layout, extra_widget_, r.op_info)) {
      has_cards = true;
    }
    shown++;
  }

  const bool has_any = has_cards || !agg_desc.isEmpty();
  extra_sep_->setVisible(has_any);
  extra_widget_->setVisible(has_any);

  const auto now =
      QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat);
  time_label_->setText(tr("Issued:  %1").arg(now));

  // Build plain-text copy of document content
  {
    QStringList lines;
    if (!current_id_.isEmpty()) {
      lines << current_id_;
      lines << QString(current_id_.length(), QLatin1Char('-'));
    }
    if (!agg_operation.isEmpty()) {
      lines << tr("Operation: %1").arg(agg_operation);
    }
    lines << tr("Status:    %1").arg(build_status_symbol(status));
    if (!agg_engine.isEmpty()) lines << tr("Engine:    %1").arg(agg_engine);
    if (!count_summary.isEmpty()) lines << count_summary;
    if (!agg_desc.isEmpty()) lines << agg_desc;
    if (!agg_details.isEmpty()) {
      lines << tr("Details:   %1").arg(agg_details.join(QStringLiteral(", ")));
    }
    for (const auto& r : results) {
      QStringList card_lines;
      if (!r.op_info.inputHash.isEmpty()) {
        card_lines << tr("  SHA-256: %1").arg(r.op_info.inputHash);
      }
      card_lines.append(build_op_info_copy_lines(r.op_info));
      if (!card_lines.isEmpty()) {
        lines << QString();
        if (!r.tag.isEmpty()) lines << r.tag;
        lines.append(card_lines);
      }
    }
    lines << QString() << tr("Issued:  %1").arg(now);
    current_copy_text_ = lines.join(QLatin1Char('\n'));
  }
}

auto InfoBoardWidget::populate_extra_from_op_info(QVBoxLayout* extra_layout,
                                                  QWidget* parent,
                                                  const GpgOpResultInfo& info)
    -> bool {
  if (extra_layout == nullptr || parent == nullptr) return false;

  if (!info.inputHash.isEmpty()) {
    current_input_hash_ = info.inputHash;
  }

  const auto cards = convert_op_info_to_cards(info);
  if (cards.isEmpty()) return false;

  render_cards(extra_layout, parent, cards);
  return true;
}

auto InfoBoardWidget::build_op_info_copy_lines(
    const GpgOpResultInfo& info) const -> QStringList {
  QStringList lines;
  for (const auto& sig : info.signatures) {
    const QString who =
        sig.signer.uid.isEmpty() ? sig.signer.fingerprint : sig.signer.uid;
    if (!who.isEmpty()) lines << tr("  Signer: %1").arg(who);
    if (!sig.signer.keyId.isEmpty())
      lines << tr("  Key ID: %1").arg(sig.signer.keyId);
  }
  for (const auto& ns : info.newSignatures) {
    const QString who =
        ns.signer.uid.isEmpty() ? ns.signer.fingerprint : ns.signer.uid;
    if (!who.isEmpty()) lines << tr("  Signed: %1").arg(who);
    if (!ns.signer.keyId.isEmpty())
      lines << tr("  Key ID: %1").arg(ns.signer.keyId);
  }
  for (const auto& inv : info.invalidSigners) {
    lines << tr("  Invalid signer: %1 — %2").arg(inv.first, inv.second);
  }
  if (info.operation.contains(tr("Encrypt")) ||
      info.operation.contains(tr("Decrypt"))) {
    for (const auto& reci : info.recipients) {
      const QString who = reci.uid.isEmpty() ? reci.fingerprint : reci.uid;
      if (!who.isEmpty()) lines << tr("  Recipient: %1").arg(who);
      if (!reci.keyId.isEmpty()) lines << tr("  Key ID: %1").arg(reci.keyId);
    }
  }
  return lines;
}

}  // namespace GpgFrontend::UI
