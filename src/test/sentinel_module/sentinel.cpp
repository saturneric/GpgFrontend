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

#include <cstdio>
#include <cstdlib>

/**
 * @file sentinel.cpp
 * @brief A shared library whose loading is observable from outside.
 *
 * It exists for one assertion, and that assertion cannot be made any other
 * way. "Loading returned an error" is not evidence that nothing ran: by the
 * time a check fails the image may already be mapped and its static
 * initialisers executed, and a test asserting only the error code would pass
 * for exactly the wrong reason.
 *
 * So this writes a file the moment it is mapped. A test can then assert the
 * positive control -- that loading a good package DOES produce the file, so
 * the mechanism works -- and then that a refused package never does.
 *
 * It deliberately exports no module entry point. The host will refuse it as a
 * module, which is fine and irrelevant: the claim under test is about whether
 * the constructor ran, not about whether the module was accepted.
 */

namespace {

#if defined(__GNUC__) || defined(__clang__)
__attribute__((constructor))
#endif
void WriteSentinel() {
  const char* path = std::getenv("GPGFRONTEND_TEST_SENTINEL");
  if (path == nullptr || *path == '\0') return;

  FILE* f = std::fopen(path, "wb");
  if (f == nullptr) return;
  std::fputs("loaded", f);
  std::fclose(f);
}

}  // namespace
