@echo off
rem Double-click to play House of the Dead (arcade, US region, English text, red blood, free play) in MAME, fullscreen.
rem The mouse cursor is the light gun (left click = shoot); infinite lives and ammo; Start is pressed automatically.
rem Press any key once at MAME's red warning screen, Esc to quit.
rem Your inputs are recorded (private\mame_play\inp) so they can be replayed later for dumps.
rem If MAME fails, its error message is shown below and the window stays open.
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0play_hotd.ps1"
if errorlevel 1 echo. & echo Something went wrong - see the message above.
pause
