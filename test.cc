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


using TestBitField = mmio::BitField<DummyIO, uint32_t, 0x0, 2, 0, TestBits>;

int main()
{
    TestBitField::set(TestBits::Bit0, TestBits::Bit1);
}
