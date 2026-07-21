############################################################
# cdms-rawskim Makefile
#
# Targets:
#   make skim_raw.exe        - MIDAS skimmer (needs CDMSIOLIB only)
#   make make_eventlist.exe  - RQ preselection (needs ROOT only)
#   make / make all          - both
#
# CDMSIOLIB location can be overridden:
#   make CDMSIOLIB=/path/to/IOLibrary
############################################################

CDMSIOLIB ?= /projects/standard/yanliusp/shared/analyses/Max/IOLibrary

CXX      := g++
CXXFLAGS := -g -O2 -Wall -std=c++17 -fPIC

ROOTCFLAGS := $(shell root-config --cflags 2>/dev/null)
ROOTLIBS   := $(shell root-config --libs 2>/dev/null)

# CDMSIOLIB headers pull in ROOT (TXMLNode.h), so both executables need
# a ROOT environment (cdmsfull singularity image, or module load root).
define require_root
	@if [ -z "$(ROOTCFLAGS)" ]; then \
	    echo "ERROR: root-config not found - load a ROOT environment"; \
	    echo "       (e.g. run inside a cdmsfull singularity image)"; \
	    exit 1; \
	fi
endef

all: skim_raw.exe make_eventlist.exe

skim_raw.exe: src/skim_raw.cxx
	$(require_root)
	$(CXX) $(CXXFLAGS) $(ROOTCFLAGS) -I$(CDMSIOLIB)/src $< \
	    -L$(CDMSIOLIB)/lib -lcdmsio -lz $(ROOTLIBS) -lXMLParser \
	    -Wl,-rpath,$(CDMSIOLIB)/lib -o $@

make_eventlist.exe: src/make_eventlist.cxx
	$(require_root)
	$(CXX) $(CXXFLAGS) $(ROOTCFLAGS) $< $(ROOTLIBS) -o $@

clean:
	rm -f skim_raw.exe make_eventlist.exe

.PHONY: all clean
