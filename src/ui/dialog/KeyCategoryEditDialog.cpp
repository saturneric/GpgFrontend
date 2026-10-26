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

#include "KeyCategoryEditDialog.h"

#include "core/function/openpgp/KeyCategoryRepository.h"
#include "ui/UISignalStation.h"
#include "ui/dialog/KeyCategoryRules.h"
#include "ui/function/KeyDatabaseRefresh.h"
#include "ui/function/UIStyle.h"
#include "ui/widgets/KeyList.h"

namespace GpgFrontend::UI {

namespace {

constexpr int kSwatchSide = 28;
constexpr int kPreviewSide = 16;
constexpr auto kSwatchColorProperty = "gfSwatchColor";

auto MakeCanvas(int side) -> QPixmap {
  const qreal dpr = qApp->devicePixelRatio() > 0 ? qApp->devicePixelRatio() : 1;
  QPixmap pm(QSize(side, side) * dpr);
  pm.setDevicePixelRatio(dpr);
  pm.fill(Qt::transparent);
  return pm;
}

void PaintDisc(QPainter& p, const QRectF& r, const QColor& color) {
  // Same outline as the category rail's swatches.
  QColor border = color.darker(125);
  border.setAlpha(190);
  p.setPen(QPen(border, 1));
  p.setBrush(color);
  p.drawEllipse(r);
}

auto MakeSwatchIcon(const QColor& color) -> QIcon {
  QIcon icon;
  const QRectF canvas(0, 0, kSwatchSide, kSwatchSide);

  for (const bool on : {false, true}) {
    auto pm = MakeCanvas(kSwatchSide);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);

    if (on) {
      // A ring around a smaller disc marks the choice by itself, without
      // relying on how the platform style draws a checked tool button.
      p.setPen(QPen(color, 2));
      p.setBrush(Qt::NoBrush);
      p.drawEllipse(canvas.adjusted(2, 2, -2, -2));
      PaintDisc(p, canvas.adjusted(7, 7, -7, -7), color);
    } else {
      PaintDisc(p, canvas.adjusted(5, 5, -5, -5), color);
    }

    p.end();
    icon.addPixmap(pm, QIcon::Normal, on ? QIcon::On : QIcon::Off);
  }

  return icon;
}

auto MakeHueWheel(const QRectF& r) -> QConicalGradient {
  QConicalGradient wheel(r.center(), 90);
  constexpr int kHueSteps = 6;
  for (int i = 0; i <= kHueSteps; ++i) {
    wheel.setColorAt(static_cast<qreal>(i) / kHueSteps,
                     QColor::fromHsv((i * 360 / kHueSteps) % 360, 170, 235));
  }
  return wheel;
}

auto MakeCustomSwatchIcon(const QColor& color) -> QIcon {
  QIcon icon;
  const QRectF canvas(0, 0, kSwatchSide, kSwatchSide);

  for (const bool on : {false, true}) {
    auto pm = MakeCanvas(kSwatchSide);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);

    if (on && color.isValid()) {
      // Selected: the hue wheel becomes the selection ring the presets draw
      // in their own colour, around the chosen colour.
      const auto ring = canvas.adjusted(2, 2, -2, -2);
      p.setPen(QPen(QBrush(MakeHueWheel(ring)), 2));
      p.setBrush(Qt::NoBrush);
      p.drawEllipse(ring);
      PaintDisc(p, canvas.adjusted(7, 7, -7, -7), color);
    } else {
      // A hue wheel reads as "any colour"; once one was picked it shows in
      // the middle, so the slot remembers it.
      const auto disc = canvas.adjusted(5, 5, -5, -5);
      p.setPen(Qt::NoPen);
      p.setBrush(MakeHueWheel(disc));
      p.drawEllipse(disc);
      if (color.isValid()) PaintDisc(p, canvas.adjusted(8, 8, -8, -8), color);
    }

    p.end();
    icon.addPixmap(pm, QIcon::Normal, on ? QIcon::On : QIcon::Off);
  }

  return icon;
}

