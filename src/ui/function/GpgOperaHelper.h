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

#include "GpgFrontendUI.h"
#include "ui/struct/GpgOperaResultContext.h"

namespace GpgFrontend::UI {

class GpgOperaHelper : QObject {
  Q_OBJECT
 public:
  using GpgOperaFactory = std::function<OperaWaitingCb(
      QSharedPointer<GpgOperaContext>& context, int channel, int index)>;

  /**
   * @brief
   *
   * @tparam ResultType
   * @tparam AnalyseType
   * @tparam OperaFunc
   * @param context
   * @param channel
   * @param index
   * @param opera_func
   * @return OperaWaitingCb
   */
  template <typename ResultType, typename AnalyseType, typename OperaFunc>
  static auto BuildSimpleGpgFileOperasHelper(
      QSharedPointer<GpgOperaContext>& context, int channel, int index,
      OperaFunc opera_func) -> OperaWaitingCb;

  /**
   * @brief
   *
   * @tparam ResultTypeA
   * @tparam AnalyseTypeA
   * @tparam ResultTypeB
   * @tparam AnalyseTypeB
   * @tparam OperaFunc
   * @param context
   * @param channel
   * @param index
   * @param opera_func
   * @return OperaWaitingCb
   */
  template <typename ResultTypeA, typename AnalyseTypeA, typename ResultTypeB,
            typename AnalyseTypeB, typename OperaFunc>
  static auto BuildComplexGpgFileOperasHelper(
      QSharedPointer<GpgOperaContext>& context, int channel, int index,
      OperaFunc opera_func) -> OperaWaitingCb;

  /**
   * @brief
   *
   * @tparam ResultType
   * @tparam AnalyseType
   * @tparam OperaFunc
   * @param context
   * @param channel
   * @param index
   * @param opera_func
   * @return OperaWaitingCb
   */
  template <typename ResultType, typename AnalyseType, typename OperaFunc>
  static auto BuildSimpleGpgOperasHelper(
      QSharedPointer<GpgOperaContext>& context, int channel, int index,
      OperaFunc opera_func) -> OperaWaitingCb;

  /**
   * @brief
   *
   * @tparam ResultTypeA
   * @tparam AnalyseTypeA
   * @tparam ResultTypeB
   * @tparam AnalyseTypeB
   * @tparam OperaFunc
   * @param context
   * @param channel
   * @param index
   * @param opera_func
   * @return OperaWaitingCb
   */
  template <typename ResultTypeA, typename AnalyseTypeA, typename ResultTypeB,
            typename AnalyseTypeB, typename OperaFunc>
  static auto BuildComplexGpgOperasHelper(
      QSharedPointer<GpgOperaContext>& context, int channel, int index,
      OperaFunc opera_func) -> OperaWaitingCb;

  /**
   * @brief
   *
   * @param base
   * @param category
   * @param channel
   * @param f
   */
  static void BuildOperas(QSharedPointer<GpgOperaContextBasement>& base,
                          int category, int channel, const GpgOperaFactory& f);

  /**
   * @brief
   *
   * @param context
   * @param channel
   * @param index
   * @return OperaWaitingCb
   */
  static auto BuildOperasEncrypt(QSharedPointer<GpgOperaContext>& context,
                                 int channel, int index) -> OperaWaitingCb;

  /**
   * @brief
   *
   * @param context
   * @param channel
   * @param index
   * @return OperaWaitingCb
   */
  static auto BuildOperasFileEncrypt(QSharedPointer<GpgOperaContext>& context,
                                     int channel, int index) -> OperaWaitingCb;

  /**
   * @brief
   *
   * @param context
   * @param channel
   * @param index
   * @return OperaWaitingCb
   */
  static auto BuildOperasDirectoryEncrypt(
      QSharedPointer<GpgOperaContext>& context, int channel,
      int index) -> OperaWaitingCb;

  /**
   * @brief
   *
   * @param context
   * @param channel
   * @param index
   * @return OperaWaitingCb
   */
  static auto BuildOperasDecrypt(QSharedPointer<GpgOperaContext>& context,
                                 int channel, int index) -> OperaWaitingCb;

  /**
   * @brief
   *
   * @param context
   */
  static auto BuildOperasFileDecrypt(QSharedPointer<GpgOperaContext>& context,
                                     int channel, int index) -> OperaWaitingCb;

  /**
   * @brief
   *
   * @param context
   * @param channel
   * @param index
   * @return OperaWaitingCb
   */
  static auto BuildOperasArchiveDecrypt(
      QSharedPointer<GpgOperaContext>& context, int channel,
      int index) -> OperaWaitingCb;

  /**
   * @brief
   *
   * @param context
   * @param channel
   * @param index
   * @return OperaWaitingCb
   */
  static auto BuildOperasSign(QSharedPointer<GpgOperaContext>& context,
                              int channel, int index) -> OperaWaitingCb;

  /**
   * @brief
   *
   * @param context
   * @return OperaWaitingCb
   */
  static auto BuildOperasFileSign(QSharedPointer<GpgOperaContext>& context,
                                  int channel, int index) -> OperaWaitingCb;

  /**
   * @brief
   *
   * @param context
   * @param channel
   * @param index
   * @return OperaWaitingCb
   */
  static auto BuildOperasVerify(QSharedPointer<GpgOperaContext>& context,
                                int channel, int index) -> OperaWaitingCb;

  /**
   * @brief
   *
   * @param context
   * @param channel
   * @param index
   * @return OperaWaitingCb
   */
  static auto BuildOperasFileVerify(QSharedPointer<GpgOperaContext>& context,
                                    int channel, int index) -> OperaWaitingCb;

  /**
   * @brief
   *
   * @param context
   * @param channel
   * @param index
   * @return OperaWaitingCb
   */
  static auto BuildOperasEncryptSign(QSharedPointer<GpgOperaContext>& context,
                                     int channel, int index) -> OperaWaitingCb;

  /**
   * @brief
   *
   * @param context
   * @param channel
   * @param index
   */
  static auto BuildOperasFileEncryptSign(
      QSharedPointer<GpgOperaContext>& context, int channel,
      int index) -> OperaWaitingCb;

  /**
   * @brief
   *
   * @param context
   * @param channel
   * @param index
   * @return OperaWaitingCb
   */
  static auto BuildOperasDirectoryEncryptSign(
      QSharedPointer<GpgOperaContext>& context, int channel,
      int index) -> OperaWaitingCb;

  /**
   * @brief
   *
   * @param context
   * @param channel
   * @param index
   * @return OperaWaitingCb
   */
  static auto BuildOperasDecryptVerify(QSharedPointer<GpgOperaContext>& context,
                                       int channel,
                                       int index) -> OperaWaitingCb;

  /**
   * @brief
   *
   * @param context
   * @param channel
   * @param index
   * @return OperaWaitingCb
   */
  static auto BuildOperasFileDecryptVerify(
      QSharedPointer<GpgOperaContext>& context, int channel,
      int index) -> OperaWaitingCb;

  /**
   * @brief
   *
   * @param context
   * @param channel
   * @param index
   * @return OperaWaitingCb
   */
  static auto BuildOperasArchiveDecryptVerify(
      QSharedPointer<GpgOperaContext>& context, int channel,
      int index) -> OperaWaitingCb;

  /**
   * @brief
   *
   * @param parent
   * @param title
   * @param operas
   */
  static void WaitForMultipleOperas(QWidget* parent, const QString& title,
                                    const QContainer<OperaWaitingCb>& operas);
};

}  // namespace GpgFrontend::UI