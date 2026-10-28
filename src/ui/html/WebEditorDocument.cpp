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

#include "ui/html/WebEditorDocument.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>

#include "ui/html/HtmlViewDocument.h"

namespace GpgFrontend::UI::WebEditor {

auto ContentSecurityPolicy() -> QString {
  return u"default-src 'none'; script-src gfasset:; "
         "style-src gfasset: 'unsafe-inline'; img-src gfres: gfasset: blob:; "
         "font-src gfasset:; connect-src 'none'; media-src 'none'; "
         "object-src 'none'; frame-src 'none'; worker-src 'none'; "
         "form-action 'none'; base-uri 'none'"_s;
}

auto InjectPolicy(const QByteArray& html) -> QByteArray {
  const auto meta =
      (u"<meta http-equiv=\"Content-Security-Policy\" content=\""_s +
       ContentSecurityPolicy() + u"\">"_s)
          .toUtf8();
  static const QRegularExpression kHead(
      u"<head(\\s[^>]*)?>"_s, QRegularExpression::CaseInsensitiveOption);
  const auto text = QString::fromUtf8(html);
  const auto m = kHead.match(text);
  if (!m.hasMatch()) return "<head>" + meta + "</head>" + html;
  auto out = text.left(m.capturedEnd()).toUtf8();
  out += meta;
  out += text.mid(m.capturedEnd()).toUtf8();
  return out;
}

auto AssetMime(const QString& path) -> QString {
  const auto dot = path.lastIndexOf(u'.');
  const auto ext = dot < 0 ? QString() : path.mid(dot + 1).toLower();
  if (ext == u"html"_s) return u"text/html"_s;
  if (ext == u"js"_s || ext == u"mjs"_s) return u"text/javascript"_s;
  if (ext == u"css"_s) return u"text/css"_s;
  if (ext == u"woff2"_s) return u"font/woff2"_s;
  if (ext == u"png"_s) return u"image/png"_s;
  return {};
}

auto IsAllowedResourceMime(const QString& mime) -> bool {
  static const QStringList kAllowed = {u"image/png"_s, u"image/jpeg"_s,
                                       u"image/gif"_s, u"image/webp"_s};
  return kAllowed.contains(mime.toLower());
}

auto IsCleanPath(const QString& path) -> bool {
  if (path.isEmpty() || path.size() > 1024) return false;
  if (path.startsWith(u'/') || path.contains(u'\\')) return false;
  for (const auto& segment : path.split(u'/')) {
    if (segment.isEmpty() || segment == u"."_s || segment == u".."_s) {
      return false;
    }
  }
  return true;
}

auto ParseRequest(const QString& url) -> std::optional<Request> {
  const QUrl parsed(url, QUrl::StrictMode);
  if (!parsed.isValid()) return std::nullopt;
  Request r;
  const auto scheme = parsed.scheme().toLower();
  if (scheme == QLatin1StringView(kAssetScheme)) {
    r.asset = true;
  } else if (scheme != QLatin1StringView(kResourceScheme)) {
    return std::nullopt;
  }
  r.session = parsed.host().toLower();
  // Decoded: "%2e%2e" is ".." for every purpose that matters here.
  r.path = parsed.path(QUrl::FullyDecoded);
  if (r.path.startsWith(u'/')) r.path.remove(0, 1);
  if (!HtmlView::IsSessionName(r.session) || !IsCleanPath(r.path)) {
    return std::nullopt;
  }
  // A query or fragment never selects anything; ignored, not refused.
  return r;
}

auto IsValidMessage(const QByteArray& json) -> bool {
  if (json.isEmpty() || json.size() > kMaxMessageBytes) return false;
  QJsonParseError error{};
  const auto doc = QJsonDocument::fromJson(json, &error);
  if (error.error != QJsonParseError::NoError || !doc.isObject()) return false;
  const auto type = doc.object().value(u"type"_s);
  return type.isString() && !type.toString().isEmpty() &&
         type.toString().size() <= 64;
}

}  // namespace GpgFrontend::UI::WebEditor
