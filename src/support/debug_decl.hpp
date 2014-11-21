/*************************************************************************************/
/*      Copyright 2014 Barcelona Supercomputing Center                               */
/*                                                                                   */
/*      This file is part of the NANOS++ library.                                    */
/*                                                                                   */
/*      NANOS++ is free software: you can redistribute it and/or modify              */
/*      it under the terms of the GNU Lesser General Public License as published by  */
/*      the Free Software Foundation, either version 3 of the License, or            */
/*      (at your option); any later version.                                          */
/*                                                                                   */
/*      NANOS++ is distributed in the hope that it will be useful,                   */
/*      but WITHOUT ANY WARRANTY; without even the implied warranty of               */
/*      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                */
/*      GNU Lesser General Public License for more details.                          */
/*                                                                                   */
/*      You should have received a copy of the GNU Lesser General Public License     */
/*      along with NANOS++.  If not, see <http://www.gnu.org/licenses/>.             */
/*************************************************************************************/

#ifndef DEBUG_DECL
#define DEBUG_DECL

namespace nanos {
   template <typename...Ts>
   void fatal( const Ts&... msg );
   
   template <typename...Ts>
   void fatal0( const Ts&... msg );
   
   template <typename...Ts>
   void fatal_cond( bool cond, const Ts&... msg );
   
   template <typename...Ts>
   void fatal_cond0( bool cond, const Ts&... msg );
   
   template <typename...Ts>
   void warning( const Ts&... msg );
   
   template <typename...Ts>
   void warning0( const Ts&... msg );
   
   template <typename...Ts>
   void message( const Ts&... msg );
   
   template <typename...Ts>
   void message0( const Ts&... msg);
   
   template <typename T>
   void messageMaster( T msg );
   
   template <typename T>
   void message0Master( T msg );
   
   template <typename...Ts>
   void ensure( bool cond, const Ts&... msg );
   
   template <typename...Ts>
   void ensure0( bool cond, const Ts&... msg );
   
   template <typename...Ts>
   void verbose( const Ts&... msg );
   
   template <typename...Ts>
   void verbose0( const Ts&... msg);
   
   template <typename...Ts>
   void debug( const Ts&... msg );
   
   template <typename...Ts>
   void debug0( const Ts&... msg);
};
#endif // DEBUG_DECL
