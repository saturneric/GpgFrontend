/* util.h - Helper for managing malloced pointers
 * Copyright (C) 2021 g10 Code GmbH
 *
 * Software engineering by Ingo Klöcker <dev@ingo-kloecker.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses/>.
 * SPDX-License-Identifier: GPL-2.0+
 */

#ifndef __PINENTRY_QT_UTIL_H__
#define __PINENTRY_QT_UTIL_H__

#include <memory>

#include <stdlib.h>

namespace _detail
{
struct FreeDeleter {
    void operator()(void *ptr) const {
        free(ptr);
    }
};
}

template<class T>
using unique_malloced_ptr = std::unique_ptr<T, _detail::FreeDeleter>;

#endif // __PINENTRY_QT_UTIL_H__
