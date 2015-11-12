/*************************************************************************************/
/*      Copyright 2014 Barcelona Supercomputing Center                               */
/*                                                                                   */
/*      This file is part of the NANOS++ library.                                    */
/*                                                                                   */
/*      NANOS++ is free software: you can redistribute it and/or modify              */
/*      it under the terms of the GNU Lesser General Public License as published by  */
/*      the Free Software Foundation, either version 3 of the License, or            */
/*      (at your option) any later version.                                          */
/*                                                                                   */
/*      NANOS++ is distributed in the hope that it will be useful,                   */
/*      but WITHOUT ANY WARRANTY; without even the implied warranty of               */
/*      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                */
/*      GNU Lesser General Public License for more details.                          */
/*                                                                                   */
/*      You should have received a copy of the GNU Lesser General Public License     */
/*      along with NANOS++.  If not, see <http://www.gnu.org/licenses/>.             */
/*************************************************************************************/
#ifndef MPOISON_HPP
#include "mpoison.h"
#include "mpoisonmanager.hpp"

#define MPOISON_SCAN_MODE 0
#define MPOISON_TRACK_MODE 1
namespace nanos {
namespace vm {

/* Grants access to the object that controls allocated memory
 * and poisoned memory
 */
MPoisonManager* getMPoisonManager();


/* Function executed by mpoison thread */
void *mpoison_run( void* );

}// namespace vm
}// namespace nanos

#endif // MPOISON_HPP
