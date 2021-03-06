/*
 * Copyright (c) 2021 Aapo Vienamo
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include <cstdio>
#include <string>

#include "mmio.h"

#define assert_eq(_a, _b)                                               \
    if ((_a) != (_b))                                                   \
        printf("FAIL(%d): %s != %s: (%s), (%s)\n", __LINE__, #_a, #_b,  \
               std::to_string(_a).c_str(), std::to_string(_b).c_str());

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

struct TestVarFieldValA: public mmio::VarFieldValue<uint8_t> {
    using mmio::VarFieldValue<uint8_t>::VarFieldValue;
};

struct TestVarFieldValB: public mmio::VarFieldValue<uint8_t> {
    using mmio::VarFieldValue<uint8_t>::VarFieldValue;
};

template <typename RegBase>
using TestVarFieldA = mmio::VarField<RegBase, RegBase::mask(8, 0), 5, TestVarFieldValA>;

template <typename RegBase>
using TestVarFieldB = mmio::VarField<RegBase, RegBase::mask(8, 0), 13, TestVarFieldValB>;

using TestReg = mmio::Register<DummyIO, uint32_t, 0x1,
      TestBitField,
      TestModeField,
      TestVarFieldA,
      TestVarFieldB
>;

void test_reg()
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

    TestReg::set(TestVarFieldValA(123));
    TestReg::set(TestVarFieldValB(250));
    assert_eq(TestReg::get<TestVarFieldValA>(), 123);
    assert_eq(TestReg::get<TestVarFieldValB>(), 250);
}

using TestDynReg = mmio::DynRegister<DummyIO, uint32_t, uintptr_t,
      TestBitField,
      TestModeField,
      TestVarFieldA
>;

void test_dyn_reg()
{
    TestDynReg reg(0);
}

int main()
{
    test_reg();
    test_dyn_reg();
    return 0;
}
