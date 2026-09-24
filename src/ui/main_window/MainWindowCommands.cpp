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

#include <QMessageBox>

#include "MainWindow.h"
#include "sdk/GFSDKHostCommands.hpp"
#include "ui/command/CommandRegistry.h"
#include "ui/function/FileTypeUtils.h"
#include "ui/function/ImportKey.h"
#include "ui/lua/LuaMounts.h"
#include "ui/widgets/PlainTextEditorPage.h"
#include "ui/widgets/TextEdit.h"
#include "ui/widgets/TextEditTabWidget.h"

/**
 * @file MainWindowCommands.cpp
 * @brief The Host's own commands, and the menu actions that invoke them.
 *
 * The command registry is the one path to "do something", for the Host as
 * much as for a module: the Save menu entry, a module's C++ and a module's UI
 * script all reach `org.gpgfrontend.document.save` the same way, with the
 * same checks. What lives here is the Host's implementation of each, wrapped
 * around the slots that already existed.
 *
 * Titles are marked in MainWindow's own translation context with the same
 * source text as the menu entries, so a command shows exactly the words the
 * menu always did, in every language that already had them.
 */

namespace GpgFrontend::UI {

namespace {

namespace host = gf::cmd::host;
using gf::cmd::CommandContext;
using gf::cmd::Outcome;
using gf::cmd::Unit;

constexpr auto kContext = "GpgFrontend::UI::MainWindow";

auto Ok() -> Outcome<Unit> { return Outcome<Unit>::Success({}); }

auto NoWindow() -> Outcome<Unit> {
  return Outcome<Unit>::Failure(GF_CMD_E_UNAVAILABLE,
                                QStringLiteral("the main window is gone"));
}

auto NoDocument() -> Outcome<Unit> {
  return Outcome<Unit>::Failure(GF_CMD_E_BAD_ARGS,
                                QStringLiteral("no such document"));
}

}  // namespace

/**
 * @brief The implementations. A friend of MainWindow, so each one wraps a
 *        slot rather than reimplementing it.
 */
struct HostCommandHandlers {
  static QPointer<MainWindow> window;

  /// The target document, made current -- the existing slots all act on
  /// the current tab. Null when there is no such document.
  static auto Focus(const gf::cmd::DocumentRef& target)
      -> PlainTextEditorPage* {
    if (window.isNull()) return nullptr;
    auto* tabs = window->edit_->TabWidget();
    auto* page = tabs->PageForDocument(target.id);
    if (page != nullptr) tabs->setCurrentWidget(page);
    return page;
  }

  static auto DocumentNew(const CommandContext&,
                          const host::DocumentNew::Args& a)
      -> Outcome<host::DocumentNew::Result> {
    using R = Outcome<host::DocumentNew::Result>;
    if (window.isNull()) return R::Failure(GF_CMD_E_UNAVAILABLE, {});
    auto* page = window->edit_->TabWidget()->SlotNewTab(
        a.type.isEmpty() ? QStringLiteral("text") : a.type,
        SanitizedDocumentTitle(a.title), QIcon(), {});
    return R::Success({TextEditTabWidget::DocumentIdOf(page)});
  }

  static auto DocumentOpen(const CommandContext& ctx,
                           const host::DocumentOpen::Args& a)
      -> Outcome<host::DocumentOpen::Result> {
    using R = Outcome<host::DocumentOpen::Result>;
    if (window.isNull()) return R::Failure(GF_CMD_E_UNAVAILABLE, {});
    // A module cannot bind a tab to a file. A bound, saved tab is one the next
    // Save writes straight back to its path without asking, so letting a
    // module name that path would let it write any file the user can write.
    // What a module opens is always a new, unsaved document; saving it asks.
    const bool from_host = ctx.caller.isEmpty();
    const auto id = window->edit_->OpenDocument(
        a.type, SanitizedDocumentTitle(a.title), from_host ? a.path : QString(),
        BlobToGFBuffer(a.content), from_host && a.saved, a.modified);
    if (id == 0) return R::Failure(GF_CMD_E_FAILED, {});
    return R::Success({id});
  }

