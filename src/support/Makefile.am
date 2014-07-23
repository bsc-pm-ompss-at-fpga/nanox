#####################################################################################
#      Copyright 2009 Barcelona Supercomputing Center                               #
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

include ../common.am

devinclude_HEADERS = \
	dlb.hpp\
	new_decl.hpp\
	allocator_fwd.hpp\
	allocator_decl.hpp\
	allocator.hpp\
	atomic_decl.hpp\
	atomic.hpp\
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
	printbt_decl.hpp \
        smpbaseplugin_decl.hpp \
	pinnedallocator_decl.hpp \
	$(END) 

if RESILIENCY
if HAVE_CXX11
devinclude_HEADERS += \
	mpoison.h
endif
endif

support_sources = \
	dlb.hpp\
	new_decl.hpp\
	new.cpp\
	allocator_fwd.hpp\
	allocator_decl.hpp\
	allocator.hpp\
	allocator.cpp\
	atomic_decl.hpp\
	atomic.hpp\
	lazy.hpp\
	lazy_decl.hpp\
	compatibility.hpp\
	queue_decl.hpp\
	queue.hpp\
	debug.hpp\
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
	printbt_decl.hpp \
	printbt.cpp \
	$(END) 

debug_sources = \
	memtracker_fwd.hpp\
	memtracker_decl.hpp\
	memtracker.hpp\
	memtracker.cpp
	$(END)

gpu_support_sources = \
	pinnedallocator_decl.hpp \
	pinnedallocator.hpp \
	pinnedallocator.cpp \
	$(END)
	

noinst_LTLIBRARIES =

libmpoison_sources = \
	vmentry.hpp \
	vmentry.cpp \
        mpoison.h \
	mpoison.hpp \
	mpoison.cpp \
	mpoisonmanager.hpp \
	mpoisonmanager.cpp


if DEBUG_VERSION
noinst_LTLIBRARIES += debug/libsupport.la

nanos_debug_cxxflags=$(common_includes) $(common_debug_cxxflags) 

debug_libsupport_la_CXXFLAGS=$(nanos_debug_cxxflags)

debug_libsupport_la_SOURCES=$(support_sources) $(debug_sources)

if GPU_SUPPORT
debug_libsupport_la_SOURCES+=$(gpu_support_sources)
endif

if RESILIENCY
if HAVE_CXX11
noinst_LTLIBRARIES += debug/libmpoison.la
debug_libsupport_la_LIBADD = debug/libmpoison.la
debug_libmpoison_la_SOURCES=$(libmpoison_sources)
debug_libmpoison_la_CXXFLAGS=$(nanos_debug_cxxflags) $(CXXFLAGS_CXX11)
endif
endif
endif

if INSTRUMENTATION_VERSION
noinst_LTLIBRARIES += instrumentation/libsupport.la

nanos_instr_cxxflags=$(common_includes) $(common_instr_cxxflags) 

instrumentation_libsupport_la_CXXFLAGS=$(nanos_instr_cxxflags)

instrumentation_libsupport_la_SOURCES=$(support_sources)

if GPU_SUPPORT
instrumentation_libsupport_la_SOURCES+=$(gpu_support_sources)
endif

if RESILIENCY
if HAVE_CXX11
noinst_LTLIBRARIES += instrumentation/libmpoison.la
instrumentation_libsupport_la_LIBADD = instrumentation/libmpoison.la
instrumentation_libmpoison_la_SOURCES=$(libmpoison_sources)
instrumentation_libmpoison_la_CXXFLAGS=$(nanos_instr_cxxflags) $(CXXFLAGS_CXX11)
endif
endif
endif

if INSTRUMENTATION_DEBUG_VERSION
noinst_LTLIBRARIES += instrumentation-debug/libsupport.la

nanos_instrdebug_cxxflags=$(common_includes) $(common_instrdebug_cxxflags) 

instrumentation_debug_libsupport_la_CXXFLAGS=$(nanos_instrdebug_cxxflags)

instrumentation_debug_libsupport_la_SOURCES=$(support_sources) $(debug_sources)

if GPU_SUPPORT
instrumentation_debug_libsupport_la_SOURCES+=$(gpu_support_sources)
endif

if RESILIENCY
if HAVE_CXX11
noinst_LTLIBRARIES += instrumentation-debug/libmpoison.la
instrumentation_debug_libsupport_la_LIBADD = instrumentation-debug/libmpoison.la
instrumentation_debug_libmpoison_la_SOURCES=$(libmpoison_sources)
instrumentation_debug_libmpoison_la_CXXFLAGS=$(nanos_instrdebug_cxxflags) $(CXXFLAGS_CXX11)
endif
endif
endif

if PERFORMANCE_VERSION
noinst_LTLIBRARIES += performance/libsupport.la

nanos_perfo_cxxflags=$(common_includes) $(common_perfo_cxxflags) 

performance_libsupport_la_CXXFLAGS=$(nanos_perfo_cxxflags)

performance_libsupport_la_SOURCES=$(support_sources)

if GPU_SUPPORT
performance_libsupport_la_SOURCES+=$(gpu_support_sources)
endif

if RESILIENCY
if HAVE_CXX11
noinst_LTLIBRARIES += performance/libmpoison.la
performance_libsupport_la_LIBADD = performance/libmpoison.la
performance_libmpoison_la_SOURCES=$(libmpoison_sources)
performance_libmpoison_la_CXXFLAGS=$(nanos_perfo_cxxflags) $(CXXFLAGS_CXX11)
endif
endif
endif

