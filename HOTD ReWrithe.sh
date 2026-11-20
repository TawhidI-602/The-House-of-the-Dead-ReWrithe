#!/bin/bash
# HOTD ReWrithe - PortMaster launch script (BORRADOR para R36S / ArkOS4Clones, sin probar en la consola)

XDG_DATA_HOME=${XDG_DATA_HOME:-$HOME/.local/share}

if [ -d "/opt/system/Tools/PortMaster/" ]; then
  controlfolder="/opt/system/Tools/PortMaster"
elif [ -d "/opt/tools/PortMaster/" ]; then
  controlfolder="/opt/tools/PortMaster"
elif [ -d "$XDG_DATA_HOME/PortMaster/" ]; then
  controlfolder="$XDG_DATA_HOME/PortMaster"
else
  controlfolder="/roms/ports/PortMaster"
fi

source $controlfolder/control.txt
[ -f "${controlfolder}/mod_${CFW_NAME}.txt" ] && source "${controlfolder}/mod_${CFW_NAME}.txt"
get_controls

GAMEDIR="/$directory/ports/hotdrewrithe"
cd "$GAMEDIR"

# log para depurar (se sobrescribe en cada arranque)
> "$GAMEDIR/log.txt" && exec > >(tee "$GAMEDIR/log.txt") 2>&1

# librerias propias (libSDL3.so.0 compilada con KMSDRM) y mapeo de mandos
export LD_LIBRARY_PATH="$GAMEDIR/libs:$LD_LIBRARY_PATH"
export SDL_GAMECONTROLLERCONFIG="$sdl_controllerconfig"
export SDL_VIDEODRIVER=kmsdrm
export HOTD_GLES=1

# Select -> Esc (pausa del juego; ahi esta Exit). El apuntado va por SDL, sin teclado.
$GPTOKEYB "hotd" -c "$GAMEDIR/hotd.gptk" &

# 640x480 en 4:3, modelos del PC de 1998 y sonido del PC (menos RAM y CPU que los modelos y el sonido arcade)
./bin/hotd --fullscreen --aspect 4:3 --models pc --sound pc

pm_finish
