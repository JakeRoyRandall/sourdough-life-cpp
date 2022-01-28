#!/bin/sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
BIN=${TMPDIR:-/tmp}/sourdough-life-cycle-test
clang++ -std=c++17 -Wall -Wextra -Werror "$ROOT/app/main.cpp" "$ROOT/app/life.cpp" -o "$BIN"
CAPBIN=${TMPDIR:-/tmp}/sourdough-life-cycle-cap-test
clang++ -std=c++17 -Wall -Wextra -Werror -DSOURDOUGH_CYCLE_MEMORY_CAP=1 "$ROOT/app/main.cpp" "$ROOT/app/life.cpp" -o "$CAPBIN"

block=$($BIN --pattern block --width 6 --height 6 --steps 8 --detect-cycle)
printf '%s\n' "$block" | grep -F 'CYCLE DETECTED · transient 0 · period 1 · generation 1' >/dev/null
printf '%s\n' "$block" | grep -F 'AFTER 1 STEPS' >/dev/null

blinker=$($BIN --pattern blinker --width 7 --height 7 --steps 8 --detect-cycle)
printf '%s\n' "$blinker" | grep -F 'CYCLE DETECTED · transient 0 · period 2 · generation 2' >/dev/null
printf '%s\n' "$blinker" | grep -F 'AFTER 2 STEPS' >/dev/null

empty=${TMPDIR:-/tmp}/sourdough-empty-cycle.txt
printf '....\n....\n' > "$empty"
empty_output=$($BIN --load "$empty" --steps 8 --detect-cycle)
printf '%s\n' "$empty_output" | grep -F 'CYCLE DETECTED · transient 0 · period 1 · generation 1' >/dev/null
printf '%s\n' "$empty_output" | grep -F 'AFTER 1 STEPS' >/dev/null

glider=$($BIN --pattern glider --width 10 --height 10 --steps 2 --detect-cycle)
if printf '%s\n' "$glider" | grep -F 'CYCLE DETECTED' >/dev/null; then
    echo 'finite glider unexpectedly cycled' >&2
    exit 1
fi
printf '%s\n' "$glider" | grep -F 'AFTER 2 STEPS' >/dev/null

normal=$($BIN --pattern glider --width 10 --height 10 --steps 2)
detected=$($BIN --pattern glider --width 10 --height 10 --steps 2 --detect-cycle)
if [ "$normal" != "$detected" ]; then
    echo 'cycle flag changed a no-cycle run' >&2
    exit 1
fi

svg=${TMPDIR:-/tmp}/sourdough-cycle.svg
$BIN --pattern block --width 6 --height 6 --steps 8 --detect-cycle --svg "$svg" --force >/dev/null
grep -F 'generation 1' "$svg" >/dev/null

cap_output=$($CAPBIN --pattern block --width 6 --height 6 --steps 8 --detect-cycle)
printf '%s\n' "$cap_output" | grep -F 'CYCLE DETECTION LIMIT REACHED before storing states' >/dev/null
if printf '%s\n' "$cap_output" | grep -F 'CYCLE DETECTED' >/dev/null; then
    echo 'cycle detection remained active after its memory cap' >&2
    exit 1
fi
printf '%s\n' "$cap_output" | grep -F 'AFTER 8 STEPS' >/dev/null

empty_stats=$($BIN --load "$empty" --steps 1 --stats)
printf '%s\n' "$empty_stats" | grep -F 'INITIAL STATS · live 0 · bounds empty' >/dev/null
printf '%s\n' "$empty_stats" | grep -F 'FINAL STATS · live 0 · bounds empty' >/dev/null

block_stats=$($BIN --pattern block --width 6 --height 6 --steps 8 --detect-cycle --stats)
printf '%s\n' "$block_stats" | grep -F 'INITIAL STATS · live 4 · bounds x=2..3 y=2..3 (2x2)' >/dev/null
printf '%s\n' "$block_stats" | grep -F 'CYCLE DETECTED · transient 0 · period 1 · generation 1' >/dev/null
printf '%s\n' "$block_stats" | grep -F 'FINAL STATS · live 4 · bounds x=2..3 y=2..3 (2x2)' >/dev/null

glider_stats=$($BIN --pattern glider --width 10 --height 10 --steps 1 --stats)
printf '%s\n' "$glider_stats" | grep -F 'INITIAL STATS · live 5 · bounds x=4..6 y=4..6 (3x3)' >/dev/null
printf '%s\n' "$glider_stats" | grep -F 'FINAL STATS · live 5 · bounds x=4..6 y=5..7 (3x3)' >/dev/null

rule_grid=${TMPDIR:-/tmp}/sourdough-rule-grid.txt
printf '.....\n.###.\n.#.#.\n..#..\n.....\n' > "$rule_grid"
rule_output=$($BIN --load "$rule_grid" --rule B63/S32 --steps 1)
printf '%s\n' "$rule_output" | grep -F '· rule B36/S23' >/dev/null
rule_svg=${TMPDIR:-/tmp}/sourdough-rule.svg
$BIN --load "$rule_grid" --rule B63/S32 --steps 1 --svg "$rule_svg" --force >/dev/null
grep -F 'RULE B36/S23' "$rule_svg" >/dev/null
if $BIN --rule B33/S23 >/dev/null 2>&1; then
    echo 'duplicate rule digit was accepted' >&2
    exit 1
