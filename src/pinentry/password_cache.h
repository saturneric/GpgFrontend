/* password_cache.h - Password cache support interfaces.
   Copyright (C) 2015 g10 Code GmbH

   This file is part of PINENTRY.

   PINENTRY is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   PINENTRY is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, see <https://www.gnu.org/licenses/>.
   SPDX-License-Identifier: GPL-2.0+
 */

#ifndef PASSWORD_CACHE_H
#define PASSWORD_CACHE_H

void password_cache_save(const char *key_grip, const char *password);

char *password_cache_lookup(const char *key_grip, int *fatal_error);

int password_cache_clear(const char *keygrip);

#endif
