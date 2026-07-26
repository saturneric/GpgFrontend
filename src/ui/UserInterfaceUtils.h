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

#include "core/function/AppSecureKeyManager.h"
#include "core/function/result_analyse/GpgVerifyResultAnalyse.h"
#include "core/model/GFBuffer.h"
#include "core/model/GpgKey.h"
#include "core/thread/Task.h"
#include "core/typedef/GpgTypedef.h"

namespace GpgFrontend {
class GpgResultAnalyse;
class GpgImportInformation;
}  // namespace GpgFrontend

namespace GpgFrontend::UI {

class InfoBoardWidget;
class TextEdit;

/**
 * @brief
 *
 * @param parent
 * @param info_board
 * @param error
 * @param verify_result
 */
void show_verify_details(QWidget* parent, InfoBoardWidget* info_board,
                         GpgError error, const GpgVerifyResult& verify_result);

/**
 * @brief
 *
 * @param parent
 * @param waiting_title
 * @param func
 */
void process_operation(QWidget* parent, const QString& waiting_title,
                       Thread::Task::TaskRunnable func,
                       Thread::Task::TaskCallback callback = nullptr,
                       DataObjectPtr data_object = nullptr);

/**
 * @brief
 *
 * @param rect
 * @param available
 * @return QRect
 */
auto ClampRectToAvailableGeometry(QRect rect, const QRect& available) -> QRect;

/**
 * @brief Ask the user to confirm an unusually short user id name.
 *
 * A short name is perfectly legal in an OpenPGP user id, but it is often a
 * typo, so warn instead of refusing. Names of five characters or more are
 * accepted silently.
 *
 * @param parent
 * @param name the already trimmed name
 * @return true if generation should continue
 */
auto ConfirmShortUserIdName(QWidget* parent, const QString& name) -> bool;

/**
 * @brief Drive the full "move a (sub)key onto a smart card" user flow.
 *
 * Shared by the key-details subkey tab and the Smart Card controller so both
 * behave identically: it warns about the destructive nature of `keytocard`,
 * offers to export a secret-key backup first, resolves the target card slot
 * (auto when the key has a single capability, otherwise asks), resolves the
 * target card serial (uses @p preselected_serial when non-empty, otherwise asks
 * from the inserted cards), performs the move via
 * GpgSmartCardManager::MoveKeyToCard, and refreshes the key database on
 * success.
 *
 * @param parent dialog parent for the prompts
 * @param channel key database / engine channel the key lives in
 * @param key the key whose (sub)key is being moved
 * @param subkey_index index into key->SubKeys() (0 is the primary)
 * @param preselected_serial target card serial, or empty to ask the user
 * @return true if the key was moved successfully
 */
auto MoveKeyToCardInteractive(QWidget* parent, int channel,
                              const GpgKeyPtr& key, int subkey_index,
                              const QString& preselected_serial) -> bool;

/**
 * @brief Human name of a memory-hardening secure level.
 *
 * Shared by the Advanced tab and the About dialog so the two cannot drift
 * apart, which is how their labels came to disagree in the first place.
 *
 * @param level the GFSecureLevel value, 0..3
 * @return a translated, user-facing name
 */
auto GF_UI_EXPORT SecureLevelDisplayName(int level) -> QString;

/**
 * @brief Human name of an application key protection mode.
 *
 * @param protection resolved protection, as stored in GFAppKeyProtection
 * @return a translated, user-facing name
 */
auto GF_UI_EXPORT AppKeyProtectionDisplayName(AppKeyProtection protection)
    -> QString;

/**
 * @brief Lower-cased suffix of a file system entry.
 *
 * @param info entry to inspect
 * @return the suffix without the dot, lower cased
 */
auto GF_UI_EXPORT LowerSuffix(const QFileInfo& info) -> QString;

/**
 * @brief Palette-derived accent colour for status chips.
 *
 * Derived from the palette instead of hard coded so chips stay legible under
 * both light and dark themes without a stylesheet. A negative state is not
 * painted red, it is simply de-emphasised text.
 *
 * @param palette the palette of the widget the chip belongs to
 * @param positive whether the chip reports a good state
 * @return the colour to paint the chip text with
 */
auto GF_UI_EXPORT AccentColor(const QPalette& palette, bool positive) -> QColor;

/**
 * @brief Render a small coloured status chip into a label.
 *
 * Uses an inline coloured span rather than a stylesheet, so the label keeps
 * the platform font and the colour can follow the palette.
 *
 * @param label the label to fill
 * @param text plain text, escaped by this function
 * @param color the text colour, usually from AccentColor()
 */
void GF_UI_EXPORT SetChip(QLabel* label, const QString& text,
                          const QColor& color);

/**
 * @brief Whether the entry looks like an OpenPGP message container.
 *
 * These are the files that can be decrypted or verified inline.
 *
 * @param info entry to inspect
 * @return true for .gpg, .pgp and .asc files
 */
auto GF_UI_EXPORT IsOpenPGPMessageFile(const QFileInfo& info) -> bool;

/**
 * @brief Whether the entry is any kind of OpenPGP output.
 *
 * Adds detached signatures to the message containers. Used to keep already
 * processed files out of the encrypt/sign side of the operation menu.
 *
 * @param info entry to inspect
 * @return true for .gpg, .pgp, .asc and .sig files
 */
auto GF_UI_EXPORT IsOpenPGPRelatedFile(const QFileInfo& info) -> bool;

/**
 * @brief Whether the entry is a detached OpenPGP signature.
 *
 * @param info entry to inspect
 * @return true for .sig files
 */
auto GF_UI_EXPORT IsOpenPGPSignatureFile(const QFileInfo& info) -> bool;

/**
 * @brief The font a text surface should use for a stored appearance setting.
 *
 * Starts from the system's fixed-pitch font and only takes @p family over it
 * when that family is actually installed: a font that was uninstalled since it
 * was chosen must fall back to something readable rather than let Qt
 * substitute an arbitrary family.
 *
 * @param family stored family name, empty to keep the system fixed-pitch font
 * @param point_size point size to apply
 * @return the resolved font
 */
auto GF_UI_EXPORT ResolveAppearanceFont(const QString& family, int point_size)
    -> QFont;

/**
 * @brief
 *
 */
class CommonUtils : public QWidget {
  Q_OBJECT
 public:
  /**
   * @brief Construct a new Common Utils object
   *
   */
  CommonUtils();

