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

#include "LuaMounts.h"

#include <QCloseEvent>
#include <QVBoxLayout>

#include "sdk/GFSDKCommand.h"
#include "ui/lua/LuaHost.h"

namespace GpgFrontend::UI::Lua {

namespace {

auto Translate(const QString& source) -> QString {
  return QCoreApplication::translate("GTrC", source.toUtf8().constData());
}

/// Open dialogs, by module and mount id: one each. The mount id alone is only
/// unique within its module.
auto OpenDialogs() -> QHash<QString, QPointer<NativeDialog>>& {
  static QHash<QString, QPointer<NativeDialog>> dialogs;
  return dialogs;
}

}  // namespace

auto NativeDialogStateName(const QString& widget_id) -> QString {
  return QStringLiteral("native_dialog.") + widget_id;
}

NativeDialog::NativeDialog(const QString& mount_id, const QString& widget_id,
                           const QCborMap& args, QWidget* parent)
    : GeneralDialog(NativeDialogStateName(widget_id), parent),
      mount_id_(mount_id) {
  const auto instance =
      NativeInstances::Instance().Create(widget_id, args, this);
  if (!instance.has_value() || instance->kind != NativeWidgetKind::kDIALOG) {
    // The module built a widget; not mounted, it is the Host's to delete.
    if (instance.has_value()) {
      NativeInstances::Instance().Destroy(instance->id);
      delete instance->widget.data();
    }
    return;
  }
  instance_ = instance->id;

  const auto entry = NativeInstances::Instance().Entry(instance_);
  if (entry.has_value()) {
    setWindowTitle(Translate(entry->title));
  }
  // A module may build its widget as a QDialog; inside the Host's frame it
  // is an ordinary child, not a second window.
  instance->widget->setWindowFlags(Qt::Widget);
  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(instance->widget);

  // Such a QDialog still answers Escape, and its own Close button, with
  // done(), which for a widget that is no longer a window only hides it: the
  // frame would stay open around nothing. Its end is the frame's end, on the
  // same terms as the frame's own close button.
  if (auto* inner = qobject_cast<QDialog*>(instance->widget.data())) {
    const QPointer<QWidget> widget = instance->widget;
    connect(inner, &QDialog::finished, this, [this, widget](int) {
      if (closing_) return;
      if (CloseAllowed()) {
        closing_ = true;
        close();
      } else if (!widget.isNull()) {
        widget->show();  // refused: bring the content back
      }
    });
  }

  // The declared size is where a first open starts, never smaller than the
  // widget's own hint. With none declared the frame fits its content when it
  // is first shown. A remembered geometry overrides either.
  if (entry.has_value() && entry->width > 0 && entry->height > 0) {
    layout->activate();
    resize(QSize(entry->width, entry->height).expandedTo(sizeHint()));
  }

  if (entry.has_value() && entry->dialog.opened) {
    entry->dialog.opened(instance_, args);
  }
}

NativeDialog::~NativeDialog() {
  if (instance_ != 0) NativeInstances::Instance().Destroy(instance_);
}

auto NativeDialog::CloseAllowed() -> bool {
  if (closing_ || instance_ == 0) return true;
  const auto entry = NativeInstances::Instance().Entry(instance_);
  return !entry.has_value() || !entry->dialog.close_requested ||
         entry->dialog.close_requested(instance_);
}

void NativeDialog::closeEvent(QCloseEvent* event) {
  if (!CloseAllowed()) {
    event->ignore();
    return;
  }
  GeneralDialog::closeEvent(event);
}

void NativeDialog::reject() {
  if (CloseAllowed()) GeneralDialog::reject();
}

void NativeDialog::OnClose() {
  // The module asked: no second question.
  closing_ = true;
  close();
}

void NativeDialog::OnWithdrawn() {
  // The module is gone: nobody is left to ask whether it may close.
  instance_ = 0;
  closing_ = true;
  close();
}

NativeSettingsPage::NativeSettingsPage(const QString& widget_id,
                                       QWidget* parent)
    : QWidget(parent) {
  const auto instance = NativeInstances::Instance().Create(widget_id, {}, this);
  if (!instance.has_value() || instance->kind != NativeWidgetKind::kSETTINGS) {
    if (instance.has_value()) {
      NativeInstances::Instance().Destroy(instance->id);
      delete instance->widget.data();
    }
    return;
  }
  instance_ = instance->id;
  widget_ = instance->widget;
  instance->widget->setWindowFlags(Qt::Widget);
  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(instance->widget);
}

NativeSettingsPage::~NativeSettingsPage() {
  if (instance_ != 0) NativeInstances::Instance().Destroy(instance_);
}

void NativeSettingsPage::Load() {
  const auto entry = NativeInstances::Instance().Entry(instance_);
  if (entry.has_value() && entry->settings.load) {
    entry->settings.load(instance_);
  }
}

auto NativeSettingsPage::Apply() -> bool {
  const auto entry = NativeInstances::Instance().Entry(instance_);
  if (!entry.has_value() || !entry->settings.apply) return true;
  return entry->settings.apply(instance_);
}

void NativeSettingsPage::OnRestartNeeded(int level) {
  emit SignalRestartNeeded(level);
}

void NativeSettingsPage::OnWithdrawn() {
  instance_ = 0;
  delete widget_.data();
  if (layout() != nullptr) {
    layout()->addWidget(new QLabel(
        QCoreApplication::translate(
            "GpgFrontend::UI::Lua::NativeSettingsPage",
            "The module that provided this page is no longer active."),
        this));
  }
}

auto OpenDialogMount(const QString& module, const QString& view_id,
                     const QCborMap& args, QWidget* parent) -> int {
  if (module.isEmpty()) return GF_CMD_E_DENIED;
  auto* rt = LuaHost::Instance().Runtime(module);
  if (rt == nullptr) return GF_CMD_E_UNAVAILABLE;

  for (const auto& m : rt->Mounts()) {
    if (m.id != view_id) continue;
    if (m.kind != AnchorKind::kDIALOG) return GF_CMD_E_BAD_ARGS;

    auto& open = OpenDialogs();
    const auto key = module + QLatin1Char('/') + view_id;
    if (auto existing = open.value(key); !existing.isNull()) {
      existing->raise();
      existing->activateWindow();
      return GF_CMD_OK;
    }
    auto* dialog = new NativeDialog(m.id, m.widget, args, parent);
    if (!dialog->Ok()) {
      delete dialog;
      return GF_CMD_E_FAILED;
    }
    open.insert(key, dialog);
    dialog->show();
    return GF_CMD_OK;
  }
  // Not this module's mount -- whether or not somebody else has one by that
  // name, which a module cannot find out this way.
  return GF_CMD_E_DENIED;
}

auto BuildNativeSettingsPages() -> QList<NativeSettingsPageInfo> {
  QList<NativeSettingsPageInfo> pages;
  for (const auto& m : LuaHost::Instance().MountsOf(AnchorKind::kSETTINGS)) {
    const auto entry = NativeWidgetRegistry::Instance().Find(m.info.widget);
    if (!entry.has_value()) continue;
    auto* page = new NativeSettingsPage(m.info.widget);
    if (!page->Ok()) {
      delete page;
      continue;
    }
    page->Load();
    QStringList keywords;
    for (const auto& k : entry->keywords.split(',', Qt::SkipEmptyParts)) {
      keywords << Translate(k.trimmed());
    }
    pages.append({page, Translate(entry->title), m.info.section, keywords});
  }
  return pages;
}

}  // namespace GpgFrontend::UI::Lua
