/*
  This file is part of MADNESS.

  Copyright (C) 2007,2010 Oak Ridge National Laboratory

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

  For more information please contact:

  Robert J. Harrison
  Oak Ridge National Laboratory
  One Bethel Valley Road
  P.O. Box 2008, MS-6367

  email: harrisonrj@ornl.gov
  tel:   865-241-3937
  fax:   865-572-0680


  $Id: $
*/

#include <world/deferred_cleanup.h>

namespace madness {
    namespace detail {

        void DeferredCleanup::add(const void_ptr& item) {
            mutex.lock();
            deferred.push_back(item);
            mutex.unlock();
        }

        bool DeferredCleanup::do_cleanup() {
            bool deleted_something = false;
            mutex.lock();
            typename void_ptr_list::iterator it = deferred.begin();
            bool at_end = (it == deferred.end());
            mutex.unlock();
            while(! at_end) {
                // Do not lock here so that other pointers can be added to
                // the cleanup list as a side effect of releasing the pointer.
                it->reset();
                deleted_something = true;
                mutex.lock();
                ++it;
                if(it == deferred.end()) {
                    // all the pointers in the list have been reset and we are
                    // at the end.
                    at_end = true;
                    deferred.clear();
                }
                mutex.unlock();
            }

            return deleted_something;
        }

    }  // namespace detail
}  // namespace madness
