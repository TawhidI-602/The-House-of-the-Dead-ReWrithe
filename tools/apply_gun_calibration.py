#!/usr/bin/env python3
"""
apply_gun_calibration.py - compute the light-gun calibration from a calibrate_gun.ps1 measurement.

Input (calibration folder): clicks.txt (time, cursor x, cursor y, screen w, screen h per left click; read-only logging in Windows)
and calib_ports.txt (n, time, gun X, gun Y per trigger press, from MAME). The last 9 clicks are paired with the 9 presses, in order:
  1 center, 2 top-left, 3 top-middle, 4 top-right, 5 right-middle, 6 bottom-right, 7 bottom-middle, 8 bottom-left, 9 left-middle
of the GAME PICTURE. Output: a fit gun = a * cursor + b per axis, the picture rectangle in cursor pixels, and the test-menu key
sequence that shoots the game's two GUN ADJUSTMENT targets (picture pixels (31.5, 31.5) and (247.5, 191.5) of 496 x 384) with the
gun values the cursor produces there. Prints "KEYS=<sequence>" for calibrate_gun.ps1; exits with code 1 if the data looks wrong.
"""
import sys, os
import numpy as np

W, H = 496.0, 384.0
TARGETS = [(31.5, 31.5), (247.5, 191.5)]
# menu: TEST, GUN SETTING (9 x SERVICE), TEST, PLAYER1 GUN ADJUSTMENT (2 x SERVICE), TEST, shoot both targets, wait for
# "NOW CALCULATING" -> "GUN MARK CHECKING", TEST = memorize, then EXIT (6 x SERVICE from PLAYER1 GUN ADJUSTMENT), TEST,
# main menu EXIT (3 x SERVICE from GUN SETTING), TEST
PREFIX = "T,W,S,S,S,S,S,S,S,S,S,T,W,S,S,T,W"
SUFFIX = "W,W,W,T,W,S,S,S,S,S,S,T,W,S,S,S,T,W,W"


def main(folder):
    clicks = [l.split() for l in open(os.path.join(folder, "clicks.txt")) if l.strip()]
    ports = [l.split() for l in open(os.path.join(folder, "calib_ports.txt")) if l.strip()]
    if len(ports) < 9 or len(clicks) < 9:
        print(f"need 9 clicks and 9 gun presses, got {len(clicks)} clicks / {len(ports)} presses"); return 1
    c = np.array([[float(x[1]), float(x[2])] for x in clicks[-9:]])
    g = np.array([[float(x[2]), float(x[3])] for x in ports[:9]])
    print(f"screen during calibration: {clicks[-1][3]} x {clicks[-1][4]}")
    fits = []
    for ax, name in ((0, "X"), (1, "Y")):
        a, b = np.polyfit(c[:, ax], g[:, ax], 1)
        res = g[:, ax] - (a * c[:, ax] + b)
        fits.append((a, b))
        print(f"gun {name} = {a:.5f} * cursor {name} + {b:.2f}   (max residual {np.abs(res).max():.1f})")
        if np.abs(res).max() > 6:
            print("residuals too large: clicks and presses don't line up (wrong order, missed click?)"); return 1
    left, right = c[[1, 7, 8], 0].mean(), c[[3, 4, 5], 0].mean()
    top, bottom = c[[1, 2, 3], 1].mean(), c[[5, 6, 7], 1].mean()
    print(f"game picture in cursor pixels: x {left:.0f}..{right:.0f}, y {top:.0f}..{bottom:.0f}")
    print(f"gun values at the picture edges: X {fits[0][0]*left+fits[0][1]:.0f}..{fits[0][0]*right+fits[0][1]:.0f}, "
          f"Y {fits[1][0]*top+fits[1][1]:.0f}..{fits[1][0]*bottom+fits[1][1]:.0f}  (game calibration so far: X 173..596, Y 87..380)")
    shots = []
    for tx, ty in TARGETS:
        cx = left + tx / W * (right - left)
        cy = top + ty / H * (bottom - top)
        gx, gy = fits[0][0] * cx + fits[0][1], fits[1][0] * cy + fits[1][1]
        shots.append(f"G{int(round(gx))}:{int(round(gy))}")
        print(f"target ({tx}, {ty}) -> cursor ({cx:.0f}, {cy:.0f}) -> gun ({gx:.0f}, {gy:.0f})")
    print("KEYS=" + ",".join([PREFIX, shots[0], "W", shots[1], SUFFIX]))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1]))
