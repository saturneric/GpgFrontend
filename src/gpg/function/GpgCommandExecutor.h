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

#ifndef GPGFRONTEND_ZH_CN_TS_GPGCOMMANDEXECUTOR_H
#define GPGFRONTEND_ZH_CN_TS_GPGCOMMANDEXECUTOR_H

#ifndef WINDOWS
#include <boost/process.hpp>
#endif

#include "gpg/GpgContext.h"
#include "gpg/GpgFunctionObject.h"

namespace GpgFrontend {

/**
 * @brief Extra commands related to GPG
 *
 */
class GpgCommandExecutor : public SingletonFunctionObject<GpgCommandExecutor> {
 public:
  /**
   * @brief Construct a new Gpg Command Executor object
   *
   * @param channel Corresponding context
   */
  explicit GpgCommandExecutor(
      int channel = SingletonFunctionObject::GetDefaultChannel())
      : SingletonFunctionObject<GpgCommandExecutor>(channel) {}

#ifndef WINDOWS

  /**
   * @brief Excuting an order
   *
   * @param arguments Command parameters
   * @param interact_func Command answering function
   */
  void Execute(StringArgsRef arguments,
               const std::function<void(boost::process::async_pipe &in,
                                        boost::process::async_pipe &out)>
                   &interact_func);
#endif

 private:
  GpgContext &ctx = GpgContext::GetInstance(
      SingletonFunctionObject::GetChannel());  ///< Corresponding context
};

}  // namespace GpgFrontend

#endif  // GPGFRONTEND_ZH_CN_TS_GPGCOMMANDEXECUTOR_H