auto MakePreviewPixmap(const QColor& color) -> QPixmap {
  auto pm = MakeCanvas(kPreviewSide);
  QPainter p(&pm);
  p.setRenderHint(QPainter::Antialiasing, true);
  // Custom categories are circles in the rail.
  PaintDisc(p, QRectF(0.5, 0.5, kPreviewSide - 1, kPreviewSide - 1), color);
  p.end();
  return pm;
}

}  // namespace

KeyCategoryEditDialog::KeyCategoryEditDialog(int channel, QString category_id,
                                             QStringList key_ids,
                                             QList<QColor> colors_in_use,
                                             QWidget* parent)
    : GeneralDialog("KeyCategoryEditDialog", parent),
      current_gpg_context_channel_(channel),
      category_id_(std::move(category_id)),
      key_ids_(std::move(key_ids)) {
  // A reload of the key databases replaces the channel this works on; find
  // the database again, or say it is gone.
  KeepBoundToKeyDatabase(
      this, [this]() { return current_gpg_context_channel_; },
      [this](int channel) { current_gpg_context_channel_ = channel; });

  auto& repo = KeyCategoryRepository::GetInstance(current_gpg_context_channel_);

  // The custom categories' colours come from the repository too, so a list
  // that does not show every category still counts them.
  auto used_colors = std::move(colors_in_use);
  for (const auto& c : repo.Fetch()) {
    if (c.builtin) continue;
    if (c.id == category_id_) {
      initial_name_ = c.name;
      continue;
    }
    taken_names_.append(c.name);
    if (QColor color(repo.GetTabColor(c.id)); color.isValid()) {
      used_colors.append(color);
    }
  }

  if (is_editing()) {
    initial_color_ = QColor(repo.GetTabColor(category_id_));
    if (!initial_color_.isValid()) {
      initial_color_ = CategoryFallbackColor(category_id_);
    }
  } else {
    initial_color_ = SuggestKeyCategoryColor(used_colors);
  }

  setWindowTitle(is_editing() ? tr("Edit Category") : tr("New Category"));
  setModal(true);
  setMinimumWidth(420);

  auto* title_label = new QLabel(
      is_editing() ? tr("Edit Key Category") : tr("Create a Key Category"),
      this);
  auto title_font = title_label->font();
  title_font.setBold(true);
  title_label->setFont(title_font);

  auto* desc_label = new QLabel(
      tr("Categories sort keys into your own groups in the key list. A key can "
         "belong to several categories."),
      this);
  desc_label->setWordWrap(true);
  muted_labels_.append(desc_label);

  // How the category will look in the category list.
  auto* preview = new QFrame(this);
  preview_ = preview;
  preview->setObjectName(u"KeyCategoryPreview"_s);
  preview->setAttribute(Qt::WA_StyledBackground, true);
  preview_icon_ = new QLabel(preview);
  preview_icon_->setFixedSize(kPreviewSide, kPreviewSide);
  preview_name_ = new QLabel(preview);
  preview_name_->setTextFormat(Qt::PlainText);
  preview_name_->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);

  auto* preview_layout = new QHBoxLayout(preview);
  preview_layout->setContentsMargins(12, 10, 12, 10);
  preview_layout->setSpacing(10);
  preview_layout->addWidget(preview_icon_);
  preview_layout->addWidget(preview_name_, 1);

  name_edit_ = new QLineEdit(this);
  name_edit_->setPlaceholderText(tr("e.g. Work Contacts"));
  name_edit_->setText(initial_name_);
  name_edit_->setClearButtonEnabled(true);

  // Colour swatches.
  swatch_group_ = new QButtonGroup(this);
  swatch_group_->setExclusive(true);

  auto* swatches = new QWidget(this);
  swatches->setStyleSheet(R"(
QToolButton {
  border: none;
  background: transparent;
  padding: 0;
}
)");

  const auto make_swatch = [&](const QColor& color, const QString& name) {
    auto* button = new QToolButton(swatches);
    button->setCheckable(true);
    button->setAutoRaise(true);
    button->setCursor(Qt::PointingHandCursor);
    button->setIconSize(QSize(kSwatchSide, kSwatchSide));
    button->setFixedSize(kSwatchSide + 2, kSwatchSide + 2);
    button->setIcon(MakeSwatchIcon(color));
    button->setToolTip(name);
    button->setAccessibleName(name);
    button->setProperty(kSwatchColorProperty, color);
    swatch_group_->addButton(button);
    return button;
  };

  auto* swatch_grid = new QGridLayout();
  swatch_grid->setContentsMargins(0, 0, 0, 0);
  swatch_grid->setSpacing(2);

  // Seven columns: the twenty presets and the custom slot fill three rows.
  constexpr int kSwatchColumns = 7;
  const auto presets = KeyCategoryPresetColors();
  for (std::size_t i = 0; i < presets.size(); ++i) {
    const auto index = static_cast<int>(i);
    swatch_grid->addWidget(make_swatch(presets.at(i).color, presets.at(i).name),
                           index / kSwatchColumns, index % kSwatchColumns);
  }

  // The last slot opens the colour picker and then holds what was picked.
  custom_swatch_ = make_swatch(QColor(), tr("Custom Colour"));
  custom_swatch_->setIcon(MakeCustomSwatchIcon(QColor()));
  custom_swatch_->setToolTip(tr("Custom Colour..."));
  const auto custom_index = static_cast<int>(presets.size());
  swatch_grid->addWidget(custom_swatch_, custom_index / kSwatchColumns,
                         custom_index % kSwatchColumns);

  // Keep the swatches packed; a wide dialog would otherwise spread them out.
  swatch_grid->setColumnStretch(kSwatchColumns, 1);

  swatches->setLayout(swatch_grid);

  auto* form_layout = new QFormLayout();
  form_layout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
  form_layout->setLabelAlignment(Qt::AlignLeft | Qt::AlignTop);
  form_layout->setHorizontalSpacing(12);
  form_layout->setVerticalSpacing(10);
  form_layout->addRow(tr("Name"), name_edit_);
  form_layout->addRow(tr("Colour"), swatches);

  error_label_ = new QLabel(this);
  error_label_->setWordWrap(true);

  auto* button_box = new QDialogButtonBox(
      QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
  save_button_ = button_box->button(QDialogButtonBox::Ok);
  save_button_->setText(is_editing() ? tr("Save") : tr("Create"));
  button_box->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));

  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(14, 14, 14, 12);
  layout->setSpacing(10);
  layout->addWidget(title_label);
  layout->addWidget(desc_label);
  layout->addWidget(preview);
  layout->addLayout(form_layout);

  if (!is_editing() && !key_ids_.isEmpty()) {
    const int count = static_cast<int>(key_ids_.size());
    auto* members_label = new QLabel(
        tr("%n selected key(s) will be added to this category.", "", count),
        this);
    members_label->setWordWrap(true);
    muted_labels_.append(members_label);
    layout->addWidget(members_label);
  }

  layout->addWidget(error_label_);
  layout->addStretch(1);
  layout->addWidget(button_box);

  connect(button_box, &QDialogButtonBox::accepted, this,
          &KeyCategoryEditDialog::slot_save);
  connect(button_box, &QDialogButtonBox::rejected, this, &QDialog::reject);
  connect(name_edit_, &QLineEdit::textChanged, this,
          &KeyCategoryEditDialog::update_state);
  connect(
      swatch_group_, &QButtonGroup::buttonClicked, this,
      [this](QAbstractButton* button) {
        if (button == custom_swatch_) {
          slot_choose_custom_color();
          return;
        }
        select_color(button->property(kSwatchColorProperty).value<QColor>());
      });

  this->setAttribute(Qt::WA_DeleteOnClose, true);

  apply_theme();
  select_color(initial_color_);
  name_edit_->setFocus();
  name_edit_->selectAll();
}

