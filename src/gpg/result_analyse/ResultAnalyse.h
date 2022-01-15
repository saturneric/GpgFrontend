/**
 * This file is part of GpgFrontend.
 *
 * GpgFrontend is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Foobar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Foobar.  If not, see <https://www.gnu.org/licenses/>.
 *
 * The initial version of the source code is inherited from gpg4usb-team.
 * Their source code version also complies with GNU General Public License.
 *
 * The source code version of this software was modified and released
 * by Saturneric<eric@bktus.com> starting on May 12, 2021.
 *
 */
#ifndef GPGFRONTEND_RESULTANALYSE_H
#define GPGFRONTEND_RESULTANALYSE_H

#include <sstream>
#include <string>

#include "gpg/GpgConstants.h"
namespace GpgFrontend {

class ResultAnalyse {
 public:
  /**
   * @brief Construct a new Result Analyse object
   *
   */
  ResultAnalyse() = default;

  /**
   * @brief Get the Result Report object
   *
   * @return const std::string
   */
  [[nodiscard]] const std::string GetResultReport() const;

  /**
   * @brief Get the Status object
   *
   * @return int
   */
  [[nodiscard]] int GetStatus() const;

  /**
   * @brief
   *
   */
  void Analyse();

 protected:
  /**
   * @brief
   *
   */
  virtual void do_analyse() = 0;

  /**
   * @brief Set the status object
   *
   * @param m_status
   */
  void set_status(int m_status);

  std::stringstream stream_;  ///<
  int status_ = 1;            ///<
  bool analysed_ = false;     ///<
};

}  // namespace GpgFrontend

#endif  // GPGFRONTEND_RESULTANALYSE_H
