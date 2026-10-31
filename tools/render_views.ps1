# render_views.ps1 - per-view comparison renders: for every dump tag, export the arcade level textured with *that moment's*
# texture RAM, palette and color tables (texture sets change between scenes), render PC / arcade / textured from that moment's
# camera, and compose them next to the MAME screenshot.
#
#   .\render_views.ps1 -Dumps "<dump folder with texram>" -Out "<output folder>" -Table bef00 -PcWorld "<WORLDn.RBH>" -Tags t00560,t00800
# Needs: the regions in private\arcade_exports\regions, Blender 5.2. Output stays in -Out (keep it inside private\).
param(
    [Parameter(Mandatory)] [string]$Dumps,
    [Parameter(Mandatory)] [string]$Out,
    [Parameter(Mandatory)] [string]$PcWorld,
    [Parameter(Mandatory)] [string[]]$Tags,
    [string]$Table = "bef00",
    [string]$Blender = "C:\Program Files\Blender Foundation\Blender 5.2\blender.exe"
)
$tools = $PSScriptRoot
$regions = Join-Path (Split-Path $tools) "private\arcade_exports\regions"
New-Item -ItemType Directory -Force "$Out\renders", "$Out\compare" | Out-Null
Push-Location $tools
try {
    # untextured levels, shared by all views
    if (-not (Test-Path "$Out\arcade_level.obj")) { py m2_level.py obj "$regions" $Table "$Out\arcade_level.obj" | Out-Host }
    if (-not (Test-Path "$Out\pc_world.obj")) { py pc_rbh.py obj "$PcWorld" "$Out\pc_world.obj" | Out-Host }
    foreach ($tag in $Tags) {
        $view = "$Out\views_$tag.json"
        py mame_camera.py "$Dumps" "$view" $tag | Out-Host
        $tex = "$Out\textured_$tag"
        $tr = "$Dumps\${tag}_texram.bin"
        py m2_textures.py level "$regions" $Table "${tr}:0" "${tr}:1" "$tex" "$Dumps\$tag" | Out-Host
        & $Blender -b --factory-startup --python blender_views.py -- "$view" "$Out\renders" solid "$Out\pc_world.obj" --label pc --blue 2>&1 | Select-String "Error|Traceback"
        & $Blender -b --factory-startup --python blender_views.py -- "$view" "$Out\renders" solid "$Out\arcade_level.obj" --label arcade 2>&1 | Select-String "Error|Traceback"
        & $Blender -b --factory-startup --python blender_views.py -- "$view" "$Out\renders" textured "$tex\level.obj" 2>&1 | Select-String "Error|Traceback"
        py compose_views.py "$view" "$Out\renders" "$Out\compare" pc arcade textured | Out-Host
    }
} finally { Pop-Location }
