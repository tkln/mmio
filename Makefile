# Copyright (c) 2021 Aapo Vienamo
# SPDX-License-Identifier: LGPL-3.0-only

CXXFLAGS=-std=c++17
test: test.o
test.o: test.cc mmio.h
