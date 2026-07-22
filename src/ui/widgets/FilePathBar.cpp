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

#include "ui/widgets/FilePathBar.h"

namespace {

auto NormalizeUserPath(QString path, const QString& current_path) -> QString {
  path = path.trimmed();

  if (path.isEmpty()) {
    return current_path;
  }

  if (path == "~") {
    return QDir::homePath();
  }

  if (path.startsWith("~/") || path.startsWith("~\\")) {
    return QDir::homePath() + path.mid(1);
  }

  QFileInfo info(path);
  if (info.isRelative()) {
    return QDir(current_path).absoluteFilePath(path);
  }

  return path;
}

auto ToDisplayUserPath(const QString& path) -> QString {
  auto clean_path = QDir::cleanPath(path);

#ifdef Q_OS_WIN
  clean_path.replace("\\", "/");
#endif

  const auto home_path = QDir::cleanPath(QDir::homePath());

  if (clean_path == home_path) {
    return QStringLiteral("~");
  }

  if (clean_path.startsWith(home_path + "/")) {
    return QStringLiteral("~") + clean_path.mid(home_path.size());
  }

  return clean_path;
}

using GpgFrontend::UI::PathSegment;

auto SplitPathSegments(const QString& path)
    -> GpgFrontend::QContainer<PathSegment> {
  GpgFrontend::QContainer<PathSegment> segments;

  auto clean = QDir::cleanPath(path);
#ifdef Q_OS_WIN
  clean.replace("\\", "/");
#endif

  if (clean.isEmpty()) return segments;

  const auto home = QDir::cleanPath(QDir::homePath());
  const bool under_home = clean == home || clean.startsWith(home + "/");

  QString base;
  QString remainder;

  if (under_home) {
    base = home;
    remainder = clean.mid(home.size());
    segments.append({QObject::tr("Home"), home, true});
  } else if (clean.startsWith("/")) {
    base = "/";
    remainder = clean.mid(1);
    segments.append({QStringLiteral("/"), QStringLiteral("/"), false});
  } else {
    // Windows volume roots such as "C:/".
    const auto slash = clean.indexOf('/');
    const auto volume = slash < 0 ? clean : clean.left(slash);
    base = volume + "/";
    remainder = slash < 0 ? QString() : clean.mid(slash + 1);
    segments.append({volume, base, false});
  }

  auto walking = base;
  for (const auto& part : remainder.split('/', Qt::SkipEmptyParts)) {
    walking = QDir(walking).absoluteFilePath(part);
    segments.append({part, QDir::cleanPath(walking), false});
  }

  return segments;
}

}  // namespace

