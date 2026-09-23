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

#include "LuaPlacements.h"

#include <QBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMenu>
#include <QPushButton>

#include "core/utils/RustUtils.h"
#include "ui/command/CommandRegistry.h"
#include "ui/lua/LuaHost.h"
#include "ui/widgets/PlainTextEditorPage.h"
#include "ui/widgets/TextEdit.h"
#include "ui/widgets/TextEditTabWidget.h"

namespace GpgFrontend::UI::Lua {

namespace {

/// Past this, "is it OpenPGP" is not worth reading a document to answer on
/// the GUI thread before a menu opens.
constexpr qsizetype kOpenPgpProbeCap = 16 * 1024 * 1024;

auto HasOpenPgpStructure(const QByteArray& bytes) -> bool {
  if (bytes.isEmpty() || bytes.size() > kOpenPgpProbeCap) return false;
  const auto json = InspectOpenPGPData(GFBuffer(bytes));
  const auto doc = QJsonDocument::fromJson(json).object();
  for (const auto& block : doc.value("blocks").toArray()) {
    if (!block.toObject().value("packets").toArray().isEmpty()) return true;
  }
  return !doc.value("packets").toArray().isEmpty();
}

/// One answer per document revision: menus open far more often than
/// documents change.
auto CachedHasOpenPgp(PlainTextEditorPage* page) -> bool {
  static QHash<qint64, QPair<int, bool>> cache;
  const auto id = TextEditTabWidget::DocumentIdOf(page);
  const auto revision = page->GetTextPage()->document()->revision();
  const auto it = cache.constFind(id);
  if (it != cache.constEnd() && it->first == revision) return it->second;
  const auto answer = HasOpenPgpStructure(page->DocumentBytes());
  cache.insert(id, {revision, answer});
  return answer;
}

auto Title(const QString& command) -> QString {
  const auto d = CommandRegistry::Instance().Describe(command);
  return d.has_value() ? CommandTitle(*d) : command;
}

auto Tooltip(const QString& command) -> QString {
  const auto d = CommandRegistry::Instance().Describe(command);
  return d.has_value() ? CommandDescription(*d) : QString();
}

auto Category(const QString& command) -> QString {
  const auto d = CommandRegistry::Instance().Describe(command);
  return d.has_value() ? CommandCategory(*d) : QString();
}

/// A Host action standing for one module action. Holds ids, not pointers.
auto MakeAction(const PlacedAction& placed, QObject* parent,
                const ContextFn& ctx) -> QAction* {
  auto* action = new QAction(Title(placed.info.command), parent);
  action->setToolTip(Tooltip(placed.info.command));
  action->setStatusTip(Tooltip(placed.info.command));
  if (!placed.info.icon.isEmpty()) action->setIcon(QIcon(placed.info.icon));
  action->setProperty("gf_lua_action", placed.info.id);

  QPointer<LuaModuleRuntime> runtime = placed.runtime;
  const auto id = placed.info.id;
  QObject::connect(action, &QAction::triggered, action, [runtime, id, ctx]() {
    if (runtime.isNull()) return;  // its module is gone: nothing to run
    runtime->Trigger(id, ctx());
  });
  return action;
}

void Refresh(QAction* action, const PlacedAction& placed,
             const UiContext& ctx) {
  if (placed.runtime.isNull()) {
    action->setVisible(false);
    return;
  }
  const auto st = placed.runtime->Evaluate(placed.info.id, ctx);
  action->setVisible(st.visible);
  action->setEnabled(st.enabled);
  action->setCheckable(st.has_checked);
  if (st.has_checked) action->setChecked(st.checked);
}

/// Owns a menu's module actions for one anchor.
class MenuBinder : public QObject {
 public:
  MenuBinder(QString anchor, QMenu* menu, ContextFn ctx)
      : QObject(menu), anchor_(std::move(anchor)), menu_(menu),
        ctx_(std::move(ctx)) {
    connect(&LuaHost::Instance(), &LuaHost::SignalChanged, this,
            [this]() { Rebuild(); }, Qt::QueuedConnection);
    connect(menu_, &QMenu::aboutToShow, this, [this]() { RefreshAll(); });
    Rebuild();
  }

