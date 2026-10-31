"""
blender_overlay.py - load the PC world and the matched arcade models into Blender and render comparison images.

Run with Blender (no window):
  blender -b --factory-startup --python blender_overlay.py -- <folder> [arcade obj name] [output prefix]
The folder holds pc_world.obj and the arcade .obj (default arcade_matched.obj). Writes <prefix>_top.png, <prefix>_persp.png and
<prefix>.blend (default prefix "overlay"). PC = grey, arcade = red. Open the .blend in Blender to look around yourself.
"""
import sys, os, math
import bpy
from mathutils import Vector

args = sys.argv[sys.argv.index("--") + 1:]
folder = args[0]
arcade_name = args[1] if len(args) > 1 else "arcade_matched.obj"
prefix = args[2] if len(args) > 2 else "overlay"

bpy.ops.wm.read_factory_settings(use_empty=True)
scene = bpy.context.scene


def import_obj(name, color):
    before = set(bpy.data.objects)
    bpy.ops.wm.obj_import(filepath=os.path.join(folder, name))
    mat = bpy.data.materials.new(name)
    mat.diffuse_color = color
    objs = [o for o in bpy.data.objects if o not in before]
    for o in objs:
        o.data.materials.clear()
        o.data.materials.append(mat)
    return objs


pc = import_obj("pc_world.obj", (0.6, 0.6, 0.6, 1.0))
arcade = import_obj(arcade_name, (0.9, 0.1, 0.1, 1.0))

# bounds of everything (world space)
pts = [o.matrix_world @ Vector(c) for o in pc + arcade for c in o.bound_box]
lo = Vector((min(p.x for p in pts), min(p.y for p in pts), min(p.z for p in pts)))
hi = Vector((max(p.x for p in pts), max(p.y for p in pts), max(p.z for p in pts)))
center, extent = (lo + hi) / 2, max(hi - lo)

scene.render.engine = "BLENDER_WORKBENCH"
scene.display.shading.color_type = "MATERIAL"
scene.display.shading.show_backface_culling = False
scene.display.shading.show_xray = False
scene.render.resolution_x, scene.render.resolution_y = 1600, 1200
world = bpy.data.worlds.new("bg")
world.color = (1, 1, 1)
scene.world = world

cam_data = bpy.data.cameras.new("cam")
cam = bpy.data.objects.new("cam", cam_data)
scene.collection.objects.link(cam)
scene.camera = cam

# 1) top view, orthographic (Blender's OBJ import turns the game's Y-up into Z-up)
cam_data.type = "ORTHO"
cam_data.ortho_scale = extent * 1.05
cam_data.clip_end = extent * 10
cam.location = (center.x, center.y, hi.z + extent)
cam.rotation_euler = (0, 0, 0)
scene.render.filepath = os.path.join(folder, prefix + "_top.png")
bpy.ops.render.render(write_still=True)

# 2) perspective view from above one corner
cam_data.type = "PERSP"
cam_data.lens = 24
cam.location = center + Vector((-0.6, -0.6, 0.45)) * extent
direction = center - cam.location
cam.rotation_euler = direction.to_track_quat("-Z", "Y").to_euler()
scene.render.filepath = os.path.join(folder, prefix + "_persp.png")
bpy.ops.render.render(write_still=True)

bpy.ops.wm.save_as_mainfile(filepath=os.path.join(folder, prefix + ".blend"))
print("overlay done:", folder)