namespace GpgFrontend::UI {

FilePathBar::FilePathBar(QWidget* parent) : QWidget(parent) {
  stack_ = new QStackedLayout(this);
  stack_->setContentsMargins(0, 0, 0, 0);
  stack_->setStackingMode(QStackedLayout::StackOne);

  crumb_page_ = new QWidget(this);
  crumb_layout_ = new QHBoxLayout(crumb_page_);
  crumb_layout_->setContentsMargins(2, 0, 2, 0);
  crumb_layout_->setSpacing(1);
  crumb_layout_->addStretch(1);

  path_edit_ = new QLineEdit(this);
  path_edit_->setClearButtonEnabled(true);
  path_edit_->setPlaceholderText(tr("Type a folder path, e.g. ~/Documents"));
  path_edit_->setToolTip(
      tr("Type a folder path and press Enter. \"~\" stands for your home "
         "folder, and a relative path is resolved against the current one. "
         "Press Escape to go back to the path buttons."));
  path_edit_->addAction(
      QIcon::fromTheme(QStringLiteral("folder"), QIcon(":/icons/folder.png")),
      QLineEdit::LeadingPosition);
  path_edit_->installEventFilter(this);

  path_complete_model_ = new QStringListModel(this);

  path_edit_completer_ = new QCompleter(path_complete_model_, this);
  path_edit_completer_->setCaseSensitivity(Qt::CaseInsensitive);
  path_edit_completer_->setCompletionMode(QCompleter::PopupCompletion);
  path_edit_completer_->setFilterMode(Qt::MatchStartsWith);
  path_edit_completer_->setMaxVisibleItems(12);

  path_edit_->setCompleter(path_edit_completer_);

  stack_->addWidget(crumb_page_);
  stack_->addWidget(path_edit_);
  stack_->setCurrentWidget(crumb_page_);

  setMinimumHeight(30);
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  setCursor(Qt::IBeamCursor);
  setToolTip(
      tr("Click a folder in the path to go there, or click the edit button to "
         "type a path (Ctrl+L)."));

  connect(path_edit_, &QLineEdit::returnPressed, this,
          &FilePathBar::commit_edit);
  connect(path_edit_, &QLineEdit::textEdited, this,
          [this](const QString& text) { update_path_completion(text); });
  connect(path_edit_completer_,
          QOverload<const QString&>::of(&QCompleter::activated), this,
          [this](const QString& path) {
            path_edit_->setText(ToDisplayUserPath(path));
            path_edit_->setCursorPosition(path_edit_->text().size());
          });

  auto* edit_shortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_L), this);
  edit_shortcut->setContext(Qt::WindowShortcut);
  connect(edit_shortcut, &QShortcut::activated, this, &FilePathBar::BeginEdit);

  auto* edit_shortcut_alt = new QShortcut(QKeySequence(Qt::Key_F6), this);
  edit_shortcut_alt->setContext(Qt::WidgetWithChildrenShortcut);
  connect(edit_shortcut_alt, &QShortcut::activated, this,
          &FilePathBar::BeginEdit);
}

void FilePathBar::SetPath(const QString& path) {
  current_path_ = QDir::cleanPath(path);

  path_edit_->setText(ToDisplayUserPath(current_path_));
  path_edit_->setToolTip(current_path_);

  rebuild_crumbs();
  EndEdit();
}

auto FilePathBar::GetPath() const -> QString { return current_path_; }

void FilePathBar::BeginEdit() {
  path_edit_->setText(ToDisplayUserPath(current_path_));
  stack_->setCurrentWidget(path_edit_);
  path_edit_->setFocus(Qt::ShortcutFocusReason);
  path_edit_->selectAll();
}

void FilePathBar::EndEdit() {
  if (stack_->currentWidget() == crumb_page_) return;

  path_edit_->setText(ToDisplayUserPath(current_path_));
  path_edit_->setToolTip(current_path_);
  stack_->setCurrentWidget(crumb_page_);
}

void FilePathBar::ShowPathError(const QString& message) {
  stack_->setCurrentWidget(path_edit_);
  path_edit_->setToolTip(message);
  path_edit_->setFocus();
  path_edit_->selectAll();

  QToolTip::showText(path_edit_->mapToGlobal(QPoint(0, path_edit_->height())),
                     message, path_edit_);
}

void FilePathBar::mousePressEvent(QMouseEvent* event) {
  if (event->button() == Qt::LeftButton &&
      stack_->currentWidget() == crumb_page_) {
    BeginEdit();
    event->accept();
    return;
  }

  QWidget::mousePressEvent(event);
}

void FilePathBar::resizeEvent(QResizeEvent* event) {
  QWidget::resizeEvent(event);
  rebuild_crumbs();
}

auto FilePathBar::eventFilter(QObject* watched, QEvent* event) -> bool {
  if (watched == path_edit_) {
    if (event->type() == QEvent::KeyPress) {
      auto* key_event = dynamic_cast<QKeyEvent*>(event);
      if (key_event != nullptr && key_event->key() == Qt::Key_Escape) {
        EndEdit();
        return true;
      }
    }

    if (event->type() == QEvent::FocusOut) {
      // The completer popup takes focus while it is open; leaving edit mode
      // there would close the popup the user is picking from.
      if (path_edit_completer_ == nullptr ||
          path_edit_completer_->popup() == nullptr ||
          !path_edit_completer_->popup()->isVisible()) {
        EndEdit();
      }
    }
  }

  return QWidget::eventFilter(watched, event);
}

