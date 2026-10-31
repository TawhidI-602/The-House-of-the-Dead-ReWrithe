// Enemies, A behaviour: enemy_guard, the set-up of a standard enemy from its spawn record's action code, read from the
// disassembly (Ghidra's C drops the +0x348 and spawn-slot word stores).
#include "port/game.h"

namespace hotd::port {

// the enemy's prey: captor links both ways; none: the developers' "To search a target failed..." and a hang
template <class P> void Game<P>::guard_link_prey(uint32_t obj)
{
    uint32_t t = mem.u32(G::prey_objects + mem.u32(obj + ob(0x320)) * 4);
    if (t == 0) {
        if constexpr (P::pc) {   // PC 0x4A50EC pc_guard_link_prey: a message, no halt
            pc_debug_print();
            return;
        }
        text_cursor(10, 10);
        text_printf(0x55040);
        debug_halt();
        return;
    }
    mem.set32(obj + ob(0x390), t);
    mem.set32(mem.u32(G::prey_objects + mem.u32(obj + ob(0x320)) * 4) + ob(0x390), obj);
}

// 0x550A0 (enemy): body and creature type, spawn slot flags cleared, counted, enemy slot; then by the action code (1-20)
// the first routine, motion and links; other actions (0): the standard enemy (enemy_main, state 1).
template <class P> void Game<P>::enemy_guard(uint32_t obj)
{
    auto E = [&]() -> Enemy& { return mem.at<Enemy>(obj); };
    const int32_t type = E().type;
    const int32_t action = E().action;
    mem.set32(G::hit_type, uint32_t(type));
    body_init(obj, type, 0x206);
    if (P::pc && int32_t(mem.u32(G::hit_type)) == 16)
        pc_debug_print();
    mem.set32(obj + ob(0x348), 0);
    auto slot = [&]() { return G::spawn_slots + uint32_t(int32_t(mem.at<int8_t>(obj + ob(0x2E6)))) * 0x2C; };
    mem.at<uint8_t>(slot() + 0x28) &= 0xFE;
    mem.set32(slot() + 4, 0);
    mem.set32(slot() + 0x1C, 0);
    mem.at<uint8_t>(slot() + 0x28) &= 0xFD;
    mem.at<uint8_t>(slot() + 0x28) &= 0xFB;
    creature_type_setup(obj);
    if (mem.at<uint8_t>(G::chapter) == 1 && mem.at<uint8_t>(G::event_index) == 12)
        E().flags2 &= 0xFFFD;
    mem.at<uint16_t>(G::enemy_count) = uint16_t(mem.at<uint16_t>(G::enemy_count) + 1);
    mem.at<uint16_t>(G::enemies_alive) = uint16_t(mem.at<uint16_t>(G::enemies_alive) + 1);
    enemy_slot_take(obj);
    auto route = [&](uint32_t r) { E().routine = r; };
    switch (uint32_t(action - 1) <= 19 ? action : 0) {
    case 1: {   // scenery: motion number from the record's life field, the guard delay
        int32_t m = E().life;
        int32_t d = mem.at<int8_t>(G::scene_number);
        route(F::enemy_act1_scenery);
        E().delay = d;
        set_motion(obj, uint32_t(m));
        return;
    }
    case 2:
        guard_link_prey(obj);
        route(F::enemy_act2_approach_target);
        if (uint16_t(E().type) == 0x49)
            set_motion(obj, 0x289);
        else if (uint16_t(E().type) == 3)
            set_motion(obj, 0x22F);
        else
            set_motion(obj, 0x284);
        return;
    case 3:
        E().timer = 0xB4;
        route(F::enemy_act3_start_variant1);
        mem.at<uint8_t>(slot() + 0x28) |= 1;
        set_motion(obj, mem.u32((G::enemy_guard_motions + 4)));
        return;
    case 4:
        route(F::enemy_act4_inert);
        return;
    case 5:
        route(F::enemy_act5_wait_stop);
        set_motion(obj, mem.u32((G::enemy_guard_motions + 8)));
        return;
    case 6:
        guard_link_prey(obj);
        route(F::enemy_act6_attack_researcher);
        set_motion(obj, 0x288);
        E().motion_flags |= 1;
        return;
    case 7:
        guard_link_prey(obj);
        route(F::enemy_act7_chainsaw_attack);
        E().substate = 0;
        set_motion_blend(obj, 0x228, 60, 1);
        return;
    case 8:
        guard_link_prey(obj);
        E().flags2 ^= 1;
        route(F::ENGuardOpeningDummy);
        set_motion(obj, mem.u32(G::enemy_guard_motions));
        return;
    case 9:
        guard_link_prey(obj);
        route(F::ENGuardZombieRunAfterResearcher);
        set_motion(obj, 0x284);
        return;
    case 10:
        guard_link_prey(obj);
        route(F::ENGuardZombieRunAfterResearcher);
        mem.at<uint8_t>(slot() + 0x28) |= 1;
        mem.set32(obj + ob(0x348), 0x3E99999A);   // 0.3f
        set_motion(obj, 0x150);
        return;
    case 11:
        guard_link_prey(obj);
        route(F::ENGuardZombieRunAfterResearcher);
        mem.at<uint8_t>(slot() + 0x28) |= 1;
        set_motion(obj, 0x284);
        return;
    case 12:
        guard_link_prey(obj);
        route(F::enemy_act12_walk_to_spot);
        E().motion_flags |= 2;
        set_motion(obj, 0x217);
        return;
    case 13:
        guard_link_prey(obj);
        route(F::ENGuardZombieRunAfterResearcher2);
        set_motion(obj, 0x284);
        return;
    case 14:
        guard_link_prey(obj);
        route(F::enemy_act14_attack_researcher);
        E().substate = 0;
        set_motion(obj, 0x248);
        return;
    case 15: {  // the elevator ride: a child task (elevator_ride) at (171.75, -280, 394) linked to both
        guard_link_prey(obj);
        uint32_t child = task_create_child(F::elevator_ride, uint32_t(sizeof(Obj)));
        task_clear_work(child);
        mem.set32(child + ob(0x2C), 0);
        mem.set32(child + ob(0x30), 0);
        mem.set32(child + ob(0x34), 0);
        mem.set32(child + ob(0x20), 0x442BC000);
        mem.set32(child + ob(0x24), 0xC38C0000);
        mem.set32(child + ob(0x28), 0x43C50000);
        uint32_t captor = mem.u32(obj + ob(0x390));
        mem.set32(obj + ob(0x18), child);
        mem.set32(captor + ob(0x18), child);
        route(F::enemy_act15_attack_researcher);
        E().substate = 0;
        E().draw_flags |= 0x40;
        set_motion(obj, 0x255);
        return;
    }
    case 16:
        guard_link_prey(obj);
        route(F::ENGuardZombieRunAfterResearcher_St3_00);
        set_motion(obj, 0x284);
        return;
    case 17:
        guard_link_prey(obj);
        route(F::enemy_act17_attack_researcher);
        E().substate = 0;
        set_motion_blend(obj, 0x255, 0x6E, 2);
        return;
    case 18:
        guard_link_prey(obj);
        route(F::enemy_act17_attack_researcher);
        E().substate = 0;
        set_motion_blend(obj, 0x255, 0, 30);
        return;
    case 19: {  // the standard enemy, walking at once
        uint16_t f2 = E().flags2;
        route(F::enemy_main);
        E().state = 1;
        E().substate = 2;
        E().action = 0;
        E().walk_variant = 1;
        E().flags2 = uint16_t(f2 & ~2u);
        uint32_t row = mem.u32(G::enemy_motion_lists + uint32_t(int32_t(E().type)) * 4);
        uint32_t list = mem.u32(row + uint32_t(int32_t(E().action)) * 4);
        set_motion_blend(obj, mem.u32(list + uint32_t(int32_t(E().substate)) * 8), 0, 8);
        enemy_target_distance(obj);
        return;
    }
    case 20: {  // goes for the first object of type 2 among 0x51F440[0..13]
        int32_t i = 0;
        for (; i < 14; ++i) {
            uint32_t o = mem.u32(G::player_bodies + uint32_t(i) * 4);
            if (mem.at<uint16_t>(o + ob(0xCC)) == 2) {
                mem.set32(obj + ob(0x390), o);
                mem.set32(mem.u32(G::player_bodies + uint32_t(i) * 4) + ob(0x390), obj);
                break;
            }
        }
        if (i == 14) {
            if constexpr (P::pc) {
                pc_debug_print();   // and goes on
            } else {
                text_cursor(10, 10);
                text_printf(0x55040);
                debug_halt();
                return;
            }
        }
        int32_t d = mem.at<int8_t>(G::scene_number);
        route(F::ZombieWalk2Tom);
        E().delay = d;
        set_motion(obj, 0x284);
        return;
    }
    default: {  // the standard enemy (type 29 takes action 1)
        route(F::enemy_main);
        E().action = uint16_t(E().type) == 29 ? 1 : 0;
        E().state = 1;
        E().substate = 1;
        uint32_t row = mem.u32(G::enemy_motion_lists + uint32_t(int32_t(E().type)) * 4);
        uint32_t list = mem.u32(row + uint32_t(int32_t(E().action)) * 4);
        set_motion_blend(obj, mem.u32(list + 8), 0, 8);
        enemy_target_distance(obj);
        return;
    }
    }
}


} // namespace hotd::port
