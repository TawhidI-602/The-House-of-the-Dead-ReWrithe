// Camera rails (docs/formats.md §26): the PC's CAM\ALLSTAGE\CAMALL.RBH, the same spline data the arcade plays (the arcade's
// camera matches it exactly). 142 paths; each has six channels (eye x, y, z, target x, y, z) of Hermite keys
// (frame, value, in-tangent, out-tangent), 60 frames per second.
#pragma once

#include <string>
#include <vector>

namespace hotd {

class CameraPaths {
public:
    bool load(const std::string& camallPath, std::string& err);
    int count() const { return int(paths_.size()); }
    float length(int path) const;   // last key's frame
    // Eye and target at frame t (clamped to the path).
    void evaluate(int path, float t, float eye[3], float target[3]) const;

private:
    struct Key { float t, v, in, out; };
    using Channel = std::vector<Key>;
    struct Path { Channel ch[6]; };
    static float eval(const Channel& c, float t);
    std::vector<Path> paths_;
};

} // namespace hotd
