/*
 * Copyright (c) 2021 Aapo Vienamo
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#ifndef MMIO_H
#define MMIO_H

#include <stdint.h>

namespace mmio {

template <typename T>
struct VolatileIO {
    using ValT = T;

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

template <typename IO, uintptr_t base_addr, uintptr_t stride, uintptr_t offset>
struct ArrayIO {
    static constexpr uintptr_t get_addr(uintptr_t idx)
    {
        return base_addr + offset + stride * idx;
    }

    template <typename AddrT>
    static inline typename IO::ValT read(AddrT idx)
    {
        return *IO::addr_cast(get_addr(idx));
    }

    template <typename AddrT>
    static inline void write(AddrT idx, typename IO::ValT val)
    {
        *IO::addr_cast(get_addr(idx)) = val;
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

    static constexpr BackT mask(unsigned width, unsigned offset)
    {
        return ((1U << width) - 1U) << offset;
    }

    static constexpr BackT mask(unsigned mask)
    {
        return mask;
    }
};

template <
    typename RegBase,
    typename RegBase::BackT mask,
    unsigned offset,
    typename ValT
>
struct ModeField {
    using BackT = typename RegBase::BackT;
    static constexpr BackT shifted_mask = mask << offset;

    template <typename AddrT>
    static inline void set(AddrT addr, ValT v)
    {
        auto r = RegBase::IO::read(addr);
        r &= ~shifted_mask;
        r |= (static_cast<BackT>(v) << offset);
        RegBase::IO::write(addr, r);
    }

    template <typename AddrT>
    static inline void get(AddrT addr, ValT &v)
    {
        auto r = RegBase::IO::read(addr);
        r &= shifted_mask;
        v = static_cast<ValT>(r >> offset);
    }

    static void clear(void) = delete;
};

template <
    typename RegBase,
    typename RegBase::BackT mask,
    unsigned offset,
    typename ValT
>
struct ValueField {
    using BackT = typename RegBase::BackT;
    static constexpr BackT shifted_mask = mask << offset;

    template <typename AddrT>
    static inline void set(AddrT addr, ValT v)
    {
        auto r = RegBase::IO::read(addr);
        r &= ~shifted_mask;
        r |= (static_cast<BackT>(v) << offset);
        RegBase::IO::write(addr, r);
    }

    template <typename AddrT>
    static inline void get(AddrT addr, ValT &v)
    {
        auto r = RegBase::IO::read(addr);
        r &= shifted_mask;
        v = static_cast<ValT>(r >> offset);
    }

    template <typename AddrT>
    static void clear(AddrT addr)
    {
        auto r = RegBase::IO::read(addr);
        r &= ~shifted_mask;
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
    typename RegBase::BackT mask,
    typename ValT
>
struct BitField {
    using BackT = typename RegBase::BackT;
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
    static inline void set_wo(AddrT addr, Bits... v)
    {
        auto r = get_mask<BackT>(v...);
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

template <typename RegBase, template <typename FRegBase> typename... Fields>
struct RegisterVal;

template <typename RegBase, template <typename FRegBase> typename... Fields>
struct RegisterImpl : Fields<RegBase>... {
    using Fields<RegBase>::set...;
    using Fields<RegBase>::get...;
    using Fields<RegBase>::clear...;
    using ValT = RegisterVal<RegBase, Fields...>;
};

template <typename RegBase, template <typename FRegBase> typename... Fields>
struct RegisterVal {
    using BackT = typename RegBase::BackT;
    using ImplRegBase = RegisterBase<ValIO<BackT>, BackT>;
    using Impl = RegisterImpl<ImplRegBase, Fields...>;

    template <typename T>
    inline RegisterVal(T addr) : addr(static_cast<uintptr_t>(addr))
    {
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

    [[nodiscard]]
    inline RegisterVal clear()
    {
        val = 0;
        return *this;
    }

    template <typename ValT>
    inline RegisterVal get(ValT v) const
    {
        return Impl::get(&val, v);
    }

    [[nodiscard]]
    inline RegisterVal read()
    {
        val = RegBase::IO::read(addr);
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
    using RegBase = RegisterBase<IO, BaseT>;
    using Impl = RegisterImpl<RegBase, Fields...>;

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

    static inline void clear()
    {
        RegBase::IO::write(addr, 0);
    }

    template <typename ValT>
    static inline auto get(ValT v)
    {
        return Impl::get(addr, v);
    }

    template <typename ValT>
    static inline auto get()
    {
        ValT v;
        Impl::get(addr, v);
        return v;
    }

    [[nodiscard]]
    static inline typename Impl::ValT read()
    {
        return typename Impl::ValT(addr).read();
    }

    [[nodiscard]]
    static inline typename Impl::ValT new_val()
    {
        return typename Impl::ValT(addr);
    }
};

template <
    typename IO,
    typename BaseT,
    auto addr,
    template <typename RegBase> typename... Fields
>
struct RegisterRO : Register<IO, BaseT, addr, Fields...> {
    using Reg = Register<IO, BaseT, addr, Fields...>;
    using Reg::get;
    using Impl = typename Register<IO, BaseT, addr, Fields...>::Impl;

    static inline const typename Impl::ValT read()
    {
        return typename Impl::ValT(addr);
    }

    template <typename... Vals>
    static inline void set(Vals... v) = delete;

    template <typename... Vals>
    static inline void clear(Vals... v) = delete;
};

template <
    typename IO,
    typename BaseT,
    auto addr,
    template <typename RegBase> typename... Fields
>
struct RegisterWO : Register<IO, BaseT, addr, Fields...> {
    using Reg = Register<IO, BaseT, addr, Fields...>;
    using Impl = typename Register<IO, BaseT, addr, Fields...>::Impl;

    template <typename... Vals>
    static inline void set(Vals... v)
    {
        Impl::set_wo(addr, v...);
    }

    template <typename... Vals>
    static inline void clear(Vals... v) = delete;

    template <typename ValT>
    static inline auto get(ValT v) = delete;

    /* TODO Bit masks and write() */
    static inline const typename Impl::ValT read() = delete;
};

template <
    typename IO,
    typename BaseT,
    typename Addr,
    template <typename RegBase> typename... Fields
>
struct DynRegister {
    using RegBase = RegisterBase<IO, BaseT>;
    using Impl = RegisterImpl<RegBase, Fields...>;

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
    inline typename Impl::ValT read()
    {
        return typename Impl::ValT(addr);
    }

    Addr addr;
};

};

#endif
