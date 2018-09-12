# The following make commands are available:
# - make, make all      Builds the executable test suite.
# - make archive        Archives all source files.
# - make check          Checks the semantics of TTCN-3 and ASN.1modules.
# - make port           Generates port skeletons.
# - make clean          Removes all generated files.
# - make compile        Translates TTCN-3 and ASN.1 modules to C++.
# - make dep            Creates/updates dependency list.
# - make executable     Builds the executable test suite.
# - make library        Builds the library archive.
# - make objects        Builds the object files without linking the executable.
# WARNING! This Makefile can be used with GNU make only.
# Other versions of make may report syntax errors in it.

#
# Do NOT touch this line...
#
.PHONY: all shared_objects executable library objects check port clean dep archive copy_cfg

.SUFFIXES: .d

#
# Set these variables...
#

# The path of your TTCN-3 Test Executor installation:
# Uncomment this line to override the environment variable.
# The value below points to the location of the TITAN version
# that generated this makefile.
ifndef TTCN3_DIR
TTCN3_DIR = /home/dark/titan.core/Install
endif

# Your platform: (SOLARIS, SOLARIS8, LINUX, FREEBSD or WIN32)
PLATFORM = LINUX

# Your C++ compiler:
# (if you change the platform, you may need to change the compiler)
CXX = g++ 

# Flags for the C++ preprocessor (and makedepend as well):
CPPFLAGS = -D$(PLATFORM) -I$(TTCN3_DIR)/include -I../src/ -I../src/TSPF/ -I.

# Flags for dependency generation
CXXDEPFLAGS = -MM

# Flags for the C++ compiler:
CXXFLAGS = -Wall -std=c++14

# Flags for the linker:
LDFLAGS = 

ifeq ($(PLATFORM), WIN32)
# Silence linker warnings.
LDFLAGS += -Wl,--enable-auto-import,--enable-runtime-pseudo-reloc
endif

# Utility to create library files
AR = ar
ARFLAGS = 

# Flags for the TTCN-3 and ASN.1 compiler:
COMPILER_FLAGS = -L 

# Execution mode: (either ttcn3 or ttcn3-parallel)
TTCN3_LIB = ttcn3-parallel

# The path of your libxml2 installation:
# If you do not have your own one, leave it unchanged.
XMLDIR = $(TTCN3_DIR)

# Directory to store the archived source files:
ARCHIVE_DIR = backup

#
# You may change these variables. Add your files if necessary...
#

# TTCN-3 modules of this project:
TTCN3_MODULES = ../src/TSP_Functions.ttcn ../src/TSP_PortType.ttcn ../src/TSP_Templates.ttcn ../src/TSP_Testcases.ttcn ../src/TSP_Types.ttcn ../src/TSPF/TSPF_snort_iperf3.ttcn ../src/TSPF/TSPF_snort_iperf3_test.ttcn ../src/TSPF/TSPF_empty_iperf3.ttcn ../src/TSPF/TSPF_haproxy_nginx_wrk.ttcn ../src/TSPF/TSPF_haproxy_nginxlb_nginx_wrk.ttcn ../src/TSPF/TSPF_haproxy_nginx_wrk_verification.ttcn ../src/TSPF/TSPF_haproxy_nginxlb_nginx_ab2.ttcn

# ASN.1 modules of this project:
ASN1_MODULES =

# C++ source & header files generated from the TTCN-3 & ASN.1 modules of
# this project:
GENERATED_SOURCES = TSP_Functions.cc TSP_PortType.cc TSP_Testcases.cc TSP_Types.cc TSP_Templates.cc TSPF_snort_iperf3.cc TSPF_snort_iperf3_test.cc TSPF_empty_iperf3.cc TSPF_haproxy_nginx_wrk.cc TSPF_haproxy_nginxlb_nginx_wrk.cc TSPF_haproxy_nginx_wrk_verification.cc TSPF_haproxy_nginxlb_nginx_ab2.cc $(ASN1_MODULES:.asn=.cc)
GENERATED_HEADERS = $(GENERATED_SOURCES:.cc=.hh)

# C/C++ Source & header files of Test Ports, external functions and
# other modules:
USER_SOURCES = ../src/ManoMsg.cc ../src/OutputParser.cc ../src/Reporter.cc
USER_HEADERS = $(USER_SOURCES:.cc=.hh)

# Object files of this project that are needed for the executable test suite:
OBJECTS = $(GENERATED_OBJECTS) $(USER_OBJECTS)

