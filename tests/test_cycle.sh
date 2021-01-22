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

echo 'cycle CLI tests passed: block period-1, blinker period-2, empty grid, glider cutoff, parity, SVG generation'
