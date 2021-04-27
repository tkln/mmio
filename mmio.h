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

template <typename IOImpl, typename BackType>
struct RegisterBase {
    using IO = IOImpl;
    using BackT = BackType;
};

template <
    typename RegBase,
    unsigned width,
    unsigned off,
    typename ValT
>
struct ModeField {
    using BackT = typename RegBase::BackT;
    static constexpr BackT mask = ((1U << width) - 1U) << off;
    static void set(uintptr_t addr, ValT v)
    {
        auto r = RegBase::IO::read(addr);
        r &= ~mask;
        r |= (static_cast<BackT>(v) << off);
        RegBase::IO::write(addr, r);
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
    typename RegBase,
    unsigned width,
    unsigned off,
    typename ValT
>
struct BitField {
    using BackT = typename RegBase::BackT;
    static constexpr BackT mask = ((1U << width) - 1U) << off;
    using T = ValT;

    template <typename... Bits>
    static void set(uintptr_t addr, Bits... v)
    {
        check_types<ValT>(v...);
        auto r = RegBase::IO::read(addr);
        r |= get_mask<BackT>(v...);
        RegBase::IO::write(addr, r);
    }

    template <typename... Bits>
    static void clear(uintptr_t addr, Bits... v)
    {
        check_types<ValT>(v...);
        auto r = RegBase::IO::read(addr);
        r &= ~get_mask<BackT>(v...);
        RegBase::IO::write(addr, r);
    }
};

template <typename... Fields>
struct RegisterImpl : Fields... {
};

template <
    typename IO,
    typename BaseT,
    auto addr,
    template <typename RegBase> typename... Fields
>
struct Register {
    using ImplIO = IO;
    using RegBase = RegisterBase<IO, BaseT>;
    using Impl = RegisterImpl<Fields<RegBase>...>;

    template <typename... Vals>
    static void set(Vals... v)
    {
        Impl::set(addr, v...);
    }

    template <typename... Vals>
    static void clear(Vals... v)
    {
        Impl::clear(addr, v...);
    }
};

};

#endif