GENERATED_OBJECTS = $(GENERATED_SOURCES:.cc=.o)

USER_OBJECTS = $(USER_SOURCES:.cc=.o)

DEPFILES = $(USER_OBJECTS:.o=.d)  $(GENERATED_OBJECTS:.o=.d)

# Other files of the project (Makefile, configuration files, etc.)
# that will be added to the archived source files:
OTHER_FILES = ../src/config.cfg Makefile

# The name of the executable test suite:
EXECUTABLE = tsp-profiling 



LIBRARY = lib$(EXECUTABLE).a

TARGET = $(EXECUTABLE)

#
# Do not modify these unless you know what you are doing...
# Platform specific additional libraries:
#
SOLARIS_LIBS = -lsocket -lnsl -lxml2
SOLARIS8_LIBS = -lsocket -lnsl -lxml2
LINUX_LIBS = -lxml2 -lcpprest -lboost_system -lboost_filesystem -lpthread -lssl -lcrypto
FREEBSD_LIBS = -lxml2
WIN32_LIBS = -lxml2

#
# Rules for building the executable...
#

all: $(TARGET) copy_cfg;

executable: $(EXECUTABLE) ;

library: $(LIBRARY) ;

objects: $(OBJECTS) compile;

$(EXECUTABLE): $(OBJECTS)
	if $(CXX) $(LDFLAGS) -o $@ $^ \
	-L$(TTCN3_DIR)/lib -l$(TTCN3_LIB) \
	-L$(OPENSSL_DIR)/lib -lcrypto \
	-L$(XMLDIR)/lib $($(PLATFORM)_LIBS); \
	then : ; else $(TTCN3_DIR)/bin/titanver $(OBJECTS); exit 1; fi

$(LIBRARY): $(OBJECTS)
	$(AR) -r $(ARFLAGS) $(LIBRARY) $(OBJECTS)

.cc.o .c.o:
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) -o $@ $<

.cc.d .c.d:
	@echo Creating dependency file for '$<'; set -e; \
	$(CXX) $(CXXDEPFLAGS) $(CPPFLAGS) $(CXXFLAGS) $< \
	| sed 's/\($*\)\.o[ :]*/\1.o $@ : /g' > $@; \
	[ -s $@ ] || rm -f $@

$(GENERATED_SOURCES) $(GENERATED_HEADERS): compile
	@if [ ! -f $@ ]; then $(RM) compile; $(MAKE) compile; fi

check: $(TTCN3_MODULES) $(ASN1_MODULES)
	$(TTCN3_DIR)/bin/compiler -s $(COMPILER_FLAGS) $^

port: $(TTCN3_MODULES)  $(ASN1_MODULES)
	$(TTCN3_DIR)/bin/compiler -t $(COMPILER_FLAGS) $^

compile: $(TTCN3_MODULES)  $(ASN1_MODULES)
	$(TTCN3_DIR)/bin/compiler $(COMPILER_FLAGS) $^ - $?
	touch $@

clean:
	-$(RM) $(EXECUTABLE) $(LIBRARY) $(OBJECTS) $(GENERATED_HEADERS) \
	$(GENERATED_SOURCES) compile $(DEPFILES) config.cfg \
	tags *.log

dep: $(GENERATED_SOURCES) $(USER_SOURCES) ; 

ifeq ($(findstring n,$(MAKEFLAGS)),)
ifeq ($(filter clean check port compile archive diag,$(MAKECMDGOALS)),)
-include $(DEPFILES)
endif
endif

archive:
	mkdir -p $(ARCHIVE_DIR)
	tar -cvhf - $(TTCN3_MODULES) $(ASN1_MODULES) \
	$(USER_HEADERS) $(USER_SOURCES) $(OTHER_FILES) \
	| gzip >$(ARCHIVE_DIR)/`basename $(TARGET) .exe`-`date '+%y%m%d-%H%M'`.tgz

diag:
	$(TTCN3_DIR)/bin/compiler -v 2>&1
	$(TTCN3_DIR)/bin/mctr_cli -v 2>&1
	$(CXX) -v 2>&1
	$(AR) -V 2>&1
	@echo TTCN3_DIR=$(TTCN3_DIR)
	@echo OPENSSL_DIR=$(OPENSSL_DIR)
	@echo XMLDIR=$(XMLDIR)
	@echo PLATFORM=$(PLATFORM)

#
# Add your rules here if necessary...
#
copy_cfg:
	cp ../src/config.cfg .
