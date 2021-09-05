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

#ifndef GPGFRONTEND_H_IN
#define GPGFRONTEND_H_IN

/**
 * Platform Vars
 */
#define WINDOWS 0
#define MACOS 1
#define LINUX 2

#define OS_PLATFORM 2

/**
 * Build Options Vars
 */
#define RELEASE 0
#define DEBUG 1

/**
 * Resources File(s) Path Vars
 */
#if OS_PLATFORM == MACOS && BUILD_FLAG == RELEASE
#define RESOURCE_DIR(appDir) (appDir + "/../Resources/")
#elif OS_PLATFORM == LINUX && BUILD_FLAG == RELEASE
#define RESOURCE_DIR(appDir) (appDir + "/../share/")
#else
#define RESOURCE_DIR(appDir) (appDir)
#endif

#endif // GPGFRONTEND_H_IN
