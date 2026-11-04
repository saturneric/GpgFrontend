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

#include <QtQml/qqmlregistration.h>

#include <QObject>
#include <QVariantMap>
#include <functional>
#include <memory>

namespace GpgFrontend {
class SecureWindowScope;
}

namespace GpgFrontend {
struct GpgOperaOutcome;
}  // namespace GpgFrontend

namespace GpgFrontend::Mobile {

struct OperationParams;

/**
 * @brief Encrypt, decrypt, sign and verify text, for one page.
 *
 * Every operation runs on the default key database's channel through the
 * core's MessageCryptoOperation, asynchronously, in its turn in the
 * application's OperationQueue: the interface keeps running, and a
 * passphrase the operation needs is asked through the PassphraseBroker.
 * busy holds from the call on, while it waits its turn too.
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
  /// The OperationQueue record of the last operation submitted; 0 before
  /// the first.
  Q_PROPERTY(int operationId READ OperationId NOTIFY SignalBusyChanged)

 public:
  explicit TextCryptoController(QObject* parent = nullptr);
  ~TextCryptoController() override;

  [[nodiscard]] auto Busy() const -> bool;
  [[nodiscard]] auto Output() const -> QString;
  [[nodiscard]] auto Result() const -> QVariantMap;
  [[nodiscard]] auto Sensitive() const -> bool;
  [[nodiscard]] auto OperationId() const -> int;

  /**
   * @brief Encrypt @p text to @p recipients (fingerprints), signed by
   * @p signer when it is not empty; or, when @p symmetric, to a passphrase
   * (and neither recipients nor signer are used).
   *
   * Never to a passphrase for want of recipients: with none and not
   * @p symmetric it is refused, and false returned, as when one of this
   * controller's operations is still going.
   */
  Q_INVOKABLE bool encrypt(const QString& text, const QStringList& recipients,
                           const QString& signer = {}, bool symmetric = false);

  /// Decrypt @p text; with @p verify, check any signature inside too
  /// (Decrypt & Verify), else decrypt it alone, as the desktop's Decrypt.
  Q_INVOKABLE bool decrypt(const QString& text, bool verify = true);

  /// Sign @p text in the clear with @p signer (a fingerprint).
  Q_INVOKABLE bool sign(const QString& text, const QString& signer);

  /// Verify a clear-signed @p text.
  Q_INVOKABLE bool verify(const QString& text);

  /// Forget the last output and result, and the text it was made from.
  Q_INVOKABLE void clear();

  /// Whether the text of operation @p id is still held here: its output
  /// (output) and the text it read. Only the last operation's is, until
  /// clear() or the next.
  [[nodiscard]] Q_INVOKABLE bool holds(int id) const;

  /**
   * @brief Run operation @p id again on the text it read, as it was asked
   * then (OperationParams): never with what is chosen now. The keys are
   * looked up again as it runs. False when the text is no longer held
   * (holds()) or one is still going.
   */
  Q_INVOKABLE bool retry(int id);

 signals:
  void SignalBusyChanged();
  void SignalOutputChanged();
  /// An operation ended; @p succeeded unless its result is a failure.
  void SignalFinished(bool succeeded);

 private:
  bool busy_ = false;
  QString output_;
  QString input_;  ///< what the last operation read
  QVariantMap result_;
  std::unique_ptr<SecureWindowScope> secure_;
  /// The operation submitted last, and the one that runs (0: none).
  int submitted_id_ = 0;
  int running_id_ = 0;

  /// Submit @p work, the operation on @p text, to the queue; false when one
  /// of this controller's is still going or the queue refuses it.
  void run_encrypt(const QString& text, const QStringList& recipients,
                   const QString& signer);
  auto submit(OperationParams params, const QString& text,
              std::function<void()> work) -> bool;
  /// The queue ended @p id without running it (it was closed).
  void dropped(int id);
  void finish(const GpgOperaOutcome& outcome, bool sensitive = false);
};

}  // namespace GpgFrontend::Mobile
