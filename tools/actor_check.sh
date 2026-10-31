#!/bin/bash
# pmfix (2026-10-28): the cut-scene actors' render check. Every actor in every role (--cast X,X,X,X) in a scene of each
# chapter, both models, with HOTD_ACTOR_CHECK=1 (game/creature.cpp actor_dress: every body part drawn as the actor's,
# Rogan's coat only on Rogan, the actor's belly skin / Sophie's strip on any body it plays). Prints one line per run and
# "actor check: all N runs passed" or the failed ones.
# usage: tools/actor_check.sh <hotd.exe> <checkout (--root)> <out dir>   (HOTD_PREF_DIR: a test folder)
EXE="$1"; ROOT="$2"; OUT="$3"
[ -x "$EXE" ] && [ -d "$ROOT" ] && [ -n "$OUT" ] || { echo "usage: $0 <hotd.exe> <checkout> <out dir>"; exit 2; }
mkdir -p "$OUT"
export HOTD_ACTOR_CHECK=1 EXE ROOT OUT
run() {
    local a=$1 sc=$2 m=$3 f="$OUT/${1}_${2/:/-}_$3.txt"
    timeout 120 "$EXE" --root "$ROOT" --scene-view "$sc" --cast "$a,$a,$a,$a" --models "$m" --frames 1 --mute > "$f" 2>&1
    local e=$? fails; fails=$(grep -c 'actor check FAILED' "$f")
    echo "$a $sc $m exit=$e $(grep 'actor check:' "$f" | tail -1 | sed 's/.*actor check: //') failures=$fails"
}
export -f run
for a in rogan g p3 p4 sophie r1 r2 r3 r4 r5 r6 w1 w2 w3 w4 curien curien2 zsophie; do
    for sc in 1:7 2:12 3:15 4:17 4:19; do
        for m in arcade pc; do echo "$a $sc $m"; done
    done
done | xargs -P 4 -L 1 bash -c 'run "$0" "$1" "$2"' | tee "$OUT/summary.txt"
bad=$(grep -v "exit=0 .*failures=0$" "$OUT/summary.txt" | wc -l)
n=$(wc -l < "$OUT/summary.txt")
[ "$bad" -eq 0 ] && echo "actor check: all $n runs passed" || { echo "actor check: $bad of $n runs FAILED"; exit 1; }
