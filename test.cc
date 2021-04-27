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

int main()
{
}
