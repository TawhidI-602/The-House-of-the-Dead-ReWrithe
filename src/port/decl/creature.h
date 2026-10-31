// Declarations of class Game, included inside its body (port/game.h, docs/port.md "Adding a kind"): the creature core
// (creature.cpp, teaser L3): spawning a body (body_init, the part tree, creature_type_setup), the motion family
// (set_motion ..., the frame counters, the pose), drawing the body (object_draw, body_draw, part_draw: the part matrices
// the shot test reads). Arcade forms only so far (no Game<Pc> instantiation). These stay virtual: the runtime (src/game)
// still overrides some of them with its own first versions until R2 drops those.
    // --- the creature core (creature.cpp) ---
    // spawning a body
    virtual void body_init(uint32_t obj, int32_t type, uint32_t motion);      // 0x38BD0
    void part_init(uint32_t part, uint32_t parent, uint32_t data);            // 0x36750
    void body_part_alloc(uint32_t parent, uint32_t data, int32_t index);      // 0x36880
    virtual void creature_type_setup(uint32_t obj);                           // 0x67CF0
    void creature_part_types_set(uint32_t part);                              // 0x67C90
    virtual void obj_clear_37c(uint32_t obj);                                 // 0x32C70
    void skin_template_copy(uint32_t obj);                                    // 0x211F0
    void sophie_skin_template_copy(uint32_t obj);                             // 0x21800
    // motions
    virtual void set_motion(uint32_t obj, uint32_t motion);                   // 0x39180
    virtual void set_motion2(uint32_t obj, uint32_t motion);                  // 0x391C0
    virtual void set_motion_blend(uint32_t obj, uint32_t motion, int a, int frames);   // 0x381D0
    void set_motion_at_frame(uint32_t obj, uint32_t motion, int a, int frames);        // 0x38490
    virtual void turn_motion_set_speed(uint32_t obj, uint32_t motion, int a, int frames, uint32_t angle);   // 0x38750
    virtual void set_motion2_at_frame(uint32_t obj, uint32_t motion, int frame, int blend);   // 0x388A0
    virtual void set_motion_blend_hold(uint32_t obj, uint32_t motion, int a, int b, int frames);   // 0x38A20
    virtual void body_second_motion(uint32_t obj, int frames);                // 0x39220
    virtual void set_motion2_blend(uint32_t obj, int frames);                 // 0x39330 UNVERIFIED (teaser): not reached in l3_cr1
    void body_pose_copy(uint32_t obj, uint32_t from, uint32_t to);            // 0x37FF0
    void body_pose_set_rec(uint32_t part, uint32_t pose);                     // 0x38060
    void body_motion2_frame_flags(uint32_t obj);                              // 0x368F0
    void body_motion_advance(uint32_t obj);                                   // 0x369E0
    void body_motion2_advance(uint32_t obj);                                  // 0x36B80 (the end flags, after drawing)
    void body_pose(uint32_t obj);                                             // 0x36CA0
    // drawing
    virtual void object_draw(uint32_t obj);                                   // 0x67DF0
    void object_draw_model(uint32_t obj);                                     // 0x67F60 (draw mode 2) UNVERIFIED (teaser): not reached in l3_cr1
    virtual void body_draw(uint32_t obj);                                     // 0x37720 (draw mode 0)
    void part_draw(uint32_t part);                                            // 0x36240
    void part_hit_mark(uint32_t part);                                        // 0x9CBD0
    void body_hook_angle_ease(uint32_t body);                                 // 0x9EE20
    // helpers without an original routine of their own (inline in the routines above)
    int32_t body_joints(uint32_t obj);                                        // joints of the body's type (0xC73C0)
    uint32_t motion_frame_record(uint32_t obj, int32_t motion, int32_t frame);
    void pose_blend_joints(uint32_t obj, uint32_t from, uint32_t to, uint32_t out, int32_t t);
    void pose_blend_root(uint32_t from, uint32_t to, uint32_t out, int32_t frame, int32_t rest);
    void motion_mirror(uint32_t obj, int32_t oldMotion, int32_t newMotion, uint32_t from, uint32_t to);
    void motion_blend_start(uint32_t obj, uint32_t motion, int start, int frames, bool atFrame);
    // boundaries only the creature core calls
    virtual void skin_draw(uint32_t obj) {}                                    // 0x214F0
    virtual void sophie_skin_draw(uint32_t obj);                               // 0x21C30 (blow_off.cpp)
