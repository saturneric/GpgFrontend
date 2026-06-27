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

#include <functional>

namespace GpgFrontend::UI {

// QGraphicsView that supports free zooming via the mouse wheel and exposes
// programmatic zoom helpers for the toolbar buttons. Zoom is expressed as a
// factor relative to the document's natural (on-screen) size: 1.0 == 100%.
class ZoomGraphicsView final : public QGraphicsView {
 public:
  ZoomGraphicsView(qreal source_scale, QWidget* parent)
      : QGraphicsView(parent), source_scale_(source_scale) {
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorViewCenter);
    setDragMode(QGraphicsView::ScrollHandDrag);
    setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    setAlignment(Qt::AlignCenter);
  }

  // Callback invoked whenever the zoom factor changes.
  std::function<void(qreal)> on_zoom_changed;

  [[nodiscard]] auto Zoom() const -> qreal { return zoom_; }

  void SetZoom(qreal zoom) {
    zoom_ = std::clamp(zoom, kMinZoom, kMaxZoom);
    QTransform t;
    t.scale(zoom_ / source_scale_, zoom_ / source_scale_);
    setTransform(t);
    if (on_zoom_changed) on_zoom_changed(zoom_);
  }

  void ZoomBy(qreal factor) { SetZoom(zoom_ * factor); }

  void FitToWindow() {
    if (scene() == nullptr || scene()->items().isEmpty()) return;
    fitInView(scene()->itemsBoundingRect(), Qt::KeepAspectRatio);
    zoom_ = transform().m11() * source_scale_;
    if (on_zoom_changed) on_zoom_changed(zoom_);
  }

 protected:
  void wheelEvent(QWheelEvent* event) override {
    if (event->angleDelta().y() == 0) {
      QGraphicsView::wheelEvent(event);
      return;
    }
    ZoomBy(event->angleDelta().y() > 0 ? kWheelStep : 1.0 / kWheelStep);
    event->accept();
  }

 private:
  static constexpr qreal kMinZoom = 0.1;
  static constexpr qreal kMaxZoom = 10.0;
  static constexpr qreal kWheelStep = 1.15;

  qreal source_scale_;
  qreal zoom_ = 1.0;
};

DocViewerDialog::DocViewerDialog(const QPixmap& doc, qreal source_scale,
                                 const QString& doc_id, QWidget* parent)
    : GeneralDialog(QStringLiteral("doc_viewer_dialog"), parent) {
  setWindowTitle(doc_id.isEmpty() ? tr("Document Preview")
                                  : tr("Document Preview — %1").arg(doc_id));

  auto* scene = new QGraphicsScene(this);
  scene->addPixmap(doc);
  scene->setSceneRect(QRectF(QPointF(0, 0), doc.size()));

  view_ = new ZoomGraphicsView(source_scale, this);
  view_->setScene(scene);

  auto* zoom_label = new QLabel(this);
  zoom_label->setMinimumWidth(48);
  zoom_label->setAlignment(Qt::AlignCenter);
  view_->on_zoom_changed = [zoom_label](qreal zoom) {
    zoom_label->setText(QStringLiteral("%1%").arg(qRound(zoom * 100)));
  };

  auto make_button = [this](const QString& icon, const QString& tip,
                            auto&& handler) -> QToolButton* {
    auto* button = new QToolButton(this);
    button->setIcon(QIcon(icon));
    button->setToolTip(tip);
    button->setAutoRaise(true);
    button->setFocusPolicy(Qt::NoFocus);
    connect(button, &QToolButton::clicked, this,
            std::forward<decltype(handler)>(handler));
    return button;
  };

  auto* toolbar = new QHBoxLayout();
  toolbar->setContentsMargins(4, 4, 4, 4);
  toolbar->setSpacing(4);
  toolbar->addWidget(make_button(QStringLiteral(":/icons/zoomout.png"),
                                 tr("Zoom out"),
                                 [this]() { view_->ZoomBy(1.0 / 1.25); }));
  toolbar->addWidget(zoom_label);
  toolbar->addWidget(make_button(QStringLiteral(":/icons/zoomin.png"),
                                 tr("Zoom in"),
                                 [this]() { view_->ZoomBy(1.25); }));
  toolbar->addSpacing(8);
  toolbar->addWidget(make_button(QStringLiteral(":/icons/expand.png"),
                                 tr("Fit to window"),
                                 [this]() { view_->FitToWindow(); }));
  toolbar->addWidget(make_button(QStringLiteral(":/icons/search.png"),
                                 tr("Actual size (100%)"),
                                 [this]() { view_->SetZoom(1.0); }));
  toolbar->addStretch(1);

  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);
  layout->addLayout(toolbar);
  layout->addWidget(view_, 1);

  resize(900, 720);
}

void DocViewerDialog::showEvent(QShowEvent* event) {
  GeneralDialog::showEvent(event);
  if (!first_show_) return;
  first_show_ = false;
  // Start by fitting the whole document, but never upscale a small report
  // beyond its natural size.
  view_->FitToWindow();
  if (view_->Zoom() > 1.0) view_->SetZoom(1.0);
}

}  // namespace GpgFrontend::UI
