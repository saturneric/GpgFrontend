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

#pragma once

#include "GFSDKCommand.hpp"
#include "GFSDKHostApi.h"

/**
 * @file GFSDKHostCommands.hpp
 * @brief The commands the Host itself provides, as types.
 *
 * Ids, arguments and results only. Their titles are the Host's and are
 * written -- and translated -- beside the implementations, because the
 * Host's translation scan does not reach the SDK. The titles below are
 * placeholders for the descriptor and never shown.
 *
 * Semantic, not mechanical: "open a document", "encrypt this document" --
 * never "set this widget's text". Near-identical operations are one family
 * with a typed target, not one id per document type: `crypto.encrypt`
 * encrypts a text document and an e-mail alike, and the document's own type
 * decides how.
 */

namespace gf::cmd::host {

namespace detail {

/// No arguments beyond which document.
struct TargetArgs {
  DocumentRef target;  ///< id 0: the active document
  static constexpr auto Fields() {
    return std::make_tuple(F("target", &TargetArgs::target));
  }
};

struct DocumentResult {
  qint64 document_id = 0;
  static constexpr auto Fields() {
    return std::make_tuple(F("document_id", &DocumentResult::document_id));
  }
};

}  // namespace detail

// ------------------------------------------------------------------ documents

/// A new, empty document of @p type, in a tab of its own.
struct DocumentNew {
  static constexpr Meta kMeta{"org.gpgfrontend.document.new", "New Document",
                              "", "", 0, kNeedsGuiThread};
  struct Args {
    QString type;   ///< "text", or a document type a module provides
    QString title;  ///< may be empty: the Host picks one
    static constexpr auto Fields() {
      return std::make_tuple(F("type", &Args::type), F("title", &Args::title));
    }
  };
  using Result = detail::DocumentResult;
};

/// A document with content, in a tab of its own.
struct DocumentOpen {
  static constexpr Meta kMeta{"org.gpgfrontend.document.open",
                              "Open Document", "", "", 0, kNeedsGuiThread};
  struct Args {
    QString type;
    QString title;
    QString path;          ///< where it came from, or empty
    Blob content;          ///< the document's bytes, never copied into CBOR
    bool saved = false;    ///< true: identical to @p path on disk
    static constexpr auto Fields() {
      return std::make_tuple(F("type", &Args::type), F("title", &Args::title),
                             F("path", &Args::path),
                             F("content", &Args::content),
                             F("saved", &Args::saved));
    }
  };
  using Result = detail::DocumentResult;
};

struct DocumentSave {
  static constexpr Meta kMeta{"org.gpgfrontend.document.save", "Save", "", "",
                              0, kNeedsGuiThread};
  using Args = detail::TargetArgs;
  using Result = Unit;
};

struct DocumentSaveAs {
  static constexpr Meta kMeta{"org.gpgfrontend.document.save_as", "Save As",
                              "", "", 0, kNeedsGuiThread};
  using Args = detail::TargetArgs;
  using Result = Unit;
};

struct DocumentClose {
  static constexpr Meta kMeta{"org.gpgfrontend.document.close", "Close", "",
                              "", 0, kNeedsGuiThread};
  using Args = detail::TargetArgs;
  using Result = Unit;
};

// ------------------------------------------------------------------ crypto
//
// One family over every document type. Each starts the Host's own operation
// -- key selection, passphrase, progress -- on the target document and
// returns once it has started; the result lands in the document.

#define GF_HOST_CRYPTO_COMMAND(Name, id, title)                         \
  struct Name {                                                          \
    static constexpr Meta kMeta{id, title, "", "", GF_HOST_CAP_GPG,      \
                                kNeedsGuiThread};                        \
    using Args = detail::TargetArgs;                                     \
    using Result = Unit;                                                 \
  };

GF_HOST_CRYPTO_COMMAND(CryptoEncrypt, "org.gpgfrontend.crypto.encrypt",
                       "Encrypt")
GF_HOST_CRYPTO_COMMAND(CryptoDecrypt, "org.gpgfrontend.crypto.decrypt",
                       "Decrypt")
GF_HOST_CRYPTO_COMMAND(CryptoSign, "org.gpgfrontend.crypto.sign", "Sign")
GF_HOST_CRYPTO_COMMAND(CryptoVerify, "org.gpgfrontend.crypto.verify", "Verify")
GF_HOST_CRYPTO_COMMAND(CryptoEncryptSign,
                       "org.gpgfrontend.crypto.encrypt_sign",
                       "Encrypt and Sign")
GF_HOST_CRYPTO_COMMAND(CryptoDecryptVerify,
                       "org.gpgfrontend.crypto.decrypt_verify",
                       "Decrypt and Verify")

#undef GF_HOST_CRYPTO_COMMAND

// ------------------------------------------------------------------ keys

/// Import keys from bytes, with the Host's own import dialog and report.
struct KeysImport {
  static constexpr Meta kMeta{"org.gpgfrontend.keys.import", "Import Keys", "",
                              "", GF_HOST_CAP_GPG, kNeedsGuiThread};
  struct Args {
    Blob data;  ///< may hold secret keys, so never inside the CBOR
    static constexpr auto Fields() {
      return std::make_tuple(F("data", &Args::data));
    }
  };
  using Result = Unit;
};

struct KeysOpenManager {
  static constexpr Meta kMeta{"org.gpgfrontend.keys.open_manager",
                              "Key Management", "", "", 0, kNeedsGuiThread};
  using Args = Unit;
  using Result = Unit;
};

// ------------------------------------------------------------------ views

/// Open one of the caller's own mounted views -- a dialog, say. A module may
/// open only what it mounted.
struct ViewOpen {
  static constexpr Meta kMeta{"org.gpgfrontend.view.open", "Open", "", "", 0,
                              kNeedsGuiThread};
  struct Args {
    ViewRef view;
    static constexpr auto Fields() {
      return std::make_tuple(F("view", &Args::view));
    }
  };
  using Result = Unit;
};

// ------------------------------------------------------------------ app

struct AppOpenSettings {
  static constexpr Meta kMeta{"org.gpgfrontend.app.open_settings", "Settings",
                              "", "", 0, kNeedsGuiThread};
  using Args = Unit;
  using Result = Unit;
};

/// A message to the user, from the Host's window, in the Host's style.
/// Replaces message boxes a module used to parent to a Host window.
struct AppMessage {
  static constexpr Meta kMeta{"org.gpgfrontend.app.message", "Message", "", "",
                              0, kNeedsGuiThread};
  enum class Severity : int { kInfo = 0, kWarning = 1, kError = 2 };
  struct Args {
    Severity severity = Severity::kInfo;
    QString title;
    QString text;
    static constexpr auto Fields() {
      return std::make_tuple(F("severity", &Args::severity),
                             F("title", &Args::title), F("text", &Args::text));
    }
  };
  using Result = Unit;
};

}  // namespace gf::cmd::host
