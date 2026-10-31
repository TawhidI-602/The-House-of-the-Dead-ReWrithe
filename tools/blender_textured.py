"""
blender_textured.py - render a textured level .obj (from m2_textures.py level) with Blender, no window.

  blender -b --factory-startup --python blender_textured.py -- <folder> <name> [cam x y z  target x y z] ...
Camera positions are in game coordinates (Y up); several cameras = several images. Always writes <name>_top.png too.
Writes <name>_cam<n>.png and <name>.blend into the folder.
"""
import sys, os
import bpy
from mathutils import Vector

args = sys.argv[sys.argv.index("--") + 1:]
folder, name = args[0], args[1]
nums = [float(x) for x in args[2:]]
cams = [(nums[i:i + 3], nums[i + 3:i + 6]) for i in range(0, len(nums) - 5, 6)]

bpy.ops.wm.read_factory_settings(use_empty=True)
scene = bpy.context.scene
bpy.ops.wm.obj_import(filepath=os.path.join(folder, "level.obj"))
for img in bpy.data.images:          # keep texels sharp, like the 4-bit sheets
    img.alpha_mode = "NONE"
for mat in bpy.data.materials:
    if mat.use_nodes:
        for node in mat.node_tree.nodes:
            if node.type == "TEX_IMAGE":
                node.interpolation = "Closest"

scene.render.engine = "BLENDER_WORKBENCH"
scene.display.shading.light = "FLAT"
scene.display.shading.color_type = "TEXTURE"
scene.display.shading.show_backface_culling = False
scene.render.resolution_x, scene.render.resolution_y = 1280, 960
world = bpy.data.worlds.new("bg"); world.color = (0.2, 0.2, 0.25); scene.world = world

cam_data = bpy.data.cameras.new("cam")
cam = bpy.data.objects.new("cam", cam_data)
scene.collection.objects.link(cam)
scene.camera = cam


def game_to_blender(p):                # OBJ import: game (x, y, z) Y-up -> Blender (x, -z, y)
    return Vector((p[0], -p[2], p[1]))


objs = [o for o in bpy.data.objects if o.type == "MESH"]
pts = [o.matrix_world @ Vector(c) for o in objs for c in o.bound_box]
lo = Vector((min(p.x for p in pts), min(p.y for p in pts), min(p.z for p in pts)))
hi = Vector((max(p.x for p in pts), max(p.y for p in pts), max(p.z for p in pts)))
center, extent = (lo + hi) / 2, max(hi - lo)
cam_data.type = "ORTHO"; cam_data.ortho_scale = extent * 1.05; cam_data.clip_end = extent * 10
cam.location = (center.x, center.y, hi.z + extent); cam.rotation_euler = (0, 0, 0)
scene.render.filepath = os.path.join(folder, f"{name}_top.png")
bpy.ops.render.render(write_still=True)

cam_data.type = "PERSP"; cam_data.lens = 18; cam_data.clip_start = 0.5; cam_data.clip_end = 5000
for n, (pos, target) in enumerate(cams):
    cam.location = game_to_blender(pos)
    cam.rotation_euler = (game_to_blender(target) - cam.location).to_track_quat("-Z", "Y").to_euler()
    scene.render.filepath = os.path.join(folder, f"{name}_cam{n}.png")
    bpy.ops.render.render(write_still=True)

bpy.ops.wm.save_as_mainfile(filepath=os.path.join(folder, f"{name}.blend"))
print("textured render done")
