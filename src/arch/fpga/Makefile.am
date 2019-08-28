#####################################################################################
#      Copyright 2009-2018 Barcelona Supercomputing Center                          #
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
#      along with NANOS++.  If not, see <https://www.gnu.org/licenses/>.            #
#####################################################################################

include $(top_srcdir)/src/common.am

EXTRA_DIST= \
	$(END)

pkginclude_HEADERS=\
	nanos-fpga.h \
	$(END)

devinclude_HEADERS = \
	fpgaconfig.hpp \
	fpgadd.hpp \
	fpgadevice_fwd.hpp \
	fpgadevice.hpp \
	fpgalistener.hpp \
	fpgaprocessor.hpp \
	fpgaprocessorinfo.hpp \
	fpgathread.hpp \
	fpgawd_decl.hpp \
	fpgaworker.hpp \
	fpgapinnedallocator.hpp \
	fpgainstrumentation.hpp \
	libxtasks_wrapper.hpp \
	$(END)

fpga_api_sources = \
	nanos-fpga.h \
	fpga-api.cpp \
	$(END)

fpga_sources = \
	fpgaconfig.hpp \
	fpgaconfig.cpp \
	fpgadd.hpp \
	fpgadd.cpp \
	fpgadevice_fwd.hpp \
	fpgadevice.hpp \
	fpgadevice.cpp \
	fpgaprocessor.hpp \
	fpgaprocessor.cpp \
	fpgaprocessorinfo.hpp \
	fpgathread.hpp \
	fpgathread.cpp \
	fpgawd_decl.hpp \
	fpgawd.cpp \
	fpgaworker.hpp \
	fpgaworker.cpp \
	fpgapinnedallocator.hpp \
	fpgapinnedallocator.cpp \
	fpgainstrumentation.hpp \
	fpgainstrumentation.cpp \
	fpgalistener.hpp \
	fpgalistener.cpp \
	libxtasks_wrapper.hpp \
	$(END)

pe_fpga_sources = \
	fpgaplugin.cpp \
	$(END)

noinst_LTLIBRARIES =
debug_LTLIBRARIES =
instrumentation_LTLIBRARIES =
instrumentation_debug_LTLIBRARIES =
performance_LTLIBRARIES =

if is_debug_enabled
debug_LTLIBRARIES += debug/libnanox-fpga-core.la debug/libnanox-fpga-api.la debug/libnanox-pe-fpga.la
debug_fpga_core_LIBS = debug/libnanox-fpga-core.la

debug_libnanox_fpga_core_la_CPPFLAGS=$(common_debug_CPPFLAGS) @xtasksinc@
debug_libnanox_fpga_core_la_CXXFLAGS=$(common_debug_CXXFLAGS)
debug_libnanox_fpga_core_la_LDFLAGS=$(AM_LDFLAGS) @xtaskslib@
debug_libnanox_fpga_core_la_LIBADD=@xtaskslibs@
debug_libnanox_fpga_core_la_SOURCES=$(fpga_sources)

debug_libnanox_fpga_api_la_CPPFLAGS=$(common_debug_CPPFLAGS) $(api_includes) @xtasksinc@
debug_libnanox_fpga_api_la_CXXFLAGS=$(common_debug_CXXFLAGS)
debug_libnanox_fpga_api_la_LDFLAGS=$(AM_LDFLAGS) $(version_flags)
debug_libnanox_fpga_api_la_LIBADD=$(debug_fpga_core_LIBS)
debug_libnanox_fpga_api_la_SOURCES=$(fpga_api_sources)

debug_libnanox_pe_fpga_la_CPPFLAGS=$(common_debug_CPPFLAGS) @xtasksinc@
debug_libnanox_pe_fpga_la_CXXFLAGS=$(common_debug_CXXFLAGS)
debug_libnanox_pe_fpga_la_LDFLAGS=$(AM_LDFLAGS) $(ld_plugin_flags)
debug_libnanox_pe_fpga_la_LIBADD=$(debug_fpga_core_LIBS)
debug_libnanox_pe_fpga_la_SOURCES=$(pe_fpga_sources)
endif

if is_instrumentation_debug_enabled
instrumentation_debug_LTLIBRARIES += instrumentation-debug/libnanox-fpga-core.la \
	instrumentation-debug/libnanox-fpga-api.la instrumentation-debug/libnanox-pe-fpga.la
instrumentation_debug_fpga_core_LIBS = instrumentation-debug/libnanox-fpga-core.la

instrumentation_debug_libnanox_fpga_core_la_CPPFLAGS=$(common_instrumentation_debug_CPPFLAGS) @xtasksinc@
instrumentation_debug_libnanox_fpga_core_la_CXXFLAGS=$(common_instrumentation_debug_CXXFLAGS)
instrumentation_debug_libnanox_fpga_core_la_LDFLAGS=$(AM_LDFLAGS) @xtaskslib@
instrumentation_debug_libnanox_fpga_core_la_LIBADD=@xtaskslibs@
instrumentation_debug_libnanox_fpga_core_la_SOURCES=$(fpga_sources)

