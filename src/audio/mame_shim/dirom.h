// Stand-in for MAME's device_rom_interface (see emu.h): the SCSP's own address space = the board's 512 KB sound RAM,
// big-endian bytes, byte addresses; unmapped (>= the RAM's size) reads 0. Our own code (MIT).
#pragma once

#include "emu.h"

template <int AddrWidth, int DataWidth, int AddrShift, endianness_t Endian>
class device_rom_interface {
public:
    device_rom_interface(const machine_config&, device_t&) {}
    virtual ~device_rom_interface() = default;
    void set_ram(u8* ram, u32 size) { space_.ram = ram; space_.size = size; }

    u8 read_byte(offs_t a) { return space_.read_byte(a & ((1u << AddrWidth) - 1)); }
    u16 read_word(offs_t a) { return space_.read_word(a & ((1u << AddrWidth) - 1)); }
    address_space& space(int = 0) { return space_; }

protected:
    virtual void rom_bank_pre_change() {}

private:
    address_space space_;
};