void FilePathBar::clear_crumbs() {
  while (crumb_layout_->count() > 0) {
    auto* item = crumb_layout_->takeAt(0);
    if (item->widget() != nullptr) item->widget()->deleteLater();
    delete item;
  }
}

auto FilePathBar::make_crumb_button(const QString& text, const QString& path,
                                    bool current) -> QToolButton* {
  auto* button = new QToolButton(crumb_page_);
  button->setAutoRaise(true);
  button->setFocusPolicy(Qt::NoFocus);
  button->setToolButtonStyle(Qt::ToolButtonTextOnly);
  button->setText(text);
  button->setToolTip(path);
  button->setCursor(Qt::PointingHandCursor);

  if (current) {
    // The folder the view is showing carries the weight; everything before it
    // is navigation.
    auto font = button->font();
    font.setBold(true);
    button->setFont(font);
    button->setCursor(Qt::ArrowCursor);
  }

  // QToolButton hugs its text, which leaves crumbs colliding with the
  // separators around them.
  button->setMinimumHeight(26);
  button->setMinimumWidth(button->fontMetrics().horizontalAdvance(text) +
                          kCrumbPadding);

  connect(button, &QToolButton::clicked, this,
          [this, path]() { emit SignalPathRequested(path); });

  return button;
}

auto FilePathBar::make_chevron() -> QLabel* {
  auto* label = new QLabel(QStringLiteral("›"), crumb_page_);
  label->setForegroundRole(QPalette::Dark);
  label->setAlignment(Qt::AlignCenter);
  label->setFixedWidth(kChevronWidth);
  return label;
}

void FilePathBar::rebuild_crumbs() {
  clear_crumbs();

  const auto segments = SplitPathSegments(current_path_);
  if (segments.isEmpty()) {
    crumb_layout_->addStretch(1);
    crumb_layout_->addWidget(make_edit_button());
    return;
  }

  const auto home_icon = QIcon::fromTheme(QStringLiteral("user-home"),
                                          QIcon(":/icons/folder.png"));

  QContainer<QToolButton*> buttons;
  buttons.reserve(static_cast<int>(segments.size()));

  const auto chevron_width = make_chevron_width();
  auto wanted_width = 0;

  for (auto i = 0; i < segments.size(); ++i) {
    const auto& segment = segments.at(i);
    const bool current = i == segments.size() - 1;

    auto* button = make_crumb_button(segment.label, segment.path, current);
    if (segment.home) {
      button->setIcon(home_icon);
      button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    }

    buttons.append(button);
    wanted_width += button->sizeHint().width() + (i > 0 ? chevron_width : 0);
  }

  // Drop leading segments into an overflow menu rather than let a deep path
  // push the whole toolbar wider than the window. The current folder always
  // stays visible, and so does the edit button.
  const auto available = width() - kEditHitAreaWidth - kEditButtonSide;

  auto first_visible = 0;
  while (available > 0 && wanted_width > available &&
         first_visible < buttons.size() - 1) {
    wanted_width -= buttons.at(first_visible)->sizeHint().width();
    wanted_width -= chevron_width;
    ++first_visible;
  }

  if (first_visible > 0) {
    crumb_layout_->addWidget(make_overflow_button(segments, first_visible));
    crumb_layout_->addWidget(make_chevron());
  }

  for (auto i = 0; i < buttons.size(); ++i) {
    if (i < first_visible) {
      buttons.at(i)->deleteLater();
      continue;
    }

    if (i > first_visible) crumb_layout_->addWidget(make_chevron());
    crumb_layout_->addWidget(buttons.at(i));
  }

  crumb_layout_->addStretch(1);
  crumb_layout_->addWidget(make_edit_button());
}

