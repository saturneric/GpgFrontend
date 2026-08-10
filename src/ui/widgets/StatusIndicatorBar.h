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

#include "ui/widgets/StatusIndicatorInfo.h"

namespace GpgFrontend::UI {

/**
 * @brief One clickable reading in the main window's status strip.
 *
 * Draws a dimmed caption followed by the value, both from the palette rather
 * than a stylesheet so it stays legible under either theme. Clicking opens
 * whatever explains the reading, which the tooltip names.
 */
class StatusIndicatorSegment : public QLabel {
  Q_OBJECT

 public:
  /**
   * @brief Construct a new Status Indicator Segment object
   *
   * @param parent the parent widget
   */
  explicit StatusIndicatorSegment(QWidget* parent = nullptr);

  /**
   * @brief Show a new reading, hiding the segment when there is none.
   *
   * @param info caption, value and tooltip to render
   */
  void SetInfo(const StatusIndicatorInfo& info);

 signals:
  /**
   * @brief The segment was clicked with the left button.
   */
  void SignalClicked();

 protected:
  /**
   * @brief Re-render when the palette changes, so the dim colour follows the
   * theme.
   *
   * @param e the event
   */
  void changeEvent(QEvent* e) override;

  /**
   * @brief Underline the value while the pointer is over the segment.
   *
   * @param e the event
   */
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  void enterEvent(QEnterEvent* e) override;
#else
  void enterEvent(QEvent* e) override;
#endif

  /**
   * @brief Drop the underline again.
   *
   * @param e the event
   */
  void leaveEvent(QEvent* e) override;

  /**
   * @brief Emit SignalClicked() for a left button released on the segment.
   *
   * @param e the event
   */
  void mouseReleaseEvent(QMouseEvent* e) override;

 private:
  /**
   * @brief Write the cached reading into the label.
   */
  void render_info();

  StatusIndicatorInfo info_;  ///< the reading, kept for re-rendering
  bool hovered_ = false;      ///< whether the pointer is over the segment
};

/**
 * @brief The strip of readings in the corner of the main window.
 *
 * Which profile, which OpenPGP engine, and whether this session is portable or
 * installed. With two windows open on two profiles, nothing else on screen says
 * which keys are in front of you.
 *
 * No dividers are drawn: platform styles frame status bar items themselves, so
 * drawing our own would double up on Windows.
 */
class StatusIndicatorBar : public QWidget {
  Q_OBJECT

 public:
  /**
   * @brief Construct a new Status Indicator Bar object
   *
   * @param parent the parent widget
   */
  explicit StatusIndicatorBar(QWidget* parent = nullptr);

  /**
   * @brief Set the profile reading.
   *
   * @param info as DescribeProfileIndicator() gives it
   */
  void SetProfile(const StatusIndicatorInfo& info);

  /**
   * @brief Set the engine reading.
   *
   * @param info as DescribeEngineIndicator() gives it
   */
  void SetEngine(const StatusIndicatorInfo& info);

  /**
   * @brief Set the portable or installed reading.
   *
   * @param info as DescribeDeploymentIndicator() gives it
   */
  void SetDeployment(const StatusIndicatorInfo& info);

 signals:
  /**
   * @brief The profile segment was clicked.
   */
  void SignalProfileClicked();

  /**
   * @brief The engine segment was clicked.
   */
  void SignalEngineClicked();

  /**
   * @brief The portable or installed segment was clicked.
   */
  void SignalDeploymentClicked();

 private:
  StatusIndicatorSegment* profile_segment_;     ///< which profile is in front
  StatusIndicatorSegment* engine_segment_;      ///< which backend does the work
  StatusIndicatorSegment* deployment_segment_;  ///< portable or installed
};

}  // namespace GpgFrontend::UI
