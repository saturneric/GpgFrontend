/* secmem.c  -	memory allocation from a secure heap
 * Copyright (C) 1998, 1999, 2003 Free Software Foundation, Inc.
 * Copyright (C) 2015 g10 Code GmbH
 *
 * This file is part of GnuPG.
 *
 * GnuPG is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * GnuPG is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses/>.
 * SPDX-License-Identifier: GPL-2.0+
 */

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#ifndef WINDOWS
#include <errno.h>
#endif
#include <stdarg.h>
#include <unistd.h>
#if defined(HAVE_MLOCK) || defined(HAVE_MMAP)
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#endif
#include <string.h>

#include "secmem.h"

#ifdef ORIGINAL_GPG_VERSION
#include "types.h"
#include "util.h"
#else /* ORIGINAL_GPG_VERSION */

#include "secmem_util.h"

typedef union {
  int a;
  short b;
  char c[1];
  long d;
#ifdef HAVE_U64_TYPE
  u64 e;
#endif
  float f;
  double g;
} PROPERLY_ALIGNED_TYPE;

#define log_error log_info
#define log_bug log_fatal

void log_info(char *template_, ...) {
  va_list args;

  va_start(args, template_);
  vfprintf(stderr, template_, args);
  va_end(args);
}

void log_fatal(char *template_, ...) {
  va_list args;

  va_start(args, template_);
  vfprintf(stderr, template_, args);
  va_end(args);
  exit(EXIT_FAILURE);
}

#endif /* ORIGINAL_GPG_VERSION */

#if defined(MAP_ANON) && !defined(MAP_ANONYMOUS)
#define MAP_ANONYMOUS MAP_ANON
#endif

#define DEFAULT_POOLSIZE 16384

typedef struct memblock_struct MEMBLOCK;
struct memblock_struct {
  unsigned size;
  union {
    MEMBLOCK *next;
    PROPERLY_ALIGNED_TYPE aligned;
  } u;
};

static void *pool;
static volatile int pool_okay; /* may be checked in an atexit function */
#if HAVE_MMAP
static int pool_is_mmapped;
#endif
static size_t poolsize; /* allocated length */
static size_t poollen;  /* used length */
static MEMBLOCK *unused_blocks;
static unsigned max_alloced;
static unsigned cur_alloced;
static unsigned max_blocks;
static unsigned cur_blocks;
static int disable_secmem;
static int show_warning;
static int no_warning;
static int suspend_warning;

static void print_warn(void) {
  if (!no_warning) log_info("Warning: using insecure memory!\n");
}

static void lock_pool(void *p, size_t n) {
#if defined(HAVE_MLOCK)
  uid_t uid;
  int err;

  uid = getuid();

#ifdef HAVE_BROKEN_MLOCK
  if (uid) {
    errno = EPERM;
    err = -1;
  } else {
    err = mlock(p, n);
  }
#else
  err = mlock(p, n);
#endif

  if (uid && !geteuid()) {
    if (setuid(uid) || getuid() != geteuid())
      log_fatal("failed to reset uid: %s\n", strerror(errno));
  }

  if (err) {
    if (errno != EPERM
#ifdef EAGAIN /* OpenBSD returns this */
        && errno != EAGAIN
#endif
    )
      log_error("can't lock memory: %s\n", strerror(errno));
    show_warning = 1;
  }

#else
  (void)p;
  (void)n;
  log_info("Please note that you don't have secure memory on this system\n");
#endif
}

