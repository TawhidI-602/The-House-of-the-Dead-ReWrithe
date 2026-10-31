"""
blender_views.py - render levels from the arcade's own camera positions (views.json from mame_camera.py), no window.

  blender -b --factory-startup --python blender_views.py -- <views.json> <out folder> <mode> <obj> [<obj2>] [--fov DEG] [--flip-up]
Modes:
  solid     render <obj> in one flat color (orange) -> <name>_<label>.png; use it once per level (PC, arcade)
  textured  render <obj> with its textures (unlit, like the game's pre-lit textures) -> <name>_textured.png
Optional: --label NAME (default: obj file name), --fov vertical field of view in degrees (default 45),
--flip-up if the camera's y axis points down on screen, --flip-forward if the camera looks along its -z axis.
Camera: game coordinates (Y up) are converted like Blender's OBJ import does: (x, y, z) -> (x, -z, y).
"""
import sys, os, json, math
import bpy
from mathutils import Vector, Matrix

args = sys.argv[sys.argv.index("--") + 1:]
views_file, out, mode, obj = args[0], args[1], args[2], args[3]
fov = float(args[args.index("--fov") + 1]) if "--fov" in args else 45.0
flip_up = "--flip-up" in args
flip_forward = "--flip-forward" in args
label = args[args.index("--label") + 1] if "--label" in args else os.path.splitext(os.path.basename(obj))[0]
color = (0.95, 0.55, 0.1, 1.0) if "--blue" not in args else (0.15, 0.45, 0.95, 1.0)
os.makedirs(out, exist_ok=True)

bpy.ops.wm.read_factory_settings(use_empty=True)
scene = bpy.context.scene
bpy.ops.wm.obj_import(filepath=obj)
for img in bpy.data.images:
    img.alpha_mode = "STRAIGHT"
for mat in bpy.data.materials:
    if mat.use_nodes:
        for node in mat.node_tree.nodes:
            if node.type == "TEX_IMAGE":
                node.interpolation = "Closest"

scene.render.engine = "BLENDER_WORKBENCH"
shading = scene.display.shading
shading.show_backface_culling = False
if mode == "textured":
    shading.light = "FLAT"
    shading.color_type = "TEXTURE"
else:
    shading.light = "STUDIO"
    shading.color_type = "SINGLE"
    shading.single_color = color[:3]
    shading.show_cavity = True
scene.render.resolution_x, scene.render.resolution_y = 496 * 2, 384 * 2     # Model 2 screen is 496 x 384
world = bpy.data.worlds.new("bg"); world.color = (0.05, 0.05, 0.08); scene.world = world

cam_data = bpy.data.cameras.new("cam")
cam_data.sensor_fit = "VERTICAL"
cam_data.angle = math.radians(fov)
cam_data.clip_start, cam_data.clip_end = 0.5, 20000
cam = bpy.data.objects.new("cam", cam_data)
scene.collection.objects.link(cam)
scene.camera = cam


def g2b(v):
    return Vector((v[0], -v[2], v[1]))


for view in json.load(open(views_file)):
    pos, fwd, up = g2b(view["pos"]), g2b(view["forward"]).normalized(), g2b(view["down_or_up"]).normalized()
    if flip_up:
        up = -up
    if flip_forward:
        fwd = -fwd
    right = fwd.cross(up).normalized()
    up = right.cross(fwd).normalized()
    rot = Matrix((right, up, -fwd)).transposed()          # columns = camera X, Y, Z (Blender cameras look along -Z)
    cam.matrix_world = Matrix.Translation(pos) @ rot.to_4x4()
    suffix = "textured" if mode == "textured" else label
    scene.render.filepath = os.path.join(out, f"{view['name']}_{suffix}.png")
    bpy.ops.render.render(write_still=True)
print("views done")
