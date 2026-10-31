// The body's part tree: severed parts detached and freed, models set by role / joint, a joint's model looked up. First
// written as runtime overrides (T1, src/game/t1_bounds.cpp), read from the disassembly. Arcade forms
// (decl/support_b.h).
#include "port/game.h"

namespace hotd::port {

namespace {
constexpr uint32_t kDetachedOut = 0x53E3A8;   // the next slot of the detached list (arcade)
constexpr uint32_t kDetached = 0x520AA0;      // the detached nodes, 0-terminated (arcade)
constexpr uint32_t kNodeModel = 0x5434A0;     // body_node_model's result (arcade)
constexpr uint32_t kSetJoint = 0x5434A4, kSetModel = 0x5434A8;   // body_part_set_model's joint / model (arcade)
} // namespace

// 0x39440 part_tree_detach_severed_rec(node, parent, index): a node its parent marks severed (parent +0x28 + the
// node's index +0x30, bit 0) leaves the parent's child list (the rest shift down, their indices - 1, the caller's loop
// index (a word at `index`) - 1; the list is freed when it empties) and goes to the detached list; else its children
// in turn
template <class P> void Game<P>::part_tree_detach_severed_rec(uint32_t node, uint32_t parent, uint32_t index)
{
    if (!(mem.at<uint8_t>(parent + 0x28 + mem.u32(node + 0x30)) & 1)) {
        LocalScope scope(*this);
        const uint32_t i = local_alloc(4);
        mem.set32(i, 0);
        while (int32_t(mem.u32(i)) < int32_t(mem.u32(node + 0x34))) {
            part_tree_detach_severed_rec(mem.u32(mem.u32(node + 0x24) + 4 * mem.u32(i)), node, i);
            mem.set32(i, mem.u32(i) + 1);
        }
        return;
    }
    const int32_t n = int32_t(mem.u32(parent + 0x34)) - 1;
    mem.set32(parent + 0x34, uint32_t(n));
    if (n != 0) {
        for (int32_t j = int32_t(mem.u32(index)); j < int32_t(mem.u32(parent + 0x34)); ++j) {
            mem.at<uint8_t>(parent + 0x28 + uint32_t(j)) = mem.at<uint8_t>(parent + 0x29 + uint32_t(j));
            const uint32_t list = mem.u32(parent + 0x24);
            mem.set32(list + 4 * uint32_t(j), mem.u32(list + 4 * uint32_t(j) + 4));
            const uint32_t c = mem.u32(mem.u32(parent + 0x24) + 4 * uint32_t(j));
            mem.set32(c + 0x30, mem.u32(c + 0x30) - 1);
        }
        const uint32_t out = mem.u32(kDetachedOut);
        mem.set32(kDetachedOut, out + 4);
        mem.set32(index, mem.u32(index) - 1);
        mem.set32(out, node);
    } else {
        mem_free(mem.u32(parent + 0x24));
        mem.set32(parent + 0x24, 0);
        const uint32_t out = mem.u32(kDetachedOut);
        mem.set32(kDetachedOut, out + 4);
        mem.set32(out, node);
    }
}

// 0x39550 part_tree_detach_severed(root): every severed node of the tree into 0x520AA0 (0-terminated); the first level
// inline (as part_tree_detach_severed_rec)
template <class P> void Game<P>::part_tree_detach_severed(uint32_t root)
{
    mem.set32(kDetachedOut, kDetached);
    LocalScope scope(*this);
    const uint32_t i = local_alloc(4);
    mem.set32(i, 0);
    while (int32_t(mem.u32(i)) < int32_t(mem.u32(root + 0x34))) {
        part_tree_detach_severed_rec(mem.u32(mem.u32(root + 0x24) + 4 * mem.u32(i)), root, i);
        mem.set32(i, mem.u32(i) + 1);
    }
    const uint32_t out = mem.u32(kDetachedOut);
    mem.set32(kDetachedOut, out + 4);
    mem.set32(out, 0);
}

// 0x396C0 body_free_cut_parts(root): the severed nodes detached and freed with their subtrees (the limb is gone from the
// body)
template <class P> void Game<P>::body_free_cut_parts(uint32_t root)
{
    if constexpr (P::pc) {
        (void)root;   // the PC form: not ported (vtable only; ProofPc replays it)
    } else {
        part_tree_detach_severed(root);
        for (uint32_t p = kDetached; mem.u32(p) != 0; p += 4) {
            const uint32_t node = mem.u32(p);
            for (int32_t i = 0; i < int32_t(mem.u32(node + 0x34)); ++i) {
                const uint32_t c = mem.u32(mem.u32(node + 0x24) + 4 * uint32_t(i));
                const uint32_t cl = mem.u32(c + 0x24);
                for (int32_t j = 0; j < int32_t(mem.u32(c + 0x34)); ++j) {
                    part_tree_free_children(mem.u32(cl + 4 * uint32_t(j)));
                    part_node_free(mem.u32(cl + 4 * uint32_t(j)));
                }
                if (mem.u32(c + 0x24) != 0)
                    mem_free(mem.u32(c + 0x24));
                part_node_free(c);
            }
            if (mem.u32(node + 0x24) != 0)
                mem_free(mem.u32(node + 0x24));
            part_node_free(node);
        }
    }
}

// 0x1D8A0 body_role_model_set_rec(part, role, model): the part draws `model` when its role class (0x1184E3 + (joint +
// type x 31) x 8, the type of the part's body +0x40) is `role`; then its children
template <class P> void Game<P>::body_role_model_set_rec(uint32_t part, int role, uint32_t model)
{
    const int32_t type = mem.at<int16_t>(mem.u32(part + 0x40) + 0xCC);
    const uint32_t list = mem.u32(part + 0x24);
    if (int32_t(mem.at<int8_t>(0x1184E3 + (mem.u32(part + 0x38) + uint32_t(type * 31)) * 8)) == role)
        mem.set32(part, model);
    for (int32_t i = 0; i < int32_t(mem.u32(part + 0x34)); ++i)
        body_role_model_set_rec(mem.u32(list + 4 * uint32_t(i)), role, model);
}

// 0x1D910 body_role_model_set(obj, role, model): every part below the root (researchers' lip-sync heads, hand items)
template <class P> void Game<P>::body_role_model_set(uint32_t obj, int role, uint32_t model)
{
    if constexpr (P::pc) {
        (void)obj, (void)role, (void)model;   // the PC form: not ported (vtable only; ProofPc replays it)
    } else {
        const uint32_t root = mem.u32(obj + 0xC8);
        const uint32_t list = mem.u32(root + 0x24);
        for (int32_t i = 0; i < int32_t(mem.u32(root + 0x34)); ++i)
            body_role_model_set_rec(mem.u32(list + 4 * uint32_t(i)), role, model);
    }
}

// 0x6A3C0 body_node_model_rec(part, joint): the model of the part with that joint index into 0x5434A0
template <class P> void Game<P>::body_node_model_rec(uint32_t part, int joint)
{
    if (int32_t(mem.u32(part + 0x38)) == joint) {
        mem.set32(kNodeModel, mem.u32(part));
        return;
    }
    const uint32_t list = mem.u32(part + 0x24);
    for (int32_t i = 0; i < int32_t(mem.u32(part + 0x34)); ++i)
        body_node_model_rec(mem.u32(list + 4 * uint32_t(i)), joint);
}

// 0x6A410 body_node_model(obj, joint): the model of the part with that joint index, 0 if none
template <class P> uint32_t Game<P>::body_node_model(uint32_t obj, int node)
{
    if constexpr (P::pc) {
        (void)obj, (void)node;   // the PC form: not ported (vtable only; ProofPc replays it)
        return 0;
    } else {
        mem.set32(kNodeModel, 0);
        body_node_model_rec(mem.u32(obj + 0xC8), node);
        return mem.u32(kNodeModel);
    }
}

// 0x6A470 body_part_set_model_rec(part): the part with the joint 0x5434A4 draws model 0x5434A8 and its children are
// marked severed; else its children in turn
template <class P> void Game<P>::body_part_set_model_rec(uint32_t part)
{
    if (int32_t(mem.u32(part + 0x38)) == int32_t(mem.u32(kSetJoint))) {
        mem.set32(part, mem.u32(kSetModel));
        for (int32_t i = 0; i < int32_t(mem.u32(part + 0x34)); ++i)
            mem.at<uint8_t>(part + 0x28 + uint32_t(i)) |= 1;
        return;
    }
    const uint32_t list = mem.u32(part + 0x24);
    for (int32_t i = 0; i < int32_t(mem.u32(part + 0x34)); ++i)
        body_part_set_model_rec(mem.u32(list + 4 * uint32_t(i)));
}

// 0x6A640 axe_hand_model_hide(obj, joint, model): that part draws `model`, its children marked severed (not freed)
template <class P> void Game<P>::axe_hand_model_hide(uint32_t obj, int node, uint32_t model)
{
    if constexpr (P::pc) {
        (void)obj, (void)node, (void)model;   // the PC form: not ported (vtable only; ProofPc replays it)
    } else {
        mem.set32(kSetJoint, uint32_t(node));
        mem.set32(kSetModel, model);
        body_part_set_model_rec(mem.u32(obj + 0xC8));
    }
}

// 0x6A4F0 body_part_set_model(obj, joint, model): as axe_hand_model_hide, then body_free_cut_parts (they are gone)
template <class P> void Game<P>::body_part_set_model(uint32_t obj, int node, uint32_t model)
{
    if constexpr (P::pc) {
        (void)obj, (void)node, (void)model;   // the PC form: not ported (vtable only; ProofPc replays it)
    } else {
        mem.set32(kSetJoint, uint32_t(node));
        mem.set32(kSetModel, model);
        body_part_set_model_rec(mem.u32(obj + 0xC8));
        body_free_cut_parts(mem.u32(obj + 0xC8));
    }
}

} // namespace hotd::port
