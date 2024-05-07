
CXX = g++
CXXFLAGS = -I. -std=c++03 -g

all : range_for_cpp03.test # unparen.t.i

%.test : %.t
	./$<

%.t: %.t.cpp range_for_cpp03.h
	$(CXX) $(CXXFLAGS) $< -o $@

%.i: %.cpp
	$(CXX) $(CXXFLAGS) -E $<
