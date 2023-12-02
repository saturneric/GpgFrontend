/* Quintuple Agent utilities
 * Copyright (C) 1999 Robert Bihlmeyer <robbe@orcus.priv.at>
 * Copyright (C) 2003 g10 Code GmbH
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

#ifndef _UTIL_H
#define _UTIL_H

#include <sys/types.h>

#ifndef HAVE_TYPE_BYTE
# undef byte
# if !(defined(_WIN32) && defined(cbNDRContext))
   /* Windows typedefs byte in the rpc headers.  Avoid warning about
      double definition.  */
   typedef unsigned char byte;
# endif
# define HAVE_TYPE_BYTE
#endif

#ifndef HAVE_TYPE_ULONG
# undef ulong
  typedef unsigned long ulong;
# define HAVE_TYPE_ULONG
#endif


ssize_t xwrite(int, const void *, size_t); /* write until finished */
int debugmsg(const char *, ...); /* output a debug message if debugging==on */
void drop_privs(void);		/* finally drop privileges */


/* To avoid that a compiler optimizes certain memset calls away, these
   macros may be used instead. */
#define wipememory2(_ptr,_set,_len) do { \
              volatile char *_vptr=(volatile char *)(_ptr); \
              size_t _vlen=(_len); \
              while(_vlen) { *_vptr=(_set); _vptr++; _vlen--; } \
                  } while(0)
#define wipememory(_ptr,_len) wipememory2(_ptr,0,_len)
#define wipe(_ptr,_len)       wipememory2(_ptr,0,_len)




#define xtoi_1(p)   (*(p) <= '9'? (*(p)- '0'): \
                     *(p) <= 'F'? (*(p)-'A'+10):(*(p)-'a'+10))
#define xtoi_2(p)   ((xtoi_1(p) * 16) + xtoi_1((p)+1))


#endif
