CXX      ?= g++
CXXOPT   ?= -g
CXXSTD   ?= c++23
CXXDEFS  ?=
CXXFLAGS ?= -Wall $(CXXOPT) -std=$(CXXSTD) -I. $(CXXDEFS)
OBJDIR   ?= obj

TERM ?= dumb  # Prevent color output in emacs within docker

# Remember compiler and flags from previous run. If they change, regenerate
# the cxx_config file, thus forcing a rebuild.
CXX_CONFIG_FILE = $(OBJDIR)/cxx_config.txt
OLD_CXX_CONFIG = $(shell cat $(CXX_CONFIG_FILE) 2> /dev/null)
NEW_CXX_CONFIG = $(CXX) $(CXXFLAGS)
ifneq ($(OLD_CXX_CONFIG),$(NEW_CXX_CONFIG))
	REBUILD_CONFIG = .FORCE   # Build flags have changed; force rebuild.
else
	REBUILD_CONFIG =          # Build flags have not changed.
endif

$(CXX_CONFIG_FILE) : $(REBUILD_CONFIG)
	mkdir -p $(OBJDIR)
	echo "$(NEW_CXX_CONFIG)" > $@

%.test : %.t
	$(OBJDIR)/$*.t $(TEST_ARGS)

%.t : %.t.cpp *.h $(CXX_CONFIG_FILE)
	$(CXX) $(CXXFLAGS) -o $(OBJDIR)/$@ $<

.FORCE:

.PRECIOUS: %.t %.html %.pdf
