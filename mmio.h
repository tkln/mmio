#ifndef MMIO_H
#define MMIO_H

#include <cstdint>

namespace mmio {

template <typename T>
struct VolatileIO {
    T read(uintptr_t addr)
    {
        return *(volatile T *)addr;
    }
    void write(uintptr_t addr, T val)
    {
        *(volatile T *)addr = val;
    }
};

template <typename T>
struct BitMask {
    T clear;
    T set;
};

};

#endif
