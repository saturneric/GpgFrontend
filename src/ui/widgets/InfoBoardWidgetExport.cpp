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

#include "ui/dialog/DocViewerDialog.h"
#include "ui/widgets/InfoBoardWidget.h"
#include "ui_InfoBoard.h"

namespace GpgFrontend::UI {

void InfoBoardWidget::slot_copy() {
  QString text;
  if (doc_scroll_ != nullptr && doc_scroll_->isVisible()) {
    text = current_copy_text_;
  } else {
    text = ui_->infoBoard->toPlainText();
  }
  if (text.trimmed().isEmpty()) return;
  QGuiApplication::clipboard()->setText(text);
  ui_->copyToolButton->setToolTip(tr("Copied"));
  QTimer::singleShot(1200, this, [this]() {
    ui_->copyToolButton->setToolTip(tr("Copy status text"));
  });
}

auto InfoBoardWidget::render_doc_pixmap(qreal scale) const -> QPixmap {
  if (doc_frame_ == nullptr) return {};

  QPixmap pm(doc_frame_->size() * scale);
  pm.fill(Qt::transparent);

  QPainter painter(&pm);
  painter.setRenderHint(QPainter::Antialiasing, true);
  painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
  painter.scale(scale, scale);
  doc_frame_->render(&painter);
  painter.end();

  return pm;
}

void InfoBoardWidget::slot_open_magnifier() {
  if (doc_scroll_ == nullptr || !doc_scroll_->isVisible()) return;

  constexpr qreal kViewerScale = 3.0;
  const QPixmap pm = render_doc_pixmap(kViewerScale);
  if (pm.isNull()) return;

  auto* dialog =
      new DocViewerDialog(pm, kViewerScale, current_report_.ref_id, this);
  dialog->setAttribute(Qt::WA_DeleteOnClose);
  dialog->show();
  dialog->raise();
  dialog->activateWindow();
}

auto InfoBoardWidget::ExportReportPng(const QString& file_path) const -> bool {
  const qreal export_scale = 3.0;
  const QPixmap pm = render_doc_pixmap(export_scale);
  if (pm.isNull()) return false;

  const auto& report = current_report_;

  // The image is the document as it is on screen; the text chunks carry the
  // whole report, so nothing a collapsed view leaves out is lost.
  QImage image = pm.toImage();
  image.setText(u"Software"_s, u"GpgFrontend"_s);
  image.setText(u"Title"_s, tr("GpgFrontend Security Report"));
  image.setText(u"Description"_s, tr("Cryptographic operation result report"));
  image.setText(u"Creation Time"_s,
                report.issued_at.toUTC().toString(Qt::ISODate));
  image.setText(u"Truncated"_s, IsReportCollapsed() ? u"true"_s : u"false"_s);

  if (!report.ref_id.isEmpty()) image.setText(u"Document ID"_s, report.ref_id);
  if (!report.report_hash.isEmpty()) {
    image.setText(u"Document Hash"_s, report.report_hash);
  }
  image.setText(u"Status"_s, HeadlineText(report.headline));
  if (!report.operation_label.isEmpty()) {
    image.setText(u"Operation"_s, report.operation_label);
  }
  if (!report.engine.isEmpty()) image.setText(u"Engine"_s, report.engine);
  if (report.objects.size() == 1 &&
      !report.objects.first().input_hash.isEmpty()) {
    image.setText(u"Input Hash"_s, report.objects.first().input_hash);
  }
  if (!current_copy_text_.isEmpty()) {
    image.setText(u"Comment"_s, current_copy_text_);
  }

  return image.save(file_path, "PNG");
}

void InfoBoardWidget::export_doc_as_png(const QString& file_path) {
  if (!ExportReportPng(file_path)) {
    QMessageBox::warning(this, tr("Unable to Save"),
                         tr("The image could not be saved."));
  }
}

void InfoBoardWidget::slot_save() {
  if (doc_scroll_ != nullptr && doc_scroll_->isVisible() &&
      doc_frame_ != nullptr) {
    const QString suggested = u"GpgFrontend_%1"_s.arg(
        current_report_.ref_id.isEmpty() ? u"Document"_s
                                         : current_report_.ref_id);
    QString file_path =
        QFileDialog::getSaveFileName(this, tr("Export Certificate"), suggested,
                                     tr("PNG Image (*.png);;All Files (*)"));
    if (file_path.isEmpty()) return;
    if (!file_path.endsWith(u".png"_s, Qt::CaseInsensitive)) {
      file_path += u".png"_s;
    }
    export_doc_as_png(file_path);
    return;
  }

  const auto text = ui_->infoBoard->toPlainText();
  if (text.trimmed().isEmpty()) return;

  auto file_path =
      QFileDialog::getSaveFileName(this, tr("Save Status Panel Content"), {},
                                   tr("Text Files (*.txt);;All Files (*)"));
  if (file_path.isEmpty()) return;

  QFile file(file_path);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate |
                 QIODevice::Text)) {
    QMessageBox::warning(this, tr("Unable to Save"),
                         tr("The file could not be saved. Please check the "
                            "path and permissions."));
    return;
  }
  file.write(text.toUtf8());
  file.close();
}

}  // namespace GpgFrontend::UI
