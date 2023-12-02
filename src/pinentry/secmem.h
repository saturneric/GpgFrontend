/* Quintuple Agent secure memory allocation
 * Copyright (C) 1998,1999 Free Software Foundation, Inc.
 * Copyright (C) 1999,2000 Robert Bihlmeyer <robbe@orcus.priv.at>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses/>.
 * SPDX-License-Identifier: GPL-2.0+
 */

#ifndef _MEMORY_H
#define _MEMORY_H

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#if 0
}
#endif
#endif


/* values for flags, hardcoded in secmem.c */
#define SECMEM_WARN		0
#define SECMEM_DONT_WARN	1
#define SECMEM_SUSPEND_WARN	2

void secmem_init( size_t npool );
void secmem_term( void );
void *secmem_malloc( size_t size );
void *secmem_realloc( void *a, size_t newsize );
void secmem_free( void *a );
int  m_is_secure( const void *p );
void secmem_dump_stats(void);
void secmem_set_flags( unsigned flags );
unsigned secmem_get_flags(void);
size_t secmem_get_max_size (void);

#if 0
{
#endif
#ifdef __cplusplus
}
#endif
#endif /* _MEMORY_H */
