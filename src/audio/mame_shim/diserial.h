// Stand-in for MAME's device_serial_interface (see emu.h), byte level: the board hands the SCSP's MIDI input whole
// bytes (receive_byte, at the MIDI byte rate) and finishes each MIDI output byte one service() later (nothing listens:
// HOTD's main CPU side is our game code). Our own code (MIT).
#pragma once

#include "emu.h"

enum { PARITY_NONE = 0 };
enum { STOP_BITS_1 = 1 };

class device_serial_interface {
public:
    device_serial_interface(const machine_config&, device_t&) {}
    virtual ~device_serial_interface() = default;

    // the board: one received byte (the game's command bytes)
    void receive_byte(u8 b)
    {
        rx_ = b;
        rcv_complete();
    }
    // the board: once per sample; a byte being sent is done
    void service()
    {
        if (tx_busy_) {
            tx_busy_ = false;
            tra_complete();
        }
    }
    void rx_w(int) {}   // bit level (MAME's midi_in): unused here

protected:
    void set_data_frame(int, int, int, int) {}
    void set_rate(int) {}
    void transmit_register_setup(u8 b) { tx_ = b; tx_busy_ = true; }
    int transmit_register_get_data_bit() { return 1; }
    void receive_register_extract() {}
    u8 get_received_char() const { return rx_; }

    virtual void tra_callback() {}
    virtual void tra_complete() {}
    virtual void rcv_complete() {}

private:
    u8 rx_ = 0, tx_ = 0;
    bool tx_busy_ = false;
};
