
CXX = g++
CXXFLAGS = -I. -std=c++03
CXXOPT ?= -g

all : range_for_cpp03.test # unparen.t.i

%.test : %.t
	./$<

%.t: %.t.cpp range_for_cpp03.h
	$(CXX) $(CXXFLAGS) $(CXXOPT) $< -o $@

%.i: %.cpp
	$(CXX) $(CXXFLAGS) -E $<

# Generate asm file
%.s: %.cpp range_for_cpp03.h
	$(CXX) $(CXXFLAGS) $(CXXOPT) -S $< -o $@.raw
	c++filt < $@.raw > $@
	rm -f $@.raw
