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
// Closing tabs: one path for a tab's button, middle click, the tab menu, the
// File menu, the open-tabs list and quitting.

#include "core/function/GlobalSettingStation.h"
#include "core/function/SettingCatalog.h"
#include "core/module/ModuleManager.h"
#include "ui/function/FilePanelPath.h"
#include "ui/widgets/EditorEmptyView.h"
#include "ui/widgets/OpenTabsPopup.h"
#include "ui/widgets/TextEdit.h"

namespace GpgFrontend::UI {

namespace {

enum class CloseKind {
  kOTHER,           ///< nothing to save (a file panel)
  kPLAIN,           ///< the Host holds the document and saves it
  kMODULE_DECIDES,  ///< a module persists it and takes the Host's decision
  kMODULE_ASKS,     ///< a module persists it and asks the user itself
};

struct CloseTarget {
  quint64 id = 0;
  QPointer<QWidget> page;
  CloseKind kind = CloseKind::kOTHER;
  bool modified = false;
  bool asked = false;  ///< in the Host's question
  bool save = false;   ///< the answer to it
  bool ready = false;  ///< may be removed
  bool kept = false;   ///< stays open
};

auto TextPageOf(const CloseTarget& t) -> PlainTextEditorPage* {
  return qobject_cast<PlainTextEditorPage*>(t.page.data());
}

/// Put the keyboard where the user can type into @p page.
void FocusPage(QWidget* page) {
  if (page == nullptr) return;
  if (auto* text_page = qobject_cast<PlainTextEditorPage*>(page)) {
    if (auto* view = text_page->PrimaryView()) {
      view->setFocus();
    } else {
      text_page->GetTextPage()->setFocus();
    }
    return;
  }
  page->setFocus();
}

}  // namespace

void TextEdit::SetTabClosePrompter(std::unique_ptr<TabClosePrompter> prompter) {
  prompter_ = prompter != nullptr ? std::move(prompter)
                                  : std::make_unique<TabClosePrompter>();
}

void TextEdit::slot_remove_tab(int index) {
  const auto id = TextEditTabWidget::TabIdOf(tab_widget_->widget(index));
  if (id != 0) CloseTabs({id});
}

void TextEdit::SlotCloseTab() { slot_remove_tab(tab_widget_->currentIndex()); }

void TextEdit::SlotCloseAllTabs() { CloseTabs(tab_widget_->TabIds()); }

auto TextEdit::CloseTabs(const QList<quint64>& ids) -> TabCloseOutcome {
  return run_close(ids, true, false);
}

auto TextEdit::MaybeSaveAnyTab() -> bool {
  // Quitting: the same questions, but the tabs stay until the window goes.
  // With restore enabled, unsaved plain text goes to the recovery cache
  // instead of being asked about; a module's document is always asked.
  const bool cache_plain =
      SettingItems::kRestoreTextEditorPage.Read(GetSettings());
  const auto outcome = run_close(tab_widget_->TabIds(), false, cache_plain);
  if (outcome.cancelled || !outcome.kept.isEmpty()) return false;

  if (cache_plain) {
    FLOG_D("restore_text_editor_page is true, caching messages and exiting");
    tab_widget_->SlotCacheTextEditors();
  }
  return true;
}

auto TextEdit::run_close(const QList<quint64>& ids, bool teardown,
                         bool cache_plain) -> TabCloseOutcome {
  TabCloseOutcome outcome;

  // 1. Collect, before anything can change: the targets, and where the user
  //    was.
  const auto active = TextEditTabWidget::TabIdOf(tab_widget_->currentWidget());
  const QPointer<QWidget> focus = QApplication::focusWidget();

  QList<CloseTarget> targets;
  for (const auto id : ids) {
    auto* page = tab_widget_->PageForTab(id);
    if (page == nullptr) continue;
    const bool listed =
        std::any_of(targets.cbegin(), targets.cend(),
                    [id](const CloseTarget& t) { return t.id == id; });
    if (listed) continue;
    CloseTarget t;
    t.id = id;
    t.page = page;
    targets.append(t);
  }
  if (targets.isEmpty()) return outcome;

  // A dialog, a module's callback or a save may run the event loop; a target
  // is only ever touched again after checking it is still one of the tabs.
  const auto alive = [this](const CloseTarget& t) {
    return !t.page.isNull() && tab_widget_->indexOf(t.page) >= 0;
  };
  const auto keep = [&outcome](CloseTarget& t) {
    t.kept = true;
    outcome.kept.append(t.id);
  };

  // 2. Assess. Nothing here saves, closes, wipes or calls into a module.
  QList<UnsavedTab> unsaved;
  for (auto& t : targets) {
    auto* page = TextPageOf(t);
    if (page == nullptr) continue;

    t.modified = TextEditTabWidget::IsPageModified(page);
    if (!page->IsModulePersisted()) {
      t.kind = CloseKind::kPLAIN;
    } else {
      t.kind = page->ModuleTakesCloseDecision() ? CloseKind::kMODULE_DECIDES
                                                : CloseKind::kMODULE_ASKS;
    }

    t.asked = t.modified && (t.kind == CloseKind::kMODULE_DECIDES ||
                             (t.kind == CloseKind::kPLAIN && !cache_plain));
    if (t.asked) {
      unsaved.append(
          {t.id, TextEditTabWidget::TabTitleOf(page), page->GetFilePath()});
    }
  }

  // 3. Decide: one question for everything unsaved. Cancel leaves every tab
  //    exactly as it was.
  if (unsaved.size() == 1) {
    const auto choice = prompter_->AskSaveOne(this, unsaved.front());
    if (choice == SaveChoice::kCANCEL) {
      outcome.cancelled = true;
      return outcome;
    }
    for (auto& t : targets) {
      if (t.asked) t.save = choice == SaveChoice::kSAVE;
    }
  } else if (unsaved.size() > 1) {
    const auto to_save = prompter_->AskSaveMany(this, unsaved);
    if (!to_save.has_value()) {
      outcome.cancelled = true;
      return outcome;
    }
    for (auto& t : targets) {
      if (t.asked) t.save = to_save->contains(t.id);
    }
  }

  // 4. Tabs gone while the question was open are no longer this close's.
  targets.removeIf([&](const CloseTarget& t) { return !alive(t); });

  // 5. Views that ask the user themselves. One that says no keeps every tab,
  //    and nothing has been written yet.
  for (auto& t : targets) {
    if (t.kind != CloseKind::kMODULE_ASKS || !alive(t)) continue;
    if (!TextPageOf(t)->ModuleCloseRequested()) {
      outcome.cancelled = true;
      return outcome;
    }
    t.ready = true;
  }

  // 6. Saves. One that fails, is cancelled or leaves changes unsaved (a note
  //    whose encrypted snapshot is older than its text) keeps its tab, with
  //    its content; the others go on.
  for (auto& t : targets) {
    if (!t.save || t.ready || !alive(t)) continue;
    auto* page = TextPageOf(t);

    bool saved = false;
    if (t.kind == CloseKind::kPLAIN) {
      const auto path = page->GetFilePath();
      saved = path.isEmpty() ? save_page_as(page) : save_page(page, path);
      saved = saved && alive(t) &&
              !TextEditTabWidget::IsPageModified(TextPageOf(t));
    } else {
      const auto decided = page->ModuleCloseDecided(true);
      saved = decided.has_value() ? *decided : page->ModuleCloseRequested();
    }

    if (saved) {
      t.ready = true;
    } else {
      keep(t);
    }
  }

  // 7. Let go of the rest: a discarded module document is told so, a clean
  //    one is asked as it always was (it does not prompt when clean).
  for (auto& t : targets) {
    if (t.ready || t.kept || !alive(t)) continue;
    auto* page = TextPageOf(t);

    bool may = true;
    if (t.kind == CloseKind::kMODULE_DECIDES) {
      if (t.modified) {
        const auto decided = page->ModuleCloseDecided(false);
        may = decided.has_value() ? *decided : page->ModuleCloseRequested();
      } else {
        may = page->ModuleCloseRequested();
      }
    }

    if (may) {
      t.ready = true;
    } else {
      keep(t);
    }
  }

  QList<CloseTarget> leaving;
  for (const auto& t : targets) {
    if (t.ready && !t.kept && alive(t)) leaving.append(t);
  }

  if (!teardown) {
    for (const auto& t : leaving) outcome.closed.append(t.id);
    return outcome;
  }

  // 8. Teardown, only now. The tab to show next is chosen first, so closing
  //    the active tab never flashes whichever neighbour Qt would pick.
  QSet<quint64> closing;
  for (const auto& t : leaving) closing.insert(t.id);
  const bool active_closing = closing.contains(active);

  const auto next = TabToShowAfterClose(tab_widget_->MruTabIds(),
                                        tab_widget_->TabIds(), closing, active);
  if (active_closing && next.has_value()) {
    tab_widget_->setCurrentWidget(tab_widget_->PageForTab(*next));
  }

  for (const auto& t : leaving) {
    if (!alive(t)) continue;
    auto* tab = t.page.data();
    const int index = tab_widget_->indexOf(tab);
    const auto closed_type = tab->property("type").toString();
    tab_widget_->removeTab(index);

    // After the removal, because the fact being reported is that the tab is
    // gone. A module told beforehand would still see it in the widget and
    // could act on something about to be destroyed.
    Module::TriggerEvent("TAB_CLOSED",
                         {{"tab_index", GFBuffer{QString::number(index)}},
                          {"tab_type", GFBuffer{closed_type}}});

    // Wiped through the page rather than left to close(): by now the widget
    // is removed, hidden and unparented, and hanging the erasure of a
    // decrypted message on a QCloseEvent still reaching it is not a bet
    // worth taking.
    if (auto* page = qobject_cast<PlainTextEditorPage*>(tab)) {
      page->WipeContent();
    }

    tab->close();
    tab->deleteLater();
    outcome.closed.append(t.id);
  }

  // The user intentionally closed these tabs. Rewrite the recovery cache after
  // removeTab(), so they will not be recovered again.
  if (!outcome.closed.isEmpty()) tab_widget_->SlotRefreshRecoveryCache();

  // The keyboard stays where it was when the active tab survives; otherwise
  // it follows the tab now shown.
  if (!active_closing) {
    if (!focus.isNull() && focus->isVisible()) focus->setFocus();
  } else {
    FocusPage(tab_widget_->currentWidget());
  }

  return outcome;
}

void TextEdit::SlotSwitchTabRecent() { tab_widget_->StepSwitchSession(1); }

void TextEdit::SlotSwitchTabRecentBack() { tab_widget_->StepSwitchSession(-1); }

void TextEdit::SetEmptyViewActions(std::function<QList<QAction*>()> provider) {
  empty_view_->SetStartActions(std::move(provider));
}

void TextEdit::update_empty_view() {
  const bool empty = tab_widget_->count() == 0;
  QWidget* wanted = empty ? static_cast<QWidget*>(empty_view_) : tab_widget_;
  if (stack_->currentWidget() == wanted) return;

  const bool had_focus = isAncestorOf(QApplication::focusWidget());
  stack_->setCurrentWidget(wanted);
  if (empty && had_focus) empty_view_->setFocus();
}

void TextEdit::SlotShowOpenTabs() {
  if (tab_widget_->count() == 0) return;
  if (open_tabs_popup_.isNull()) open_tabs_popup_ = new OpenTabsPopup(this);
  open_tabs_popup_->Open();
}

void TextEdit::SlotOpenFileDialog() {
  const auto path = QFileDialog::getOpenFileName(this, tr("Open File"),
                                                 GetDefaultUserFilePath());
  if (!path.isEmpty()) SlotOpenFile(path);
}

}  // namespace GpgFrontend::UI
