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

include $(top_srcdir)/src/common.am

devinclude_HEADERS = \
	containeradapter_fwd.hpp \
	bitcounter_fwd.hpp \
	region_fwd.hpp \
	regionbuilder_fwd.hpp \
	regioncollection_fwd.hpp \
	regionpart_fwd.hpp \
	regiontree_fwd.hpp \
	regiontreenode_fwd.hpp \
	new_decl.hpp\
	allocator_fwd.hpp\
	allocator_decl.hpp\
	allocator.hpp\
	atomic_decl.hpp\
	atomic.hpp\
	atomic_flag.hpp\
	lock_decl.hpp\
	lock.hpp\
	recursivelock_decl.hpp\
	lazy.hpp\
	lazy_decl.hpp\
	compatibility.hpp\
	queue_decl.hpp\
	queue.hpp\
	debug.hpp\
	config_fwd.hpp\
	config_decl.hpp\
	config.hpp\
	functors_decl.hpp \
	functors.hpp \
	plugin_decl.hpp \
	plugin.hpp \
	archplugin_decl.hpp \
	archplugin.hpp \
	xstring.hpp \
	simpleallocator_fwd.hpp \
	simpleallocator_decl.hpp \
	simpleallocator.hpp \
	list_decl.hpp \
	list.hpp \
	hashfunction_decl.hpp \
	hashmap.hpp \
	hashmap_decl.hpp \
	copydescriptor.hpp \
	copydescriptor_decl.hpp \
	malign.hpp \
	memorymap_decl.hpp \
	memorymap.hpp \
	packer_decl.hpp \
	containeradapter_fwd.hpp \
	containeradapter_decl.hpp \
	containeradapter.hpp \
	containertraits.hpp \
	bitcounter_fwd.hpp \
	bitcounter_decl.hpp \
	bitcounter.hpp \
	region_fwd.hpp \
	region_decl.hpp \
	region.hpp \
	regionbuilder_fwd.hpp \
	regionbuilder_decl.hpp \
	regionbuilder.hpp \
	regioncollection_fwd.hpp \
	regioncollection_decl.hpp \
	regioncollection.hpp \
	regionpart_fwd.hpp \
	regionpart_decl.hpp \
	regionpart.hpp \
	regiontree_fwd.hpp \
	regiontree_decl.hpp \
	regiontree.hpp \
	regiontreenode_fwd.hpp \
	regiontreenode_decl.hpp \
	regiontreenode.hpp \
	functor_decl.hpp \
	requestqueue_decl.hpp \
	requestqueue.hpp \
	smpbaseplugin_decl.hpp \
	pinnedallocator_decl.hpp \
	genericevent_decl.hpp \
	hwloc_decl.hpp \
	error.hpp \
	frequency_traits.hpp \
	frequency.hpp \
	$(END)

memorydir = $(devincludedir)/memory
memory_HEADERS= \
	memory/accessrights.hpp \
	memory/blockedpage.hpp \
	memory/memoryaddress.hpp \
	memory/memorychunk.hpp \
	memory/memorypage.hpp \
	$(END) 

exceptiondir = $(devincludedir)/exception
exception_HEADERS= \
	exception/checkpointfailure.hpp \
	exception/exceptiontracer.hpp \
	exception/executionfailure.hpp \
	exception/failurestats.hpp \
	exception/genericexception.hpp \
	exception/invalidatedregionfound.hpp \
	exception/mpierror.hpp \
	exception/mpierrorhandler.hpp \
	exception/mpierrortranslator.hpp \
	exception/operationfailure.hpp \
	exception/restorefailure.hpp \
	exception/signalexception.hpp \
	exception/signalinfo.hpp \
	exception/signaltranslator.hpp \
	exception/taskrecoveryfailed.hpp \
	$(END)

error_injectiondir = $(devincludedir)/error-injection
error_injection_HEADERS= \
	error-injection/errorinjectionconfiguration.hpp \
	error-injection/errorinjectioninterface.hpp \
	error-injection/errorinjectionplugin.hpp \
	error-injection/errorinjectionpolicy.hpp \
	error-injection/errorinjectionthread.hpp \
	error-injection/periodicinjectionpolicy.hpp \
	error-injection/blockaccessinjector.hpp \
	error-injection/stubinjector.hpp \
	$(END)

