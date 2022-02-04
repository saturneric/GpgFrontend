/**
 * Copyright (C) 2021 Saturneric
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
 * Saturneric<eric@bktus.com> starting on May 12, 2021.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */
#include "core/function/GpgCommandExecutor.h"
#ifndef WINDOWS
#include <boost/asio.hpp>
#endif

#ifndef WINDOWS

using boost::process::async_pipe;

void GpgFrontend::GpgCommandExecutor::Execute(
    StringArgsRef arguments,
    const std::function<void(async_pipe& in, async_pipe& out)>& interact_func) {
  using namespace boost::process;

  boost::asio::io_service ios;

  std::vector<char> buf;

  async_pipe in_pipe_stream(ios);
  async_pipe out_pipe_stream(ios);

  child child_process(ctx_.GetInfo().AppPath.c_str(), arguments,
                      std_out > in_pipe_stream, std_in < out_pipe_stream);

  boost::asio::async_read(
      in_pipe_stream, boost::asio::buffer(buf),
      [&](const boost::system::error_code& ec, std::size_t size) {
        interact_func(in_pipe_stream, out_pipe_stream);
      });

  ios.run();
  child_process.wait();
  child_process.exit_code();
}

#endif
