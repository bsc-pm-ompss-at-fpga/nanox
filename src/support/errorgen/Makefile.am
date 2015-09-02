#####################################################################################
#      Copyright 2015 Barcelona Supercomputing Center                               #
#                                                                                   #
#      This file is part of the NANOS++ library.                                    #
#                                                                                   #
#      NANOS++ is free software: you can redistribute it and/or modify              #
#      it under the terms of the GNU Lesser General Public License as published by  #
#      the Free Software Foundation, either version 3 of the License, or            #
#      (at your option) any later version.                                          #
#                                                                                   #
#      NANOS++ is distributed in the hope that it will be useful,                   #
#      but WITHOUT ANY WARRANTY; without even the implied warranty of               #
#      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                #
#      GNU Lesser General Public License for more details.                          #
#                                                                                   #
#      You should have received a copy of the GNU Lesser General Public License     #
#      along with NANOS++.  If not, see <http://www.gnu.org/licenses/>.             #
#####################################################################################

errorgen_sources = \
	errorgen.hpp \
	errorgen.cpp \
	$(END) 

flagcxx11=-std=c++11
#flagcxx11=

noinst_LTLIBRARIES =

if DEBUG_VERSION
noinst_LTLIBRARIES += debug/liberrorgen.la

debug_liberrorgen_la_CXXFLAGS=$(flagcxx11)
debug_liberrorgen_la_SOURCES=$(errorgen_sources)
endif

if INSTRUMENTATION_VERSION
noinst_LTLIBRARIES += instrumentation/liberrorgen.la

instrumentation_liberrorgen_la_CXXFLAGS=$(flagcxx11)
instrumentation_liberrorgen_la_SOURCES=$(errorgen_sources)
endif

if INSTRUMENTATION_DEBUG_VERSION
noinst_LTLIBRARIES += instrumentation-debug/liberrorgen.la

instrumentation_debug_liberrorgen_la_CXXFLAGS=$(flagcxx11)
instrumentation_debug_liberrorgen_la_SOURCES=$(errorgen_sources)
endif

if PERFORMANCE_VERSION
noinst_LTLIBRARIES += performance/liberrorgen.la

performance_liberrorgen_la_CXXFLAGS=$(flagcxx11)
performance_liberrorgen_la_SOURCES=$(errorgen_sources)
endif
