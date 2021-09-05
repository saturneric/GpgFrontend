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
#define GIT_BRANCH_NAME "develop-core"
#define GIT_COMMIT_HASH "f8668adc7fe319896a3444409c076bd61c690c84"

/**
 * Generated Information (According to CMake)
 */
#define PROJECT_NAME "GpgFrontend"
#define BUILD_VERSION "1.3.1_Linux-5.4.0-81-generic_x86_64_Debug"
#define GIT_VERSION "develop-core_f8668adc7fe319896a3444409c076bd61c690c84"

/**
 * Build Information
 */
#define BUILD_FLAG 1
#define BUILD_TIMESTAMP "2021-09-05 21:22:47"

#endif // GPGFRONTEND_BUILD_INFO_H_IN