auto KeyCategoryEditDialog::is_editing() const -> bool {
  return !category_id_.isEmpty();
}

void KeyCategoryEditDialog::select_color(const QColor& color) {
  if (!color.isValid()) return;

  QAbstractButton* match = nullptr;
  for (auto* button : swatch_group_->buttons()) {
    if (button == custom_swatch_) continue;
    if (button->property(kSwatchColorProperty).value<QColor>().rgb() ==
        color.rgb()) {
      match = button;
      break;
    }
  }

  if (match == nullptr) {
    custom_swatch_->setIcon(MakeCustomSwatchIcon(color));
    custom_swatch_->setProperty(kSwatchColorProperty, color);
    custom_swatch_->setToolTip(tr("Custom Colour (%1)...").arg(color.name()));
    match = custom_swatch_;
  }

  match->setChecked(true);
  selected_color_ = color;
  update_state();
}

void KeyCategoryEditDialog::slot_choose_custom_color() {
  const QColor picked = QColorDialog::getColor(selected_color_, this,
                                               tr("Choose Category Colour"));
  // Clicking the slot already checked it; on cancel, put the check back on
  // the colour that is still in effect.
  select_color(picked.isValid() ? picked : selected_color_);
}

void KeyCategoryEditDialog::update_state() {
  const auto name = name_edit_->text().trimmed();
  const auto problem = ValidateKeyCategoryName(name, taken_names_);

  // Stay quiet until there is something to type into: an error shown before
  // the first keystroke reads as a complaint about opening the dialog.
  error_label_->setText(name_edit_->text().isEmpty()
                            ? QString{}
                            : DescribeKeyCategoryNameProblem(problem));

  const bool changed = !is_editing() || name != initial_name_ ||
                       selected_color_.rgb() != initial_color_.rgb();
  save_button_->setEnabled(problem == KeyCategoryNameProblem::kNone && changed);

  preview_icon_->setPixmap(MakePreviewPixmap(selected_color_));
  preview_name_->setText(name.isEmpty() ? tr("Category Name") : name);
  SetLabelTextColor(preview_name_, name.isEmpty()
                                       ? MutedTextColor(palette())
                                       : palette().color(QPalette::WindowText));
}

