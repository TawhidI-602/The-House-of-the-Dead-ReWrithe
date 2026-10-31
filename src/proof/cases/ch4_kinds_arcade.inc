// Cases of runRoutine (the arcade: routine address -> the port), included inside its switch (proof/main.cpp):
// Chapter 4's own kinds (ch4_kinds.cpp) and prop sub-types (props_ch4.cpp).
    case 0x4D50: a.kind191_route_by_rescues(regs[0]); return true;
    case 0x31390: a.event_model_fade(regs[0]); return true;
    case 0x3B020: a.kind154_prop_group(regs[0]); return true;
    case 0x87F10: a.kind25_kyurianb_curien(regs[0]); return true;
    case 0x87F80: a.kyurianb_wait_scene(regs[0]); return true;
    case 0x87FB0: a.kyurianb_wait_point(regs[0]); return true;
    case 0x88000: a.kyurianb_pointing(regs[0]); return true;
    case 0x88050: a.kyurianb_wait_struck(regs[0]); return true;
    case 0x88090: a.kyurianb_struck(regs[0]); return true;
    case 0x880D0: a.kyurianb_collapse_a(regs[0]); return true;
    case 0x88120: a.kyurianb_collapse_b(regs[0]); return true;
    case 0x88180: a.kyurianb_collapse_c(regs[0]); return true;
    case 0x881C0: a.kyurianb_wait_end(regs[0]); return true;
    case 0x3F460: a.prop_r4_mechanism(regs[0]); return true;
    case 0x3F480: a.prop_r4_mechanism_run(regs[0]); return true;
    case 0x3F600: a.prop090_drop(regs[0]); return true;
    case 0x3F800: a.prop104_kakaaa(regs[0]); return true;
    case 0x3F840: a.prop108_pushukeke(regs[0]); return true;
    case 0x3F880: a.prop120_kami_region(regs[0]); return true;
    case 0x4DF50: a.player_scene17_dash(regs[0]); return true;
    case 0x4E040: a.player_scene17_dash_p1(regs[0]); return true;
    case 0x4E0A0: case 0x4E150: case 0x4E270: a.player_scene_nop(regs[0]); return true;
    case 0x4E0B0: a.player_scene17_dash_p2(regs[0]); return true;
    case 0x4E100: a.player_scene17_dash_p2_p(regs[0]); return true;
    case 0x4E160: a.player_body_at_scene_pos(regs[0]); return true;
    case 0x4E220: a.player_body_at_scene_pos_wait(regs[0]); return true;