  /**
   * @brief Get the Instance object
   *
   * @return CommonUtils*
   */
  static auto GF_UI_EXPORT GetInstance() -> CommonUtils*;

  /**
   * @brief What to show the user for a failed OpenPGP environment.
   */
  struct BadOpenPGPEnvText {
    QString title;
    QString body;
    bool offer_retry = true;  ///< false when retrying cannot change anything
  };

  /**
   * @brief Turn a startup failure into a title and message that describe it.
   *
   * Every cause used to be titled "No Supported OpenPGP Engine Found", which
   * was true for one of them and misleading for the rest -- a key database that
   * cannot be found is not a missing engine, and telling the user otherwise
   * sends them looking in the wrong place.
   *
   * @param reason what failed
   * @param detail specifics to append to the body
   */
  static auto DescribeBadOpenPGPEnv(GpgFrontend::BadOpenPGPEnvReason reason,
                                    const QString& detail) -> BadOpenPGPEnvText;

  /**
   * @brief
   *
   * @param err
   */
  static void RaiseMessageBox(QWidget* parent, GpgError err);

  /**
   * @brief
   *
   * @param err
   */
  static void RaiseMessageBoxNotSupported(QWidget* parent);

  /**
   * @brief
   *
   * @param err
   */
  static void RaiseFailureMessageBox(QWidget* parent, GpgError err,
                                     const QString& msg = {});

  /**
   * @brief
   *
   */
  auto IsApplicationNeedRestart() -> bool;

  /**
   * @brief Notify listeners that key categories changed.
   *
   * Call after mutating a category through KeyCategoryRepository so the key
   * tables refresh their filtered views and category tabs.
   */
  void NotifyCategoriesChanged();

  /**
   * @brief
   *
   * @param parent
   * @param key
   */
  static void OpenDetailsDialogByKey(QWidget* parent, int channel,
                                     const GpgAbstractKeyPtr& key);

  /**
   * @brief
   *
   * @param parent
   * @param in_buffer
   */
  void GF_UI_EXPORT ImportKeys(QWidget* parent, int channel,
                               const GFBuffer& in_buffer);

 signals:
  /**
   * @brief
   *
   */
  void SignalKeyStatusUpdated();

  /**
   * @brief
   *
   */
  void SignalBadOpenPGPEnv(GpgFrontend::BadOpenPGPEnvReason reason,
                           QString detail);

  /**
   * @brief emit when the key database is refreshed
   *
   */
  void SignalKeyDatabaseRefreshDone();

  /**
   * @brief
   *
   */
  void SignalRestartApplication(int);

  /**
   * @brief Emitted when any key category changes.
   *
   */
  void SignalCategoriesChanged();

 public slots:

  /**
   * @brief
   *
   * @param parent
   * @param in_buffer
   */
  void SlotImportKeys(QWidget* parent, int channel, const GFBuffer& in_buffer,
                      bool rev_cert = false);

  /**
   * @brief
   *
   * @param parent
   */
  void SlotImportKeyFromFile(QWidget* parent, int channel);

  /**
   * @brief
   *
   * @param parent
   */
  void SlotImportKeyFromClipboard(QWidget* parent, int channel);

  /**
   * @brief
   *
   * @param arguments
   * @param interact_func
   */
  void SlotExecuteGpgCommand(
      const QStringList& arguments,
      const std::function<void(QProcess*)>& interact_func);

  /**
   * @brief
   *
   * @param arguments
   * @param interact_func
   */
  void SlotExecuteCommand(const QString& cmd, const QStringList& arguments,
                          const std::function<void(QProcess*)>& interact_func);

  /**
   * @brief
   *
   */
  void SlotRestartApplication(int);

 private slots:

  /**
   * @brief update the key status when signal is emitted
   *
   */
  void slot_update_key_status();

  /**
   * @brief
   *
   */
  void slot_update_key_from_server_finished(
      int channel, bool, QString, QByteArray,
      QSharedPointer<GpgImportInformation>);

 private:
  static QScopedPointer<CommonUtils> instance;  ///<

  bool application_need_to_restart_at_once_ = false;
};

}  // namespace GpgFrontend::UI