instrumentation_debug_libnanox_fpga_api_la_CPPFLAGS=$(common_instrumentation_debug_CPPFLAGS) $(api_includes) @xtasksinc@
instrumentation_debug_libnanox_fpga_api_la_CXXFLAGS=$(common_instrumentation_debug_CXXFLAGS)
instrumentation_debug_libnanox_fpga_api_la_LDFLAGS=$(AM_LDFLAGS) $(version_flags)
instrumentation_debug_libnanox_fpga_api_la_LIBADD=$(instrumentation_debug_fpga_core_LIBS)
instrumentation_debug_libnanox_fpga_api_la_SOURCES=$(fpga_api_sources)

instrumentation_debug_libnanox_pe_fpga_la_CPPFLAGS=$(common_instrumentation_debug_CPPFLAGS) @xtasksinc@
instrumentation_debug_libnanox_pe_fpga_la_CXXFLAGS=$(common_instrumentation_debug_CXXFLAGS)
instrumentation_debug_libnanox_pe_fpga_la_LDFLAGS=$(AM_LDFLAGS) $(ld_plugin_flags)
instrumentation_debug_libnanox_pe_fpga_la_LIBADD=$(instrumentation_debug_fpga_core_LIBS)
instrumentation_debug_libnanox_pe_fpga_la_SOURCES=$(pe_fpga_sources)
endif

if is_instrumentation_enabled
instrumentation_LTLIBRARIES += instrumentation/libnanox-fpga-core.la instrumentation/libnanox-fpga-api.la instrumentation/libnanox-pe-fpga.la
instrumentation_fpga_core_LIBS = instrumentation/libnanox-fpga-core.la

instrumentation_libnanox_fpga_core_la_CPPFLAGS=$(common_instrumentation_CPPFLAGS) @xtasksinc@
instrumentation_libnanox_fpga_core_la_CXXFLAGS=$(common_instrumentation_CXXFLAGS)
instrumentation_libnanox_fpga_core_la_LDFLAGS=$(AM_LDFLAGS) @xtaskslib@
instrumentation_libnanox_fpga_core_la_LIBADD=@xtaskslibs@
instrumentation_libnanox_fpga_core_la_SOURCES=$(fpga_sources)

instrumentation_libnanox_fpga_api_la_CPPFLAGS=$(common_instrumentation_CPPFLAGS) $(api_includes) @xtasksinc@
instrumentation_libnanox_fpga_api_la_CXXFLAGS=$(common_instrumentation_CXXFLAGS)
instrumentation_libnanox_fpga_api_la_LDFLAGS=$(AM_LDFLAGS) $(version_flags)
instrumentation_libnanox_fpga_api_la_LIBADD=$(instrumentation_fpga_core_LIBS)
instrumentation_libnanox_fpga_api_la_SOURCES=$(fpga_api_sources)

instrumentation_libnanox_pe_fpga_la_CPPFLAGS=$(common_instrumentation_CPPFLAGS) @xtasksinc@
instrumentation_libnanox_pe_fpga_la_CXXFLAGS=$(common_instrumentation_CXXFLAGS)
instrumentation_libnanox_pe_fpga_la_LDFLAGS=$(AM_LDFLAGS) $(ld_plugin_flags)
instrumentation_libnanox_pe_fpga_la_LIBADD=$(instrumentation_fpga_core_LIBS)
instrumentation_libnanox_pe_fpga_la_SOURCES=$(pe_fpga_sources)
endif

if is_performance_enabled
performance_LTLIBRARIES += performance/libnanox-fpga-core.la performance/libnanox-fpga-api.la performance/libnanox-pe-fpga.la
performance_fpga_core_LIBS = performance/libnanox-fpga-core.la

performance_libnanox_fpga_core_la_CPPFLAGS=$(common_performance_CPPFLAGS) @xtasksinc@
performance_libnanox_fpga_core_la_CXXFLAGS=$(common_performance_CXXFLAGS)
performance_libnanox_fpga_core_la_LDFLAGS=$(AM_LDFLAGS) @xtaskslib@
performance_libnanox_fpga_core_la_LIBADD=@xtaskslibs@
performance_libnanox_fpga_core_la_SOURCES=$(fpga_sources)

performance_libnanox_fpga_api_la_CPPFLAGS=$(common_performance_CPPFLAGS) $(api_includes) @xtasksinc@
performance_libnanox_fpga_api_la_CXXFLAGS=$(common_performance_CXXFLAGS)
performance_libnanox_fpga_api_la_LDFLAGS=$(AM_LDFLAGS) $(version_flags)
performance_libnanox_fpga_api_la_LIBADD=$(performance_fpga_core_LIBS)
performance_libnanox_fpga_api_la_SOURCES=$(fpga_api_sources)

performance_libnanox_pe_fpga_la_CPPFLAGS=$(common_performance_CPPFLAGS) @xtasksinc@
performance_libnanox_pe_fpga_la_CXXFLAGS=$(common_performance_CXXFLAGS)
performance_libnanox_pe_fpga_la_LDFLAGS=$(AM_LDFLAGS) $(ld_plugin_flags)
performance_libnanox_pe_fpga_la_LIBADD=$(performance_fpga_core_LIBS)
performance_libnanox_pe_fpga_la_SOURCES=$(pe_fpga_sources)
endif