  static auto Save(const CommandContext&, const host::DocumentSave::Args& a)
      -> Outcome<Unit> {
    if (window.isNull()) return NoWindow();
    if (Focus(a.target) == nullptr) return NoDocument();
    window->edit_->SlotSave();
    return Ok();
  }

  static auto SaveAs(const CommandContext&, const host::DocumentSaveAs::Args& a)
      -> Outcome<Unit> {
    if (window.isNull()) return NoWindow();
    if (Focus(a.target) == nullptr) return NoDocument();
    window->edit_->SlotSaveAs();
    return Ok();
  }

  static auto Close(const CommandContext&, const host::DocumentClose::Args& a)
      -> Outcome<Unit> {
    if (window.isNull()) return NoWindow();
    if (Focus(a.target) == nullptr) return NoDocument();
    window->edit_->SlotCloseTab();
    return Ok();
  }

  // --- the crypto family: one implementation, parameterized by operation

  enum class Op {
    kEncrypt,
    kDecrypt,
    kSign,
    kVerify,
    kEncryptSign,
    kDecryptVerify
  };

  static auto ActionFor(Op op) -> QAction* {
    if (window.isNull()) return nullptr;
    switch (op) {
      case Op::kEncrypt:
        return window->encrypt_act_;
      case Op::kDecrypt:
        return window->decrypt_act_;
      case Op::kSign:
        return window->sign_act_;
      case Op::kVerify:
        return window->verify_act_;
      case Op::kEncryptSign:
        return window->encrypt_sign_act_;
      case Op::kDecryptVerify:
        return window->decrypt_verify_act_;
    }
    return nullptr;
  }

  template <Op kOp>
  static auto Crypto(const CommandContext&, const host::detail::TargetArgs& a)
      -> Outcome<Unit> {
    if (window.isNull()) return NoWindow();
    if (Focus(a.target) == nullptr) return NoDocument();
    switch (kOp) {
      case Op::kEncrypt:
        window->SlotGeneralEncrypt(false);
        break;
      case Op::kDecrypt:
        window->SlotGeneralDecrypt(false);
        break;
      case Op::kSign:
        window->SlotGeneralSign(false);
        break;
      case Op::kVerify:
        window->SlotGeneralVerify(false);
        break;
      case Op::kEncryptSign:
        window->SlotGeneralEncryptSign(false);
        break;
      case Op::kDecryptVerify:
        window->SlotGeneralDecryptVerify(false);
        break;
    }
    return Ok();
  }

  /// Whether the operation applies to the current document: the menu has
  /// always worked this out, so the command asks the menu entry.
  template <Op kOp>
  static auto CryptoState(const CommandContext&) -> uint32_t {
    const auto* act = ActionFor(kOp);
    if (act == nullptr) return 0;
    return act->isEnabled() ? GF_CMD_STATE_ENABLED | GF_CMD_STATE_VISIBLE
                            : GF_CMD_STATE_VISIBLE;
  }

  static auto ImportKeys(const CommandContext&, const host::KeysImport::Args& a)
      -> Outcome<Unit> {
    if (window.isNull()) return NoWindow();
    UI::ImportKeys(window, window->GetCurrentGpgContextChannel(),
                   BlobToGFBuffer(a.data));
    return Ok();
  }

  static auto OpenKeyManager(const CommandContext&, const Unit&)
      -> Outcome<Unit> {
    if (window.isNull()) return NoWindow();
    window->slot_open_key_management();
    return Ok();
  }

  static auto OpenSettings(const CommandContext&, const Unit&)
      -> Outcome<Unit> {
    if (window.isNull()) return NoWindow();
    window->slot_open_settings_dialog();
    return Ok();
  }

