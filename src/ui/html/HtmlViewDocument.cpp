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

#include "HtmlViewDocument.h"

#include <QRandomGenerator>
#include <QRegularExpression>
#include <QUrl>

namespace GpgFrontend::UI::HtmlView {

namespace {

constexpr qint64 kMiB = qint64{1024} * 1024;

/// Every bit there is: the mail preset gives a sender nothing at all.
constexpr uint32_t kAllBits = kNO_NETWORK | kPARTS_ONLY | kNO_SCRIPT |
                              kNO_NAVIGATION | kNO_POPUPS | kNO_DOWNLOADS |
                              kNO_PERSISTENCE | kNO_PERMISSIONS |
                              kNO_DNS_PREFETCH | kNO_PLUGINS;

/**
 * @brief Replace every match of @p re in @p text with what @p fn makes of it.
 *
 * QString::replace() with a regex takes only a fixed replacement, and every
 * rewrite here depends on what was matched.
 */
template <typename Fn>
auto ReplaceEach(const QString& text, const QRegularExpression& re, Fn fn)
    -> QString {
  QString out;
  out.reserve(text.size());
  qsizetype last = 0;
  auto it = re.globalMatch(text);
  while (it.hasNext()) {
    const auto m = it.next();
    out += QStringView(text).mid(last, m.capturedStart() - last);
    out += fn(m);
    last = m.capturedEnd();
  }
  out += QStringView(text).mid(last);
  return out;
}

}  // namespace

auto PolicyFor(int preset) -> std::optional<Policy> {
  if (preset != kPresetMail) return std::nullopt;
  Policy p;
  p.preset = kPresetMail;
  p.required = kAllBits;
  p.max_document_bytes = 8 * kMiB;
  p.max_part_bytes = 16 * kMiB;
  p.max_total_part_bytes = 64 * kMiB;
  return p;
}

auto NewSession() -> QString {
  std::array<quint32, 4> words{};
  QRandomGenerator::system()->fillRange(words.data(),
                                        static_cast<qsizetype>(words.size()));
  QString s;
  for (const auto w : words) s += QString("%1").arg(w, 8, 16, QChar(u'0'));
  return s;
}

auto IsSessionName(const QString& session) -> bool {
  static const QRegularExpression kShape(u"^[0-9a-f]{32}$"_s);
  return kShape.match(session).hasMatch();
}

auto NormalizeCid(const QString& cid) -> QString {
  auto s = cid.trimmed();
  if (s.startsWith(u"cid:"_s, Qt::CaseInsensitive)) s = s.mid(4);
  s = QUrl::fromPercentEncoding(s.toUtf8()).trimmed();
  if (s.startsWith(u'<') && s.endsWith(u'>')) s = s.mid(1, s.size() - 2);
  return s.trimmed();
}

auto PartUrl(const QString& session, const QString& cid) -> QString {
  return QString("%1://%2/%3")
      .arg(QLatin1StringView(kPartScheme), session,
           QString::fromLatin1(QUrl::toPercentEncoding(NormalizeCid(cid))));
}

auto RewriteCidReferences(const QString& body, const QString& session)
    -> QString {
  const auto part = [&](const QString& reference) {
    return PartUrl(session, reference);
  };

  // src=, background=, poster=: one URL, quoted or not.
  static const QRegularExpression kAttribute(
      uR"((\b(?:src|background|poster)\s*=\s*)(["']?)\s*cid:([^"'\s>]+))"_s,
      QRegularExpression::CaseInsensitiveOption);
  auto out = ReplaceEach(body, kAttribute, [&](const auto& m) {
    return m.captured(1) + m.captured(2) + part(m.captured(3));
  });

  // srcset=: a list of candidates, each "URL [descriptor]".
  static const QRegularExpression kSrcset(
      uR"((\bsrcset\s*=\s*)(["'])(.*?)\2)"_s,
      QRegularExpression::CaseInsensitiveOption |
          QRegularExpression::DotMatchesEverythingOption);
  static const QRegularExpression kCandidate(
      uR"((^|,)(\s*)cid:([^\s,]+))"_s,
      QRegularExpression::CaseInsensitiveOption);
  out = ReplaceEach(out, kSrcset, [&](const auto& m) {
    const auto list =
        ReplaceEach(m.captured(3), kCandidate, [&](const auto& c) {
          return c.captured(1) + c.captured(2) + part(c.captured(3));
        });
    return m.captured(1) + m.captured(2) + list + m.captured(2);
  });

  // CSS url(cid:...), in a style attribute or a <style> element alike.
  static const QRegularExpression kCssUrl(
      uR"((url\(\s*)(["']?)cid:([^"')\s]+))"_s,
      QRegularExpression::CaseInsensitiveOption);
  return ReplaceEach(out, kCssUrl, [&](const auto& m) {
    return m.captured(1) + m.captured(2) + part(m.captured(3));
  });
}