support_sources = \
	new_decl.hpp\
	new.cpp\
	allocator_fwd.hpp\
	allocator_decl.hpp\
	allocator.hpp\
	allocator.cpp\
	atomic_decl.hpp\
	atomic.hpp\
	atomic_flag.hpp\
	lock_decl.hpp\
	lock.hpp\
	recursivelock_decl.hpp\
	recursivelock.cpp\
	lazy.hpp\
	lazy_decl.hpp\
	compatibility.hpp\
	queue_decl.hpp\
	queue.hpp\
	debug.hpp\
	debug.cpp\
	config_fwd.hpp\
	config_decl.hpp\
	config.hpp\
	config.cpp \
	functors_decl.hpp \
	functors.hpp \
	plugin_decl.hpp \
	plugin.hpp \
	plugin.cpp \
	archplugin_decl.hpp \
	archplugin.hpp \
	archplugin.cpp \
	xstring.hpp \
	simpleallocator_fwd.hpp \
	simpleallocator_decl.hpp \
	simpleallocator.hpp \
	simpleallocator.cpp \
	list_decl.hpp \
	list.hpp \
	hashmap.hpp \
	hashmap_decl.hpp \
	copydescriptor.hpp \
	copydescriptor_decl.hpp \
	malign.hpp \
	memorymap_decl.hpp \
	memorymap.hpp \
	memorymap.cpp \
	containeradapter.hpp \
	containertraits.hpp \
	packer_decl.hpp \
	packer.cpp \
	containeradapter_fwd.hpp \
	containeradapter_decl.hpp \
	containeradapter.hpp \
	containertraits.hpp \
	bitcounter_fwd.hpp \
	bitcounter_decl.hpp \
	bitcounter.hpp \
	region_fwd.hpp \
	region_decl.hpp \
	region.hpp \
	region.cpp \
	depsregion.hpp \
	depsregion_decl.hpp \
	regionbuilder_fwd.hpp \
	regionbuilder_decl.hpp \
	regionbuilder.hpp \
	regionbuilder.cpp \
	regioncollection_fwd.hpp \
	regioncollection_decl.hpp \
	regioncollection.hpp \
	regionpart_fwd.hpp \
	regionpart_decl.hpp \
	regionpart.hpp \
	regiontree_fwd.hpp \
	regiontree_decl.hpp \
	regiontree.hpp \
	regiontreenode_fwd.hpp \
	regiontreenode_decl.hpp \
	regiontreenode.hpp \
	functor_decl.hpp \
	requestqueue_decl.hpp \
	requestqueue.hpp \
	genericevent_decl.hpp \
	genericevent.cpp \
	hwloc_decl.hpp \
	hwloc.cpp \
	dbmanager.hpp \
	error.hpp \
	frequency_traits.hpp \
	frequency.hpp \
	$(error_injection_HEADERS) \
	error-injection/errorinjectioninterface.cpp \
	$(exception_HEADERS) \
	exception/failurestats.cpp \
	exception/genericexception.cpp \
	exception/signalexception.cpp \
	$(memory_HEADERS) \
	$(END)

mpi_exception_sources = \
	exception/mpierror.cpp \
	exception/mpierror.hpp \
	exception/mpierrorhandler.hpp \
	exception/mpierrortranslator.hpp \
	$(END)

if SQLITE3_SUPPORT
support_sources += \
	dbmanager_sqlite3.hpp \
	dbmanager_sqlite3.cpp \
	$(END)
else
	dbmanager_sqlite3_empty.hpp \
	dbmanager_sqlite3_empty.cpp \
	$(END)
endif

debug_sources = \
	memtracker_fwd.hpp\
	memtracker_decl.hpp\
	memtracker.hpp\
	memtracker.cpp
        $(END)

if GPU_SUPPORT
gpu_support_sources = \
	pinnedallocator_decl.hpp \
	pinnedallocator.hpp \
	pinnedallocator.cpp \
        $(END)
endif
	

noinst_LTLIBRARIES =

if is_debug_enabled
noinst_LTLIBRARIES += debug/libsupport.la

debug_libsupport_la_CPPFLAGS=$(common_debug_CPPFLAGS) @dlbinc@ @cudainc@ @sqlite3inc@
debug_libsupport_la_CXXFLAGS=$(common_debug_CXXFLAGS)
debug_libsupport_la_SOURCES=$(support_sources) $(debug_sources) $(gpu_support_sources)
debug_libsupport_la_LDFLAGS=$(AM_LDFLAGS) @dlblib@ @cudalib@ @sqlite3lib@
debug_libsupport_la_LIBADD=@dlb_dbg@ @sqlite3libs@
endif

if is_instrumentation_debug_enabled
noinst_LTLIBRARIES += instrumentation-debug/libsupport.la

instrumentation_debug_libsupport_la_CPPFLAGS=$(common_instrumentation_debug_CPPFLAGS) @dlbinc@ @cudainc@ @sqlite3inc@
instrumentation_debug_libsupport_la_CXXFLAGS=$(common_instrumentation_debug_CXXFLAGS)
instrumentation_debug_libsupport_la_SOURCES=$(support_sources) $(instrumentation_debug_sources) $(gpu_support_sources)
instrumentation_debug_libsupport_la_LDFLAGS=$(AM_LDFLAGS) @dlblib@ @cudalib@ @sqlite3lib@
instrumentation_debug_libsupport_la_LIBADD=@dlb_instr_dbg@ @sqlite3libs@
endif

if is_instrumentation_enabled
noinst_LTLIBRARIES += instrumentation/libsupport.la

instrumentation_libsupport_la_CPPFLAGS=$(common_instrumentation_CPPFLAGS) @dlbinc@ @cudainc@ @sqlite3inc@
instrumentation_libsupport_la_CXXFLAGS=$(common_instrumentation_CXXFLAGS)
instrumentation_libsupport_la_SOURCES=$(support_sources) $(instrumentation_sources) $(gpu_support_sources)
instrumentation_libsupport_la_LDFLAGS=$(AM_LDFLAGS) @dlblib@ @cudalib@ @sqlite3lib@
instrumentation_libsupport_la_LIBADD=@dlb_instr@ @sqlite3libs@
endif

if is_performance_enabled
noinst_LTLIBRARIES += performance/libsupport.la

performance_libsupport_la_CPPFLAGS=$(common_performance_CPPFLAGS) @dlbinc@ @cudainc@ @sqlite3inc@
performance_libsupport_la_CXXFLAGS=$(common_performance_CXXFLAGS)
performance_libsupport_la_SOURCES=$(support_sources) $(performance_sources) $(gpu_support_sources)
performance_libsupport_la_LDFLAGS=$(AM_LDFLAGS) @dlblib@ @cudalib@ @sqlite3lib@
performance_libsupport_la_LIBADD=@dlb@ @sqlite3libs@
endif

