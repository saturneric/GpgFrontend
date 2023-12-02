/* STL allocator for secmem
 * Copyright (C) 2008 Marc Mutz <marc@kdab.com>
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

#ifndef __SECMEM_SECMEMPP_H__
#define __SECMEM_SECMEMPP_H__

#include "../secmem/secmem.h"
#include <cstddef>

namespace secmem {

    template <typename T>
    class alloc {
    public:
        // type definitions:
        typedef size_t    size_type;
        typedef ptrdiff_t difference_type;
        typedef T*        pointer;
        typedef const T*  const_pointer;
        typedef T&        reference;
        typedef const T&  const_reference;
        typedef T         value_type;

        // rebind
        template <typename U>
        struct rebind {
            typedef alloc<U> other;
        };

        // address
        pointer address( reference value ) const {
            return &value;
        }
        const_pointer address( const_reference value ) const {
            return &value;
        }

        // (trivial) ctors and dtors
        alloc() {}
        alloc( const alloc & ) {}
        template <typename U> alloc( const alloc<U> & ) {}
        // copy ctor is ok
        ~alloc() {}

        // de/allocation
        size_type max_size() const {
            return secmem_get_max_size();
        }

        pointer allocate( size_type n, void * =0 ) {
            return static_cast<pointer>( secmem_malloc( n * sizeof(T) ) );
        }

        void deallocate( pointer p, size_type ) {
            secmem_free( p );
        }

        // de/construct
        void construct( pointer p, const T & value ) {
            void * loc = p;
            new (loc)T(value);
        }
        void destruct( pointer p ) {
            p->~T();
        }
    };

    // equality comparison
    template <typename T1,typename T2>
    bool operator==( const alloc<T1> &, const alloc<T2> & ) { return true; }
    template <typename T1, typename T2>
    bool operator!=( const alloc<T1> &, const alloc<T2> & ) { return false; }

}

#endif /* __SECMEM_SECMEMPP_H__ */
