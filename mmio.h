#ifndef MMIO_H
#define MMIO_H

#include <cstdint>

namespace mmio {

template <typename T>
struct VolatileIO {
    template <typename AddrT>
    static inline volatile T *addr_cast(AddrT addr)
    {
        /*
         * Performing a static_cast before reinterpret_cast prevents
         * accidentally using objects that don't generally look like numbers as
         * memory addresses.
         */
        uintptr_t uintptr = static_cast<uintptr_t>(addr);
        return reinterpret_cast<volatile T *>(uintptr);
    }

    template <typename AddrT>
    static inline T read(AddrT addr)
    {
        return *addr_cast(addr);
    }

    template <typename AddrT>
    static inline void write(AddrT addr, T val)
    {
        *addr_cast(addr) = val;
    }
};

template <typename T>
struct ValIO {
    static inline T read(T *addr)
    {
        return *addr;
    }
    static inline void write(T *addr, T val)
    {
        *addr = val;
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

    template <typename AddrT>
    static inline void set(AddrT addr, ValT v)
    {
        auto r = RegBase::IO::read(addr);
        r &= ~mask;
        r |= (static_cast<BackT>(v) << off);
        RegBase::IO::write(addr, r);
    }

    template <typename AddrT>
    static inline ValT get(AddrT addr)
    {
        auto r = RegBase::IO::read(addr);
        r &= ~mask;
        return static_cast<ValT>(r >> off);
    }

    static void clear(void) = delete;
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

    template <typename AddrT, typename... Bits>
    static inline void set(AddrT addr, Bits... v)
    {
        check_types<ValT>(v...);
        auto r = RegBase::IO::read(addr);
        r |= get_mask<BackT>(v...);
        RegBase::IO::write(addr, r);
    }

    template <typename AddrT, typename... Bits>
    static inline void clear(AddrT addr, Bits... v)
    {
        check_types<ValT>(v...);
        auto r = RegBase::IO::read(addr);
        r &= ~get_mask<BackT>(v...);
        RegBase::IO::write(addr, r);
    }

    template <typename AddrT>
    static inline bool get(AddrT addr, ValT bit)
    {
        auto r = RegBase::IO::read(addr);
        return r &= get_mask<BackT>(bit);
    }
};

template <typename... Fields>
struct RegisterImpl : Fields... {
    using Fields::set...;
    using Fields::get...;
    using Fields::clear...;
};

template <typename RegBase, template <typename FRegBase> typename... Fields>
struct RegisterVal {
    using BackT = typename RegBase::BackT;
    using ImplRegBase = RegisterBase<ValIO<BackT>, BackT>;
    using Impl = RegisterImpl<Fields<ImplRegBase>...>;

    inline RegisterVal(uintptr_t addr) : addr(addr)
    {
        val = RegBase::IO::read(addr);
    }

    template <typename... Vals>
    [[nodiscard]]
    inline RegisterVal set(Vals... v)
    {
        Impl::set(&val, v...);
        return *this;
    }

    template <typename... Vals>
    [[nodiscard]]
    inline RegisterVal clear(Vals... v)
    {
        Impl::clear(&val, v...);
        return *this;
    }

    inline void write()
    {
        RegBase::IO::write(addr, val);
    }

    BackT val;
    const uintptr_t addr;
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
    static inline void set(Vals... v)
    {
        Impl::set(addr, v...);
    }

    template <typename... Vals>
    static inline void clear(Vals... v)
    {
        Impl::clear(addr, v...);
    }

    template <typename ValT>
    static inline auto get(ValT v)
    {
        return Impl::get(addr, v);
    }

    [[nodiscard]]
    static inline RegisterVal<RegBase, Fields...> read()
    {
        return RegisterVal<RegBase, Fields...>(addr);
    }
};

template <
    typename IO,
    typename BaseT,
    typename Addr,
    template <typename RegBase> typename... Fields
>
struct DynRegister {
    using ImplIO = IO;
    using RegBase = RegisterBase<IO, BaseT>;
    using Impl = RegisterImpl<Fields<RegBase>...>;

    DynRegister(Addr addr) : addr(addr) { }

    template <typename... Vals>
    inline void set(Vals... v)
    {
        Impl::set(addr, v...);
    }

    template <typename... Vals>
    inline void clear(Vals... v)
    {
        Impl::clear(addr, v...);
    }

    template <typename ValT>
    inline auto get(ValT v)
    {
        return Impl::get(addr, v);
    }

    [[nodiscard]]
    inline RegisterVal<RegBase, Fields...> read()
    {
        return RegisterVal<RegBase, Fields...>(addr);
    }

    Addr addr;
};

};

#endif
