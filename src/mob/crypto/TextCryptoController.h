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

#include <QObject>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

#include <memory>

namespace GpgFrontend {
class SecureWindowScope;
}

namespace GpgFrontend {
struct GpgOperaOutcome;
}  // namespace GpgFrontend

namespace GpgFrontend::Mobile {

/**
 * @brief Encrypt, decrypt, sign and verify text, for one page.
 *
 * Every operation runs on the default key database's channel through the
 * core's MessageCryptoOperation, asynchronously: the interface keeps running,
 * and a passphrase the operation needs is asked through the PassphraseBroker.
 * When it is done, output holds the text it produced (empty for a
 * verification) and result what PresentResult() makes of it.
 *
 * Decrypted text is sensitive: while it is held, the window is kept out of
 * screenshots and the recent-apps preview, and clear() drops it.
 */
class TextCryptoController : public QObject {
  Q_OBJECT
  QML_ELEMENT

  Q_PROPERTY(bool busy READ Busy NOTIFY SignalBusyChanged)
  Q_PROPERTY(QString output READ Output NOTIFY SignalOutputChanged)
  Q_PROPERTY(QVariantMap result READ Result NOTIFY SignalOutputChanged)
  Q_PROPERTY(bool sensitive READ Sensitive NOTIFY SignalOutputChanged)

 public:
  explicit TextCryptoController(QObject* parent = nullptr);
  ~TextCryptoController() override;

  [[nodiscard]] auto Busy() const -> bool;
  [[nodiscard]] auto Output() const -> QString;
  [[nodiscard]] auto Result() const -> QVariantMap;
  [[nodiscard]] auto Sensitive() const -> bool;

  /**
   * @brief Encrypt @p text to @p recipients (fingerprints), signed by
   * @p signer when it is not empty; with no recipients, to a passphrase.
   */
  Q_INVOKABLE void encrypt(const QString& text, const QStringList& recipients,
                           const QString& signer = {});

  /// Decrypt @p text, verifying any signature inside.
  Q_INVOKABLE void decrypt(const QString& text);

  /// Sign @p text in the clear with @p signer (a fingerprint).
  Q_INVOKABLE void sign(const QString& text, const QString& signer);

  /// Verify a clear-signed @p text.
  Q_INVOKABLE void verify(const QString& text);

  /// Forget the last output and result.
  Q_INVOKABLE void clear();

 signals:
  void SignalBusyChanged();
  void SignalOutputChanged();
  /// An operation ended; @p succeeded unless its result is a failure.
  void SignalFinished(bool succeeded);

 private:
  bool busy_ = false;
  QString output_;
  QVariantMap result_;
  std::unique_ptr<SecureWindowScope> secure_;

  auto begin() -> bool;
  void finish(const GpgOperaOutcome& outcome, bool sensitive = false);
};

}  // namespace GpgFrontend::Mobile
