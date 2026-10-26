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

#include "ui/dialog/GeneralWizard.h"

#include "core/model/SettingsObject.h"
#include "ui/function/WindowGeometry.h"
#include "ui/struct/settings_object/WindowStateSO.h"

namespace GpgFrontend::UI {

namespace {

/// Word-wrapped labels ask for almost no width, so a wizard would open as a
/// tall ribbon; stating the width once makes everything wrap against it.
constexpr int kWizardMinimumWidth = 520;

auto AvailableGeometry(QWidget* widget) -> QRect {
  auto* screen = widget->screen();
  if (screen == nullptr) screen = QGuiApplication::primaryScreen();
  if (screen == nullptr) return {0, 0, 1024, 768};
  return screen->availableGeometry();
}

}  // namespace

auto GeneralWizardPage::isComplete() const -> bool {
  return complete ? complete() : QWizardPage::isComplete();
}

auto GeneralWizardPage::validatePage() -> bool {
  return validate ? validate() : QWizardPage::validatePage();
}

GeneralWizard::GeneralWizard(QString name, const QString& finish_text,
                             QWidget* parent)
    : QWizard(parent), name_(std::move(name)) {
  setModal(true);

  // The same style choice as the first-run wizard, so they read as one family.
  // No logo or watermark: the page titles carry the step, and a large badge
  // would only push the fields down.
#if !defined(Q_OS_MACOS)
  setWizardStyle(QWizard::ModernStyle);
#endif
  setPixmap(QWizard::LogoPixmap, {});
  setPixmap(QWizard::WatermarkPixmap, {});
  setPixmap(QWizard::BackgroundPixmap, {});
  setOption(QWizard::HaveHelpButton, false);
  setOption(QWizard::NoBackButtonOnStartPage);

  setButtonText(QWizard::BackButton, tr("Back"));
  setButtonText(QWizard::NextButton, tr("Continue"));
  setButtonText(QWizard::FinishButton, finish_text);

  setMinimumWidth(kWizardMinimumWidth);
}

auto GeneralWizard::AddStep(int index, int count, const QString& title)
    -> GeneralWizardPage* {
  auto* page = new GeneralWizardPage(this);
  page->setTitle(title);
  page->setSubTitle(tr("Step %1 of %2").arg(index + 1).arg(count));
  setPage(index, page);
  return page;
}

auto GeneralWizard::PageLayout(QWidget* page) -> QVBoxLayout* {
  auto* layout = new QVBoxLayout(page);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);
  return layout;
}

void GeneralWizard::AddLead(QVBoxLayout* layout, const QString& text,
                            QWidget* page) {
  auto* lead = new QLabel(text, page);
  lead->setWordWrap(true);
  layout->addWidget(lead);
  layout->addSpacing(4);
}

void GeneralWizard::setVisible(bool visible) {
  if (visible && !first_show_handled_) {
    first_show_handled_ = true;
    ensurePolished();
    size_is_estimate_ = !restore_geometry();
    if (size_is_estimate_) place_first_time();

    // The estimate above is made before any page has a real width, and fonts
    // differ between displays; a size remembered from an older layout can be
    // short too. Checked again once the window is laid out, so no step is ever
    // squeezed below what its fields need.
    QTimer::singleShot(0, this, &GeneralWizard::fit_tallest_page);
  }
  QWizard::setVisible(visible);
}

void GeneralWizard::fit_tallest_page() {
  const auto* current = currentPage();
  if (current == nullptr || current->width() <= 0) return;

  const int width = current->width();
  int tallest = 0;
  for (const auto id : pageIds()) {
    const auto* each = page(id);
    const auto* each_layout = each->layout();
    tallest = std::max(
        tallest, each_layout != nullptr && each_layout->hasHeightForWidth()
                     ? each_layout->totalHeightForWidth(width)
                     : each->sizeHint().height());
  }

  // Everything around the page -- header, buttons, margins -- stays as it is;
  // only the page area has to reach the tallest page.
  const int needed = height() - current->height() + tallest;
  setMinimumHeight(needed);

  // The estimate made before the window was laid out errs tall, and a tall
  // wizard is mostly blank space, so it is corrected in both directions,
  // around the same centre. A size the user chose only grows.
  const bool fit_exactly = std::exchange(size_is_estimate_, false);
  if (height() < needed || (fit_exactly && height() > needed)) {
    auto rect = geometry();
    const auto center = rect.center();
    rect.setHeight(needed);
    if (fit_exactly) rect.moveCenter(center);
    setGeometry(ClampRectToAvailableGeometry(rect, AvailableGeometry(this)));
  }
}

void GeneralWizard::done(int result) {
  if (isVisible()) save_geometry();
  QWizard::done(result);
}

auto GeneralWizard::restore_geometry() -> bool {
  SettingsObject state_object(name_ + "_dialog_state");
  const WindowStateSO state(state_object);
  if (!state.window_save || state.width <= 0 || state.height <= 0) {
    return false;
  }

  QRect rect{QPoint{state.x, state.y}, QSize(state.width, state.height)};
  rect.setSize(rect.size().expandedTo(minimumSizeHint()));
  setGeometry(ClampRectToAvailableGeometry(rect, AvailableGeometry(this)));
  return true;
}

void GeneralWizard::place_first_time() {
  // QWizard sizes itself for the page it opens on and never grows for a
  // taller one later: it squeezes that page instead. So the window starts tall
  // enough for the tallest page, which also means no step ever resizes it.
  // QWizard only picks its start page inside its own setVisible(); pick it now
  // so there is a page to measure against, and setVisible() leaves it be.
  if (currentId() == -1) restart();

  auto size = sizeHint().expandedTo(minimumSizeHint());
  if (const auto* current = currentPage(); current != nullptr) {
    // Wrapped text is measured at the width the page will get, not at its own
    // narrow hint, or every page looks a few lines taller than it is. The
    // pages are not laid out yet, so the width comes from the frame around
    // them; erring narrow only ever errs taller.
    const auto margins =
        layout() != nullptr ? layout()->contentsMargins() : QMargins{};
    const int width = size.width() - margins.left() - margins.right();
    const auto height_of = [width](const QWizardPage* page) {
      const auto* page_layout = page->layout();
      if (page_layout != nullptr && page_layout->hasHeightForWidth()) {
        return page_layout->totalHeightForWidth(width);
      }
      return page->sizeHint().height();
    };

    int tallest = 0;
    for (const auto id : pageIds()) {
      tallest = std::max(tallest, height_of(page(id)));
    }
    size.rheight() += std::max(0, tallest - current->sizeHint().height());
  }

  QRect rect{QPoint{}, size};

  auto* parent_widget = parentWidget();
  QWidget* anchor = parent_widget != nullptr ? parent_widget->window() : this;
  const auto available = AvailableGeometry(anchor);

  if (parent_widget != nullptr) {
    // The same nudge GeneralDialog gives, so a wizard over its parent is not
    // mistaken for part of it.
    rect.moveCenter(
        QRect(anchor->mapToGlobal(QPoint(0, 0)), anchor->size()).center());
    rect.translate(24, 24);
  } else {
    rect.moveCenter(available.center());
  }

  setGeometry(ClampRectToAvailableGeometry(rect, available));
}

void GeneralWizard::save_geometry() {
  SettingsObject state_object(name_ + "_dialog_state");

  const auto rect = geometry();
  WindowStateSO state;
  state.x = rect.x();
  state.y = rect.y();
  state.width = rect.width();
  state.height = rect.height();
  state.window_save = true;

  state_object.Store(state.Json());
}

}  // namespace GpgFrontend::UI
