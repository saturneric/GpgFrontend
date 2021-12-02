/**
 * This file is part of GPGFrontend.
 *
 * GPGFrontend is free software: you can redistribute it and/or modify
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

#ifndef GPGFRONTEND_BUILD_INFO_H_IN
#define GPGFRONTEND_BUILD_INFO_H_IN

/**
 * Logic Version (*.*.*)
 */
#define VERSION_MAJOR 1
#define VERSION_MINOR 3
#define VERSION_PATCH 1

/**
 * Code Version (According to Git)
 */
#define GIT_BRANCH_NAME "main"
#define GIT_COMMIT_HASH "d0146c8c640b2a7ec9a62c3c87a8f5e7c84f1fe7"

/**
 * Generated Information (According to CMake)
 */
#define PROJECT_NAME "GpgFrontend"
#define BUILD_VERSION "1.3.1_Darwin-20.5.0_x86_64_Debug"
#define GIT_VERSION "main_d0146c8c640b2a7ec9a62c3c87a8f5e7c84f1fe7"

/**
 * Build Information
 */
#define BUILD_FLAG 1
#define BUILD_TIMESTAMP "2021-08-28 12:37:33"

#endif // GPGFRONTEND_BUILD_INFO_H_IN