  /// A module's own dialog mount, and only its own.
  static auto ViewOpen(const CommandContext& ctx, const host::ViewOpen::Args& a)
      -> Outcome<Unit> {
    if (window.isNull()) return NoWindow();
    const auto status = Lua::OpenDialogMount(ctx.caller, a.view.id, {}, window);
    if (status != GF_CMD_OK) {
      return Outcome<Unit>::Failure(status, QStringLiteral("no such view"));
    }
    return Ok();
  }

  static auto Message(const CommandContext&, const host::AppMessage::Args& a)
      -> Outcome<Unit> {
    if (window.isNull()) return NoWindow();
    switch (a.severity) {
      case host::AppMessage::Severity::kWarning:
        QMessageBox::warning(window, a.title, a.text);
        break;
      case host::AppMessage::Severity::kError:
        QMessageBox::critical(window, a.title, a.text);
        break;
      default:
        QMessageBox::information(window, a.title, a.text);
        break;
    }
    return Ok();
  }
};

QPointer<MainWindow> HostCommandHandlers::window;

void MainWindow::register_host_commands() {
  using H = HostCommandHandlers;
  H::window = this;

  auto& r = CommandRegistry::Instance();
  auto reg = [&r](gf::cmd::Binding b, const char* title,
                  const char* description = "") {
    r.RegisterHost(b, HostCommandText{kContext, title, description, ""});
  };
  auto crypto = [&r](gf::cmd::Binding b, auto state, const char* title,
                     const char* description) {
    b.state = state;
    r.RegisterHost(
        b, HostCommandText{
               kContext, title, description,
               QT_TRANSLATE_NOOP("GpgFrontend::UI::MainWindow", "Crypto")});
  };

  reg(gf::cmd::Bind<host::DocumentNew, &H::DocumentNew>(),
      QT_TRANSLATE_NOOP("GpgFrontend::UI::MainWindow", "New Text Editor"));
  reg(gf::cmd::Bind<host::DocumentOpen, &H::DocumentOpen>(),
      QT_TRANSLATE_NOOP("GpgFrontend::UI::MainWindow", "Open"));
  reg(gf::cmd::Bind<host::DocumentSave, &H::Save>(),
      QT_TRANSLATE_NOOP("GpgFrontend::UI::MainWindow", "Save File"),
      QT_TRANSLATE_NOOP("GpgFrontend::UI::MainWindow",
                        "Save the current File"));
  reg(gf::cmd::Bind<host::DocumentSaveAs, &H::SaveAs>(),
      QT_TRANSLATE_NOOP("GpgFrontend::UI::MainWindow", "Save As"),
      QT_TRANSLATE_NOOP("GpgFrontend::UI::MainWindow",
                        "Save the current File as..."));
  reg(gf::cmd::Bind<host::DocumentClose, &H::Close>(),
      QT_TRANSLATE_NOOP("GpgFrontend::UI::MainWindow", "Close Tab"),
      QT_TRANSLATE_NOOP("GpgFrontend::UI::MainWindow",
                        "Close the current tab"));

  crypto(gf::cmd::Bind<host::CryptoEncrypt, &H::Crypto<H::Op::kEncrypt>>(),
         &H::CryptoState<H::Op::kEncrypt>,
         QT_TRANSLATE_NOOP("GpgFrontend::UI::MainWindow", "Encrypt"),
         QT_TRANSLATE_NOOP("GpgFrontend::UI::MainWindow", "Encrypt Message"));
  crypto(gf::cmd::Bind<host::CryptoDecrypt, &H::Crypto<H::Op::kDecrypt>>(),
         &H::CryptoState<H::Op::kDecrypt>,
         QT_TRANSLATE_NOOP("GpgFrontend::UI::MainWindow", "Decrypt"),
         QT_TRANSLATE_NOOP("GpgFrontend::UI::MainWindow", "Decrypt Message"));
  crypto(gf::cmd::Bind<host::CryptoSign, &H::Crypto<H::Op::kSign>>(),
         &H::CryptoState<H::Op::kSign>,
         QT_TRANSLATE_NOOP("GpgFrontend::UI::MainWindow", "Sign"),
         QT_TRANSLATE_NOOP("GpgFrontend::UI::MainWindow", "Sign Message"));
  crypto(gf::cmd::Bind<host::CryptoVerify, &H::Crypto<H::Op::kVerify>>(),
         &H::CryptoState<H::Op::kVerify>,
         QT_TRANSLATE_NOOP("GpgFrontend::UI::MainWindow", "Verify"),
         QT_TRANSLATE_NOOP("GpgFrontend::UI::MainWindow", "Verify Message"));
  crypto(
      gf::cmd::Bind<host::CryptoEncryptSign, &H::Crypto<H::Op::kEncryptSign>>(),
      &H::CryptoState<H::Op::kEncryptSign>,
      QT_TRANSLATE_NOOP("GpgFrontend::UI::MainWindow", "Encrypt && Sign"),
      QT_TRANSLATE_NOOP("GpgFrontend::UI::MainWindow",
                        "Encrypt and Sign Message"));
  crypto(gf::cmd::Bind<host::CryptoDecryptVerify,
                       &H::Crypto<H::Op::kDecryptVerify>>(),
         &H::CryptoState<H::Op::kDecryptVerify>,
         QT_TRANSLATE_NOOP("GpgFrontend::UI::MainWindow", "Decrypt && Verify"),
         QT_TRANSLATE_NOOP("GpgFrontend::UI::MainWindow",
                           "Decrypt and Verify Message"));

  reg(gf::cmd::Bind<host::KeysImport, &H::ImportKeys>(),
      QT_TRANSLATE_NOOP("GpgFrontend::UI::MainWindow", "Import Key"));
  reg(gf::cmd::Bind<host::KeysOpenManager, &H::OpenKeyManager>(),
      QT_TRANSLATE_NOOP("GpgFrontend::UI::MainWindow", "Manage Keys"),
      QT_TRANSLATE_NOOP("GpgFrontend::UI::MainWindow", "Open Key Management"));
  reg(gf::cmd::Bind<host::AppOpenSettings, &H::OpenSettings>(),
      QT_TRANSLATE_NOOP("GpgFrontend::UI::MainWindow", "Settings"),
      QT_TRANSLATE_NOOP("GpgFrontend::UI::MainWindow", "Open settings dialog"));
  reg(gf::cmd::Bind<host::ViewOpen, &H::ViewOpen>(),
      QT_TRANSLATE_NOOP("GpgFrontend::UI::MainWindow", "Open"));
  reg(gf::cmd::Bind<host::AppMessage, &H::Message>(),
      QT_TRANSLATE_NOOP("GpgFrontend::UI::MainWindow", "Message"));
}

void MainWindow::invoke_host_command(const char* id) {
  gf::cmd::EncodeState st;
  const auto args = gf::cmd::EncodeMap(gf::cmd::host::detail::TargetArgs{}, st);
  const auto command = QString::fromLatin1(id);

  // Commands that take no target are given none: the registry refuses an
  // argument a command does not declare, which is what makes a typo in a
  // module's call fail loudly instead of being ignored.
  const auto descriptor = CommandRegistry::Instance().Describe(command);
  const auto declares_target =
      descriptor.has_value() && !descriptor->value(QStringLiteral("args"))
                                     .toMap()
                                     .value(QStringLiteral("fields"))
                                     .toArray()
                                     .isEmpty();

  const auto ticket = CommandRegistry::Instance().Invoke(
      command, declares_target ? args : QCborMap{}, {},
      CommandCaller{{}, 0, QStringLiteral("host")}, {},
      [command](gf::cmd::RawResult r) {
        if (r.status != GF_CMD_OK) {
          LOG_W() << "host command" << command << "failed:" << r.status
                  << r.error;
        }
      });
  if (ticket.status != GF_CMD_OK && ticket.status != GF_CMD_E_DISABLED) {
    LOG_W() << "host command" << command << "refused:" << ticket.status;
  }
}

}  // namespace GpgFrontend::UI
