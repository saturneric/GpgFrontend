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

#include "GFCoreExport.h"

namespace GpgFrontend {

/**
 * @brief Abstract base class for accumulating a formatted report of an OpenPGP
 * operation result.
 *
 * Subclasses implement doAnalyse() to write Markdown-formatted text into
 * stream_, which is backed by buffer_. The accumulated report is retrieved via
 * GetResultReport(). An integer status tracks the worst-case outcome seen so
 * far: 1 = success, 0 = warning or partial success, -1 = failure, -2 = cannot
 * verify (e.g. key missing). Analyse() is idempotent; doAnalyse() is called at
 * most once.
 */
class GF_CORE_EXPORT GpgResultAnalyse : public QObject {
  Q_OBJECT
 public:
  /**
   * @brief Construct the analyser for the given OpenPGP context channel.
   *
   * @param channel channel ID of the OpenPGP context whose result is being
   * analysed
   */
  explicit GpgResultAnalyse(int channel)
      : current_gpg_context_channel_(channel){};

  /**
   * @brief Return the Markdown-formatted operation report.
   *
   * Call Analyse() before this to ensure the report has been generated.
   *
   * @return formatted report string accumulated by doAnalyse()
   */
  [[nodiscard]] auto GetResultReport() const -> QString;

  /**
   * @brief Return the accumulated worst-case status code.
   *
   * Status values: 1 = success, 0 = warning/partial, -1 = failure,
   * -2 = cannot verify (key missing). The value only decreases; it is never
   * raised by setStatus().
   *
   * @return current status code
   */
  [[nodiscard]] auto GetStatus() const -> int;

  /**
   * @brief Return the OpenPGP context channel used by this analyser.
   *
   * @return channel ID
   */
  [[nodiscard]] auto GetChannel() const -> int;

  /**
   * @brief Return a human-readable engine identifier string (e.g. "GPG
   * v2.4.1").
   *
   * @return engine name and version string from the current channel's OpenPGP
   * context
   */
  [[nodiscard]] auto EngineInfo() const -> QString;

  /**
   * @brief Run the analysis, populating the report and status. Idempotent.
   *
   * Calls doAnalyse() on the first invocation and sets a flag to prevent
   * subsequent calls from re-running the analysis.
   */
  void Analyse();

 protected:
  /**
   * @brief Perform the operation-specific analysis and write output to stream_.
   *
   * Implemented by each concrete subclass. Called exactly once by Analyse().
   */
  virtual void doAnalyse() = 0;

  /**
   * @brief Lower the accumulated status to at most @p m_status.
   *
   * Takes effect only when @p m_status is less than the current status_,
   * so the status can only worsen over the lifetime of the analyser.
   *
   * @param m_status new candidate status value
   */
  void setStatus(int m_status);

  // OpenPGP context channel for this analysis
  int current_gpg_context_channel_;
  // Backing buffer for stream_
  QString buffer_;
  // Output stream written to by doAnalyse()
  QTextStream stream_ = QTextStream(&buffer_);
  // Accumulated worst-case status (starts at 1 = success)
  int status_ = 1;
  // True after Analyse() has been called once
  bool analysed_ = false;
};

}  // namespace GpgFrontend