auto FilePathBar::make_edit_button() -> QToolButton* {
  // Clicking the empty area works only while there is empty area left, which a
  // long path takes away, and the keyboard shortcut is invisible. This button
  // keeps typing a path reachable in every case.
  auto* button = new QToolButton(crumb_page_);
  button->setAutoRaise(true);
  button->setFocusPolicy(Qt::NoFocus);
  button->setCursor(Qt::PointingHandCursor);
  const auto icon = QIcon(":/icons/pencil.png");

  if (icon.isNull()) {
    // A blank button would hide the only always-available way in.
    button->setText(QStringLiteral("✎"));
  } else {
    button->setIcon(icon);
    button->setIconSize(QSize(14, 14));
  }

  button->setFixedSize(kEditButtonSide, kEditButtonSide);
  button->setToolTip(tr("Type a path instead (Ctrl+L)"));

  connect(button, &QToolButton::clicked, this, &FilePathBar::BeginEdit);

  return button;
}

auto FilePathBar::make_chevron_width() -> int {
  auto* probe = make_chevron();
  const auto probe_width = probe->sizeHint().width();
  delete probe;
  return probe_width;
}

auto FilePathBar::make_overflow_button(const QContainer<PathSegment>& segments,
                                       int count) -> QToolButton* {
  auto* overflow = new QToolButton(crumb_page_);
  overflow->setAutoRaise(true);
  overflow->setFocusPolicy(Qt::NoFocus);
  overflow->setText(QStringLiteral("…"));
  overflow->setToolTip(tr("Show parent folders"));
  overflow->setPopupMode(QToolButton::InstantPopup);
  overflow->setCursor(Qt::PointingHandCursor);
  overflow->setMinimumHeight(26);
  overflow->setMinimumWidth(kChevronWidth + kCrumbPadding);

  auto* menu = new QMenu(overflow);
  for (auto i = 0; i < count && i < segments.size(); ++i) {
    const auto path = segments.at(i).path;
    auto* action = menu->addAction(segments.at(i).label);
    action->setToolTip(path);
    connect(action, &QAction::triggered, this,
            [this, path]() { emit SignalPathRequested(path); });
  }
  overflow->setMenu(menu);

  return overflow;
}

void FilePathBar::commit_edit() {
  const auto path = NormalizeUserPath(path_edit_->text(), current_path_);
  emit SignalPathRequested(QDir::cleanPath(path));
}

void FilePathBar::update_path_completion(const QString& input) {
  if (input.trimmed().isEmpty()) {
    path_complete_model_->setStringList({});
    return;
  }

  QString typed_prefix;

  const auto normalized_input = NormalizeUserPath(input, current_path_);

  QFileInfo normalized_info(normalized_input);

  QDir base_dir;
  if (normalized_info.exists() && normalized_info.isDir()) {
    base_dir = QDir(normalized_info.absoluteFilePath());
    typed_prefix.clear();
  } else {
    base_dir = normalized_info.dir();
    typed_prefix = normalized_info.fileName();
  }

  if (!base_dir.exists() || !base_dir.isReadable()) {
    path_complete_model_->setStringList({});
    return;
  }

  const auto entries =
      base_dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::Readable,
                             QDir::Name | QDir::IgnoreCase | QDir::DirsFirst);

  QStringList candidates;
  candidates.reserve(entries.size());

  for (const auto& entry : entries) {
    const auto file_name = entry.fileName();

    if (!typed_prefix.isEmpty() &&
        !file_name.startsWith(typed_prefix, Qt::CaseInsensitive)) {
      continue;
    }

    auto candidate_path = QDir::cleanPath(entry.absoluteFilePath());

#ifdef Q_OS_WIN
    candidate_path.replace("\\", "/");
#endif

    if (input.trimmed().startsWith("~")) {
      candidates.append(ToDisplayUserPath(candidate_path));
    } else if (QFileInfo(input).isRelative() && !input.startsWith("/") &&
               !input.startsWith("\\")) {
      candidates.append(QDir(current_path_).relativeFilePath(candidate_path));
    } else {
      candidates.append(candidate_path);
    }
  }

  path_complete_model_->setStringList(candidates);

  if (!candidates.isEmpty()) {
    path_edit_completer_->setCompletionPrefix(input);
    path_edit_completer_->complete();
  }
}

}  // namespace GpgFrontend::UI
