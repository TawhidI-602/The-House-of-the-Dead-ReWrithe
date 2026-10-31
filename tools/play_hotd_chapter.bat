@echo off
rem Double-click to play ONE chapter of House of the Dead (arcade, US, red blood, free play) in MAME, fullscreen.
rem It asks which chapter (1-4); the game then starts directly in that chapter. Everything else as in play_hotd.bat:
rem mouse cursor = light gun (left click = shoot), infinite lives and ammo, press any key once at MAME's red warning, Esc to quit.
rem The recording's name contains the chapter (e.g. play_us_ch3_...), so the replay starts the same chapter.
set /p CH=Which chapter do you want to play (1-4)?
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0play_hotd.ps1" -Chapter %CH%
if errorlevel 1 echo. & echo Something went wrong - see the message above.
pause
