############################################################
# minidas Makefile
#
# Builds the single `minidas` executable (subcommands:
# eventlist, skim). Build inside a cdmsfull singularity image,
# which ships both ROOT and CDMSIOLIB.
#
# CDMSIOLIB location can be overridden:
#   make CDMSIOLIB=/path/to/IOLibrary
############################################################

# Prefer the CDMSIOLIB shipped inside the cdmsfull singularity image
# (version-matched to the image's ROOT); fall back to the shared copy on
# MSI, which needs the matching ROOT loaded and /projects bound (-B).
ifneq ($(wildcard /opt/cdms/release/lib/libcdmsio.so),)
CDMSIOLIB ?= /opt/cdms/release
else
CDMSIOLIB ?= /projects/standard/yanliusp/shared/analyses/Max/IOLibrary
endif

CXX      := g++
CXXFLAGS := -g -O2 -Wall -std=c++17 -fPIC

ROOTCFLAGS := $(shell root-config --cflags 2>/dev/null)
ROOTLIBS   := $(shell root-config --libs 2>/dev/null)

SRCS := src/minidas.cxx src/eventlist.cxx src/skim.cxx

all: minidas

minidas: $(SRCS) src/commands.h
	@if [ -z "$(ROOTCFLAGS)" ]; then \
	    echo "ERROR: root-config not found - build inside a cdmsfull image"; \
	    exit 1; \
	fi
	$(CXX) $(CXXFLAGS) $(ROOTCFLAGS) -I$(CDMSIOLIB)/include -I$(CDMSIOLIB)/src $(SRCS) \
	    -L$(CDMSIOLIB)/lib -lcdmsio -lz $(ROOTLIBS) -lXMLParser \
	    -Wl,-rpath,$(CDMSIOLIB)/lib -o $@

clean:
	rm -f minidas skim_raw.exe make_eventlist.exe

.PHONY: all clean