static void init_pool(size_t n) {
#if HAVE_MMAP
  size_t pgsize;
#endif

  poolsize = n;

  if (disable_secmem) log_bug("secure memory is disabled");

#if HAVE_MMAP
#ifdef HAVE_GETPAGESIZE
  pgsize = getpagesize();
#else
  pgsize = 4096;
#endif

  poolsize = (poolsize + pgsize - 1) & ~(pgsize - 1);
#ifdef MAP_ANONYMOUS
  pool = mmap(0, poolsize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS,
              -1, 0);
#else /* map /dev/zero instead */
  {
    int fd;

    fd = open("/dev/zero", O_RDWR);
    if (fd == -1) {
      log_error("can't open /dev/zero: %s\n", strerror(errno));
      pool = (void *)-1;
    } else {
      pool = mmap(0, poolsize, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
      close(fd);
    }
  }
#endif
  if (pool == (void *)-1)
    log_info("can't mmap pool of %u bytes: %s - using malloc\n",
             (unsigned)poolsize, strerror(errno));
  else {
    pool_is_mmapped = 1;
    pool_okay = 1;
  }

#endif
  if (!pool_okay) {
    pool = malloc(poolsize);
    if (!pool)
      log_fatal("can't allocate memory pool of %u bytes\n", (unsigned)poolsize);
    else
      pool_okay = 1;
  }
  lock_pool(pool, poolsize);
  poollen = 0;
}

/* concatenate unused blocks */
static void compress_pool(void) { /* fixme: we really should do this */
}

void secmem_set_flags(unsigned flags) {
  int was_susp = suspend_warning;

  no_warning = flags & 1;
  suspend_warning = flags & 2;

  /* and now issue the warning if it is not longer suspended */
  if (was_susp && !suspend_warning && show_warning) {
    show_warning = 0;
    print_warn();
  }
}

unsigned secmem_get_flags(void) {
  unsigned flags;

  flags = no_warning ? 1 : 0;
  flags |= suspend_warning ? 2 : 0;
  return flags;
}

void secmem_init(size_t n) {
  if (!n) {
#if !defined(HAVE_DOSISH_SYSTEM)
    uid_t uid;

    disable_secmem = 1;
    uid = getuid();
    if (uid != geteuid()) {
      if (setuid(uid) || getuid() != geteuid())
        log_fatal("failed to drop setuid\n");
    }
#endif
  } else {
    if (n < DEFAULT_POOLSIZE) n = DEFAULT_POOLSIZE;
    if (!pool_okay)
      init_pool(n);
    else
      log_error("Oops, secure memory pool already initialized\n");
  }
}

void *secmem_malloc(size_t size) {
  MEMBLOCK *mb, *mb2;
  int compressed = 0;

  if (!pool_okay) {
    log_info("operation is not possible without initialized secure memory\n");
    log_info("(you may have used the wrong program for this task)\n");
    exit(2);
  }
  if (show_warning && !suspend_warning) {
    show_warning = 0;
    print_warn();
  }

  /* blocks are always a multiple of 32 */
  size += sizeof(MEMBLOCK);
  size = ((size + 31) / 32) * 32;

retry:
  /* try to get it from the used blocks */
  for (mb = unused_blocks, mb2 = NULL; mb; mb2 = mb, mb = mb->u.next)
    if (mb->size >= size) {
      if (mb2)
        mb2->u.next = mb->u.next;
      else
        unused_blocks = mb->u.next;
      goto leave;
    }
  /* allocate a new block */
  if ((poollen + size <= poolsize)) {
    mb = (MEMBLOCK *)((char *)pool + poollen);
    poollen += size;
    mb->size = size;
  } else if (!compressed) {
    compressed = 1;
    compress_pool();
    goto retry;
  } else
    return NULL;

leave:
  cur_alloced += mb->size;
  cur_blocks++;
  if (cur_alloced > max_alloced) max_alloced = cur_alloced;
  if (cur_blocks > max_blocks) max_blocks = cur_blocks;

  memset(&mb->u.aligned.c, 0,
         size - (size_t) & ((struct memblock_struct *)0)->u.aligned.c);

  return &mb->u.aligned.c;
}

void *secmem_realloc(void *p, size_t newsize) {
  MEMBLOCK *mb;
  size_t size;
  void *a;

  if (!p) return secmem_malloc(newsize);

  mb = (MEMBLOCK *)(void *)((char *)p - offsetof(MEMBLOCK, u.aligned.c));

  size = mb->size;
  if (newsize < size) return p; /* it is easier not to shrink the memory */
  a = secmem_malloc(newsize);
  memcpy(a, p, size);
  memset((char *)a + size, 0, newsize - size);
  secmem_free(p);
  return a;
}

void secmem_free(void *a) {
  MEMBLOCK *mb;
  size_t size;

  if (!a) return;

  mb = (MEMBLOCK *)(void *)((char *)a - offsetof(MEMBLOCK, u.aligned.c));
  size = mb->size;
  /* This does not make much sense: probably this memory is held in the
   * cache. We do it anyway: */
  wipememory2(mb, 0xff, size);
  wipememory2(mb, 0xaa, size);
  wipememory2(mb, 0x55, size);
  wipememory2(mb, 0x00, size);
  mb->size = size;
  mb->u.next = unused_blocks;
  unused_blocks = mb;
  cur_blocks--;
  cur_alloced -= size;
}

int m_is_secure(const void *p) {
  return p >= pool && p < (void *)((char *)pool + poolsize);
}

void secmem_term(void) {
  if (!pool_okay) return;

  wipememory2(pool, 0xff, poolsize);
  wipememory2(pool, 0xaa, poolsize);
  wipememory2(pool, 0x55, poolsize);
  wipememory2(pool, 0x00, poolsize);
#if HAVE_MMAP
  if (pool_is_mmapped) munmap(pool, poolsize);
#endif
  pool = NULL;
  pool_okay = 0;
  poolsize = 0;
  poollen = 0;
  unused_blocks = NULL;
}

void secmem_dump_stats(void) {
  if (disable_secmem) return;
  fprintf(stderr, "secmem usage: %u/%u bytes in %u/%u blocks of pool %lu/%lu\n",
          cur_alloced, max_alloced, cur_blocks, max_blocks, (ulong)poollen,
          (ulong)poolsize);
}

size_t secmem_get_max_size(void) { return poolsize; }
