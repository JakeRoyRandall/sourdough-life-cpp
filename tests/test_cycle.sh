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

echo 'cycle CLI tests passed: block period-1, blinker period-2, empty grid, glider cutoff, parity, SVG generation'
