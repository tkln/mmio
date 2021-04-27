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

template <typename M, typename B>
static constexpr M get_mask(B bit)
{
    return 1U << static_cast<M>(bit);
}

template <typename M, typename B, typename... Bits>
static constexpr M get_mask(B bit, Bits... bits)
{
    return get_mask<M>(bit) | get_mask<M>(bits...);
}

template <typename T>
static constexpr void check_types(){}

template <typename T, typename... Ts>
static constexpr void check_types(T v, Ts... vs)
{
    check_types<T>(vs...);
}

template <
    typename IO,
    typename BackT,
    uintptr_t addr,
    unsigned width,
    unsigned off,
    typename ValT
>
struct BitField {
    static constexpr BackT mask = ((1U << width) - 1U) << off;
    using T = ValT;

    template <typename... Bits>
    static void set(Bits... v)
    {
        check_types<ValT>(v...);
        auto r = IO::read(addr);
        r |= get_mask<BackT>(v...);
        IO::write(addr, r);
    }

    template <typename... Bits>
    static void clear(Bits... v)
    {
        check_types<ValT>(v...);
        auto r = IO::read(addr);
        r &= ~get_mask<BackT>(v...);
        IO::write(addr, r);
    }
};

};

#endif