void KeyCategoryEditDialog::apply_theme() {
  if (preview_ == nullptr || error_label_ == nullptr) return;

  for (auto* label : muted_labels_) {
    SetLabelTextColor(label, MutedTextColor(palette()));
  }
  SetLabelTextColor(error_label_, DangerColor(palette()));

  // A stylesheet keeps fixed colours, so the frame is re-styled on every
  // palette change rather than once.
  preview_->setStyleSheet(QString(R"(
QFrame#KeyCategoryPreview {
  background: palette(base);
  border: 1px solid %1;
  border-radius: 6px;
}
)")
                              .arg(BorderColor(palette()).name()));

  if (selected_color_.isValid()) update_state();
}

void KeyCategoryEditDialog::changeEvent(QEvent* event) {
  GeneralDialog::changeEvent(event);

  // The secondary colours are mixed from the palette, so a theme change has
  // to be painted rather than waited out.
  if (event->type() == QEvent::PaletteChange) apply_theme();
}

void KeyCategoryEditDialog::showEvent(QShowEvent* event) {
  GeneralDialog::showEvent(event);
  if (!isRectRestored()) movePosition2CenterOfParent();
}

void KeyCategoryEditDialog::slot_save() {
  const auto name = name_edit_->text().trimmed();
  const auto problem = ValidateKeyCategoryName(name, taken_names_);
  if (problem != KeyCategoryNameProblem::kNone) {
    error_label_->setText(DescribeKeyCategoryNameProblem(problem));
    return;
  }

  auto& repo = KeyCategoryRepository::GetInstance(current_gpg_context_channel_);
  auto id = category_id_;

  if (is_editing()) {
    const bool renamed = name != initial_name_;
    const bool recolored = selected_color_.rgb() != initial_color_.rgb();

    if (renamed) repo.Rename(id, name);
    if (recolored) repo.SetTabColor(id, selected_color_.name());

    if (renamed) {
      emit UISignalStation::GetInstance() -> SignalKeyCategoriesChanged();
    }
    if (recolored) {
      emit UISignalStation::GetInstance() -> SignalKeyCategoryColorsChanged();
    }
  } else {
    id = repo.AddCategory(name, selected_color_.name());
    for (const auto& key_id : key_ids_) repo.AddKey2Category(id, key_id);
    emit UISignalStation::GetInstance() -> SignalKeyCategoriesChanged();
  }

  emit SignalSaved(id);
  this->close();
}

}  // namespace GpgFrontend::UI
