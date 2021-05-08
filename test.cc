/*
 * Copyright (c) 2021 Aapo Vienamo
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include <cstdio>
#include <string>

#include "mmio.h"

static uint32_t io_buf[256];

struct DummyIO {
    static uint32_t read(uintptr_t addr)
    {
        return io_buf[addr];
    }
    static void write(uintptr_t addr, uint32_t val)
    {
        io_buf[addr] = val;
    }
};

enum class TestBits {
    Bit0,
    Bit1,
    Bit2,
    Bit3,
};

template <typename RegBase>
using TestBitField = mmio::BitField<RegBase, RegBase::mask(4, 0), TestBits>;

enum class TestModes {
    Mode0 = 0,
    Mode1 = 1,
};

template <typename RegBase>
using TestModeField = mmio::ModeField<RegBase, RegBase::mask(1, 0), 4, TestModes>;

template <typename RegBase>
using TestValueField = mmio::ValueField<RegBase, RegBase::mask(8, 0), 5, uint8_t>;

using TestReg = mmio::Register<DummyIO, uint32_t, 0x1,
      TestBitField,
      TestModeField,
      TestValueField
>;

#define assert_eq(_a, _b)                                               \
    if ((_a) != (_b))                                                   \
        printf("FAIL(%d): %s != %s: (%s), (%s)\n", __LINE__, #_a, #_b,  \
               std::to_string(_a).c_str(), std::to_string(_b).c_str());

int main()
{
    TestReg::set(TestBits::Bit0, TestBits::Bit1);
    assert_eq(io_buf[1], 0x03);
    assert_eq(io_buf[0], 0x00);

    TestReg::set(TestModes::Mode0);
    assert_eq(io_buf[1], 0x03);
    assert_eq(io_buf[0], 0x00);

    TestReg::set(TestModes::Mode1);
    assert_eq(io_buf[1], 0x13);

    TestReg::clear(TestBits::Bit0);
    assert_eq(io_buf[1], 0x12);

    TestReg::read().set(TestBits::Bit2).write();
    assert_eq(io_buf[1], 0x16);

    assert_eq(TestReg::get(TestBits::Bit0), false);
    assert_eq(TestReg::get(TestBits::Bit1), true);

    TestReg::set<uint8_t>(123);
    assert_eq(TestReg::get<uint8_t>(), 123);
}