 private:
  void Rebuild() {
    for (auto& a : actions_) delete a.data();
    actions_.clear();
    delete separator_.data();

    placed_ = LuaHost::Instance().ActionsOn(anchor_);
    if (placed_.isEmpty()) return;
    separator_ = menu_->addSeparator();
    for (const auto& p : placed_) {
      auto* action = MakeAction(p, menu_, ctx_);
      menu_->addAction(action);
      actions_.append(action);
    }
    RefreshAll();
  }

  void RefreshAll() {
    const auto ctx = ctx_();
    for (int i = 0; i < actions_.size() && i < placed_.size(); ++i) {
      if (!actions_[i].isNull()) Refresh(actions_[i], placed_[i], ctx);
    }
  }

  QString anchor_;
  QMenu* menu_;
  ContextFn ctx_;
  QList<PlacedAction> placed_;
  QList<QPointer<QAction>> actions_;
  QPointer<QAction> separator_;
};

}  // namespace

void LuaPlacements::AttachMenu(const QString& anchor, QMenu* menu,
                               ContextFn ctx) {
  if (menu == nullptr) return;
  new MenuBinder(anchor, menu, std::move(ctx));
}

void LuaPlacements::PopulateMenu(const QString& anchor, QMenu* menu,
                                 const ContextFn& ctx) {
  const auto placed = LuaHost::Instance().ActionsOn(anchor);
  if (menu == nullptr || placed.isEmpty()) return;
  const auto now = ctx();
  menu->addSeparator();
  for (const auto& p : placed) {
    auto* action = MakeAction(p, menu, ctx);
    Refresh(action, p, now);
    menu->addAction(action);
  }
}

void LuaPlacements::BuildButtons(const QString& anchor, QBoxLayout* layout,
                                 ContextFn ctx) {
  const auto placed = LuaHost::Instance().ActionsOn(anchor);
  if (layout == nullptr || placed.isEmpty()) return;
  const auto now = ctx();

  // One button per category; a category with several actions is a button
  // with a menu, the way the key details dialog always showed them.
  QMap<QString, QList<PlacedAction>> by_category;
  QStringList order;
  for (const auto& p : placed) {
    const auto category = Category(p.info.command);
    if (!by_category.contains(category)) order.append(category);
    by_category[category].append(p);
  }

  for (const auto& category : order) {
    const auto& group = by_category[category];
    auto* button = new QPushButton();
    if (group.size() == 1 && category.isEmpty()) {
      auto* action = MakeAction(group.first(), button, ctx);
      Refresh(action, group.first(), now);
      button->setText(action->text());
      button->setToolTip(action->toolTip());
      button->setEnabled(action->isEnabled());
      button->setVisible(action->isVisible());
      QObject::connect(button, &QPushButton::clicked, action,
                       &QAction::trigger);
    } else {
      button->setText(category.isEmpty() ? Title(group.first().info.command)
                                         : category);
      auto* menu = new QMenu(button);
      bool any = false;
      for (const auto& p : group) {
        auto* action = MakeAction(p, menu, ctx);
        Refresh(action, p, now);
        any = any || action->isVisible();
        menu->addAction(action);
      }
      button->setMenu(menu);
      button->setVisible(any);
      QObject::connect(menu, &QMenu::aboutToShow, menu, [menu, group, ctx]() {
        const auto fresh = ctx();
        const auto actions = menu->actions();
        for (int i = 0; i < actions.size() && i < group.size(); ++i) {
          Refresh(actions[i], group[i], fresh);
        }
      });
    }
    layout->addWidget(button);
  }
}

auto LuaPlacements::EditorContext(TextEdit* edit) -> UiContext {
  return PageContext(edit == nullptr ? nullptr : edit->CurTextPage());
}

void LuaPlacements::Notify(const QString& event, PlainTextEditorPage* page) {
  LuaHost::Instance().Deliver(event, PageContext(page),
                              TextEditTabWidget::DocumentIdOf(page));
}

auto LuaPlacements::PageContext(PlainTextEditorPage* page) -> UiContext {
  UiContext ctx;
  if (page == nullptr) return ctx;

  QPointer<PlainTextEditorPage> guarded(page);
  ctx.document = UiDocument{
      TextEditTabWidget::DocumentIdOf(page), page->property("type").toString(),
      page->GetTextPage()->document()->isModified(), [guarded]() {
        return !guarded.isNull() && CachedHasOpenPgp(guarded.data());
      }};
  ctx.has_selection = page->GetTextPage()->textCursor().hasSelection();
  return ctx;
}

}  // namespace GpgFrontend::UI::Lua
