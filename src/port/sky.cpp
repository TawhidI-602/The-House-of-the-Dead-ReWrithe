// The sky dome (proofs-5): sky_task 0x33F50, first written in the runtime (src/game/chapter.cpp, s1), moved here for
// its capture proof. Arcade form only (the PC form is replayed). Declaration: decl/support_b.h.
#include "port/game.h"

namespace hotd::port {

// 0x33F50 sky_task (task): the sky dome around the camera. sky_index (0x51F77A, SKYTB's operand) picks a row of table
// 0xAC1F0 (16 bytes: model, height, turn speed, start angle; a new index restarts the angle +0x30); sky_mode
// (0x51F77B) 0 = no sky, 2 = not turning. Two models with the camera's rotation and no translation (coprocessor 0x88),
// lifted by the row's height (0x12): the dome, turned about y by the task's angle (0x15), then the cloud layer
// PN_skyuv02a (0x608). The geometrizer words written after (0x800030 / 0x804000: the viewport) are hardware, not
// work RAM (the runtime's renderer does them)
template <class P> void Game<P>::sky_task(uint32_t task)
{
    if constexpr (P::pc) {
        (void)task;   // the PC form: not ported (vtable only; ProofPc replays it)
    } else {
        const uint32_t index = mem.template at<uint8_t>(G::sky_index);
        const uint32_t row = 0xAC1F0 + index * 16;
        if (index != mem.template at<uint8_t>(0x53E384))
            mem.set32(task + 0x30, mem.u32(row + 12));
        const uint8_t mode = mem.template at<uint8_t>(G::sky_mode);
        if (mode != 0) {
            Tgp::Out none;
            const uint32_t zero[3] = {0, 0, 0}, lift[3] = {0, mem.u32(row + 4), 0};
            copro(0x05, nullptr, 0, none);   // 0x880050: push
            copro(0x88, zero, 3, none);
            copro(0x12, lift, 3, none);
            uint32_t angle = mem.u32(task + 0x30);
            if (mode != 2)
                angle += mem.u32(row + 8);
            mem.set32(task + 0x30, angle);
            const uint32_t a[1] = {angle};
            copro(0x15, a, 1, none);
            geo_draw_model(mem.u32(row));
            copro(0x06, nullptr, 0, none);   // pop
            copro(0x05, nullptr, 0, none);
            copro(0x88, zero, 3, none);
            copro(0x12, lift, 3, none);
            geo_draw_model(0x608);
            copro(0x06, nullptr, 0, none);
        }
        mem.template at<uint8_t>(0x53E384) = uint8_t(index);
    }
}

} // namespace hotd::port
