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

enum class TestModes {
    Mode0 = 0,
    Mode1 = 1,
};

template <typename RegBase>
using TestBitField = mmio::BitField<RegBase, 2, 0, TestBits>;

template <typename RegBase>
using TestModeField = mmio::ModeField<RegBase, 1, 2, TestModes>;

using TestReg = mmio::Register<DummyIO, uint32_t, 0x1,
      TestBitField,
      TestModeField
  >;

int main()
{
    TestReg::set(TestBits::Bit0, TestBits::Bit1);
    TestReg::set(TestModes::Mode0);
    TestReg::clear(TestBits::Bit0);
    TestReg::read().set(TestBits::Bit2).write();
    TestReg::get(TestBits::Bit0);
}