fi
if $BIN --rule B3/S2x >/dev/null 2>&1; then
    echo 'invalid rule character was accepted' >&2
    exit 1
fi
for empty_rule in B/S B3/S B/S23; do
    $BIN --width 3 --height 3 --rule "$empty_rule" --steps 1 >/dev/null
done

density_zero=$($BIN --width 4 --height 3 --seed 17 --density 0 --steps 1 --stats)
printf '%s\n' "$density_zero" | grep -F 'INITIAL STATS · live 0 · bounds empty' >/dev/null
density_full=$($BIN --width 4 --height 3 --seed 17 --density 1 --steps 1 --stats)
printf '%s\n' "$density_full" | grep -F 'INITIAL STATS · live 12 · bounds x=0..3 y=0..2 (4x3)' >/dev/null
density_a=$($BIN --width 8 --height 5 --seed 17 --density 0.5 --steps 2)
density_b=$($BIN --width 8 --height 5 --seed 17 --density 0.5 --steps 2)
[ "$density_a" = "$density_b" ]
default_seed=$($BIN --width 8 --height 5 --seed 17 --steps 2)
explicit_default=$($BIN --width 8 --height 5 --seed 17 --density 0.28 --steps 2)
[ "$default_seed" = "$explicit_default" ]
for bad_density in nan inf -0.1 1.1; do
    if $BIN --density "$bad_density" >/dev/null 2>&1; then
        echo "invalid density was accepted: $bad_density" >&2
        exit 1
    fi
done
if $BIN --pattern block --density 0.5 >/dev/null 2>&1; then
    echo 'density was accepted with a named pattern' >&2
    exit 1
fi
if $BIN --load "$empty" --density 0.5 >/dev/null 2>&1; then
    echo 'density was accepted with a loaded grid' >&2
    exit 1
fi

origin=$($BIN --width 4 --height 4 --pattern block --at 0,0 --steps 1 --stats)
printf '%s\n' "$origin" | grep -F 'INITIAL STATS · live 4 · bounds x=0..1 y=0..1 (2x2)' >/dev/null
edge=$($BIN --width 3 --height 3 --pattern glider --at 0,0 --steps 1 --stats)
printf '%s\n' "$edge" | grep -F 'INITIAL STATS · live 5 · bounds x=0..2 y=0..2 (3x3)' >/dev/null
for bad_at in '-1,0' '1' '1,2,3' 'x,0' '1,-2' '1,' ',2'; do
    if $BIN --pattern block --at "$bad_at" >/dev/null 2>&1; then
        echo "invalid placement was accepted: $bad_at" >&2
        exit 1
    fi
done
if $BIN --pattern block --at 3,3 --width 4 --height 4 >/dev/null 2>&1; then
    echo 'oversized placement was accepted' >&2
    exit 1
fi
if $BIN --width 4 --height 4 --at 0,0 >/dev/null 2>&1; then
    echo 'placement was accepted with random pattern' >&2
    exit 1
fi
if $BIN --load "$empty" --at 0,0 >/dev/null 2>&1; then
    echo 'placement was accepted with a loaded grid' >&2
    exit 1
fi
default_block=$($BIN --width 6 --height 6 --pattern block --steps 2)
center_block=$($BIN --width 6 --height 6 --pattern block --at 2,2 --steps 2)
[ "$default_block" = "$center_block" ]

json_file=${TMPDIR:-/tmp}/sourdough-final.json
json_save=${TMPDIR:-/tmp}/sourdough-json-grid.txt
json_output=$($BIN --width 6 --height 6 --pattern glider --steps 2 --json --save "$json_save" --force)
printf '%s\n' "$json_output" > "$json_file"
python3 - "$json_file" "$json_save" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as stream:
    document = json.load(stream)
with open(sys.argv[2], encoding="utf-8") as stream:
    saved = stream.read().splitlines()
assert document["width"] == 6 and document["height"] == 6
assert document["boundary"] == "finite" and document["rule"] == "B3/S23"
assert document["generation"] == 2 and document["cycle"] is None
assert document["grid"] == saved
assert document["live"] == sum(row.count("#") for row in document["grid"])
PY
cycle_json=$($BIN --width 6 --height 6 --pattern block --steps 8 --detect-cycle --json)
printf '%s\n' "$cycle_json" | python3 -c 'import json, sys; d=json.load(sys.stdin); assert d["generation"] == 1 and d["cycle"] == {"transient": 0, "period": 1}'
wrap_json=$($BIN --width 6 --height 6 --pattern block --steps 1 --wrap --rule B63/S32 --json)
printf '%s\n' "$wrap_json" | python3 -c 'import json, sys; d=json.load(sys.stdin); assert d["boundary"] == "wrap" and d["rule"] == "B36/S23"'

echo 'cycle CLI tests passed: block period-1, blinker period-2, empty grid, glider cutoff, parity, SVG generation'
