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

#ifndef GPGFRONTEND_GPGFRONTENDCORE_H
#define GPGFRONTEND_GPGFRONTENDCORE_H

#include "GpgFrontend.h"

// gnupg
#include <gpgme.h>

// std includes
#include <cassert>
#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <random>
#include <set>
#include <shared_mutex>
#include <stdexcept>
#include <string>
#include <typeinfo>
#include <utility>
#include <vector>

// boost includes
#include <boost/date_time.hpp>
#include <boost/date_time/posix_time/conversion.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/format.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>

// Qt includes
#include <QtCore>

// libconfig includes
#include <libconfig.h++>

// libarchive includes
#include <libarchive/libarchive/archive.h>
#include <libarchive/libarchive/archive_entry.h>

// json includes
#include <nlohmann/json.hpp>

// dll export macro
#include "GpgFrontendCoreExport.h"

#endif  // GPGFRONTEND_GPGFRONTENDCORE_H
