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
#include "ui/widgets/EditorEmptyView.h"

#include "ui/function/UIStyle.h"

namespace GpgFrontend::UI {

namespace {

constexpr int kColumnWidth = 340;
constexpr int kRowHeight = 34;
constexpr int kRowIcon = 18;
constexpr int kRowPadding = 10;
constexpr int kLogoSize = 72;
constexpr qreal kLogoOpacity = 0.22;

/**
 * @brief One way to start: icon, label, and its shortcut in muted text at
 *        the far end. Flat until hovered or focused.
 */
class StartRow : public QAbstractButton {
 public:
  StartRow(const QIcon& icon, const QString& text, const QString& shortcut,
           QWidget* parent)
      : QAbstractButton(parent), shortcut_(shortcut) {
    setIcon(icon);
    setText(text);
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::TabFocus);
    setAttribute(Qt::WA_Hover);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  }

  [[nodiscard]] auto sizeHint() const -> QSize override {
    return {kColumnWidth, kRowHeight};
  }

 protected:
  void paintEvent(QPaintEvent* /*event*/) override {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    const auto pal = palette();

    if (underMouse() || hasFocus()) {
      auto bg = pal.color(QPalette::Highlight);
      bg.setAlphaF(isDown() ? 0.22F : 0.12F);
      p.setPen(Qt::NoPen);
      p.setBrush(bg);
      p.drawRoundedRect(QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5), 6, 6);
    }

    auto r = rect().adjusted(kRowPadding, 0, -kRowPadding, 0);
    icon().paint(&p, QRect(r.left(), r.center().y() - (kRowIcon / 2) + 1,
                           kRowIcon, kRowIcon));
    r.setLeft(r.left() + kRowIcon + kRowPadding);

    const QFontMetrics fm(font());
    int shortcut_width = 0;
    if (!shortcut_.isEmpty()) {
      shortcut_width = fm.horizontalAdvance(shortcut_);
      p.setPen(MutedTextColor(pal));
      p.drawText(r, Qt::AlignVCenter | Qt::AlignRight, shortcut_);
    }

    p.setPen(pal.color(QPalette::WindowText));
    const auto label = fm.elidedText(text(), Qt::ElideRight,
                                     r.width() - shortcut_width - kRowPadding);
    p.drawText(r, Qt::AlignVCenter | Qt::AlignLeft, label);
  }

  void enterEvent(QEnterEvent* event) override {
    update();
    QAbstractButton::enterEvent(event);
  }

  void leaveEvent(QEvent* event) override {
    update();
    QAbstractButton::leaveEvent(event);
  }

 private:
  QString shortcut_;
};

auto ShortcutText(const QAction* act) -> QString {
  return act->shortcut().toString(QKeySequence::NativeText);
}

/// The application's mark, faded, so it reads as a watermark.
auto FadedLogo() -> QPixmap {
  const auto dpr = qApp->devicePixelRatio();
  const auto source = QIcon(":/icons/gpgfrontend_logo.png")
                          .pixmap(QSize(kLogoSize, kLogoSize), dpr);
  QPixmap faded(source.size());
  faded.setDevicePixelRatio(source.devicePixelRatio());
  faded.fill(Qt::transparent);
  QPainter p(&faded);
  p.setOpacity(kLogoOpacity);
  p.drawPixmap(0, 0, source);
  return faded;
}

}  // namespace

EditorEmptyView::EditorEmptyView(QWidget* parent) : QWidget(parent) {
  setFocusPolicy(Qt::StrongFocus);

  logo_ = new QLabel(this);
  logo_->setAlignment(Qt::AlignCenter);
  logo_->setPixmap(FadedLogo());

  title_ = new QLabel(tr("No open documents"), this);
  title_->setAlignment(Qt::AlignCenter);
  auto title_font = title_->font();
  title_font.setPointSizeF(title_font.pointSizeF() * 1.35);
  title_font.setWeight(QFont::DemiBold);
  title_->setFont(title_font);

  hint_ = new QLabel(tr("Start a new document or open one."), this);
  hint_->setAlignment(Qt::AlignCenter);

  rows_ = new QVBoxLayout;
  rows_->setSpacing(2);

  auto* column_widget = new QWidget(this);
  column_widget->setFixedWidth(kColumnWidth);
  auto* column = new QVBoxLayout(column_widget);
  column->setContentsMargins(0, 0, 0, 0);
  column->setSpacing(0);
  column->addWidget(logo_);
  column->addSpacing(14);
  column->addWidget(title_);
  column->addSpacing(4);
  column->addWidget(hint_);
  column->addSpacing(20);
  column->addLayout(rows_);

  auto* outer = new QVBoxLayout(this);
  outer->addStretch(2);
  outer->addWidget(column_widget, 0, Qt::AlignHCenter);
  outer->addStretch(3);

  apply_palette();
  rebuild();
}

void EditorEmptyView::SetStartActions(
    std::function<QList<QAction*>()> provider) {
  start_actions_ = std::move(provider);
  if (isVisible()) rebuild();
}

void EditorEmptyView::rebuild() {
  while (auto* item = rows_->takeAt(0)) {
    if (auto* w = item->widget()) w->deleteLater();
    delete item;
  }

  QList<QAction*> actions;
  if (start_actions_) actions = start_actions_();

  if (actions.isEmpty()) {
    auto* row =
        new StartRow(QIcon(":/icons/misc_doc.png"), tr("New Text"), {}, this);
    connect(row, &QAbstractButton::clicked, this,
            &EditorEmptyView::SignalNewTextRequested);
    rows_->addWidget(row);
  }

  for (auto* act : actions) {
    if (act == nullptr || act->isSeparator() || !act->isVisible()) continue;
    auto text = act->text();
    text.remove(u'&');
    auto* row = new StartRow(act->icon(), text, ShortcutText(act), this);
    row->setToolTip(act->toolTip());
    row->setEnabled(act->isEnabled());
    connect(row, &QAbstractButton::clicked, act, &QAction::trigger);
    rows_->addWidget(row);
  }

  auto* open =
      new StartRow(QIcon(":/icons/fileopen.png"), tr("Open File..."), {}, this);
  open->setToolTip(tr("Open a file in a tab"));
  connect(open, &QAbstractButton::clicked, this,
          &EditorEmptyView::SignalOpenFileRequested);
  rows_->addWidget(open);
}

void EditorEmptyView::showEvent(QShowEvent* event) {
  rebuild();
  QWidget::showEvent(event);
}

void EditorEmptyView::changeEvent(QEvent* event) {
  if (event->type() == QEvent::PaletteChange) apply_palette();
  QWidget::changeEvent(event);
}

void EditorEmptyView::apply_palette() {
  auto p = hint_->palette();
  p.setColor(QPalette::WindowText, MutedTextColor(palette()));
  hint_->setPalette(p);
}

}  // namespace GpgFrontend::UI
