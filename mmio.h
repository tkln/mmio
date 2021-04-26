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

template <
    typename IO,
    typename BackT,
    uintptr_t addr,
    unsigned width,
    unsigned off,
    typename ValT
>
struct ModeField {
    static constexpr BackT mask = ((1U << width) - 1U) << off;
    static void set(ValT v)
    {
        auto r = IO::read(addr);
        r &= ~mask;
        r |= (static_cast<BackT>(v) << off);
        IO::write(addr, r);
    }
};

};

#endif
