CXXFLAGS=-std=c++17
test: test.o
test.o: test.cc mmio.h