auto EscapeAttribute(const QString& text) -> QString {
  QString out;
  out.reserve(text.size() + (text.size() / 8));
  for (const auto c : text) {
    switch (c.unicode()) {
      case u'&':
        out += u"&amp;"_s;
        break;
      case u'"':
        out += u"&quot;"_s;
        break;
      case u'<':
        out += u"&lt;"_s;
        break;
      case u'>':
        out += u"&gt;"_s;
        break;
      case 0:
        out += QChar(QChar::ReplacementCharacter);
        break;
      default:
        out += c;
    }
  }
  return out;
}

auto SandboxTokens() -> QString {
  // Only what turns a user's click into something the renderer can see.
  // Top navigation needs that click (a refresh, a form or a timer gets none),
  // and lands on the hook, which refuses it and reports the link. A popup
  // reaches createWindow, which shows nothing. No scripts, forms, same
  // origin, downloads, modals or pointer lock, and no escaping the sandbox.
  return u"allow-popups allow-top-navigation-by-user-activation"_s;
}

auto ContentSecurityPolicy() -> QString {
  // A srcdoc frame makes no request, so frame-src does not apply to the
  // sandbox the body runs in; it does apply to every frame the body itself
  // asks for, which the body inherits along with the rest of this policy.
  // navigate-to is not here: CSP3 dropped it and no engine implements it, so
  // navigation is the engine hooks' to refuse.
  return QString(
             "default-src 'none'; img-src %1:; style-src 'unsafe-inline'; "
             "font-src 'none'; frame-src 'none'; form-action 'none'; "
             "base-uri 'none'")
      .arg(QLatin1StringView(kPartScheme));
}

auto BuildDocument(const QString& body, const QString& session) -> QByteArray {
  // Every link in the body targets the top document unless it names a
  // target of its own: the frame itself may navigate nowhere (frame-src
  // 'none' covers it too), and the top document is where the renderer's
  // hook sees a click and hands it to the Host. First, so it is the base
  // element that counts; the body's own <base> cannot set an href under
  // base-uri 'none', and comes second for the target.
  const auto srcdoc = EscapeAttribute(u"<base target=\"_top\">"_s +
                                      RewriteCidReferences(body, session));
  const auto document =
      QString(
          "<!DOCTYPE html><html><head><meta charset=\"utf-8\">"
          "<meta http-equiv=\"Content-Security-Policy\" content=\"%1\">"
          "<meta name=\"referrer\" content=\"no-referrer\">"
          "<meta http-equiv=\"x-dns-prefetch-control\" content=\"off\">"
          "<style>html,body,iframe{margin:0;border:0;padding:0;width:100%;"
          "height:100%;display:block}</style></head><body>"
          "<iframe sandbox=\"%3\" referrerpolicy=\"no-referrer\" "
          "srcdoc=\"%2\"></iframe></body></html>")
          .arg(ContentSecurityPolicy(), srcdoc, SandboxTokens());
  return document.toUtf8();
}

auto IsAllowedPartMime(const QString& mime) -> bool {
  static const QStringList kAllowed = {
      u"image/png"_s,  u"image/jpeg"_s, u"image/gif"_s,
      u"image/webp"_s, u"image/bmp"_s,  u"image/avif"_s,
  };
  // Parameters ("; name=...") say nothing about what the bytes are.
  const auto type = mime.section(u';', 0, 0).trimmed().toLower();
  if (type == u"image/jpg"_s) return true;  // common, if not registered
  return kAllowed.contains(type);
}

}  // namespace GpgFrontend::UI::HtmlView
