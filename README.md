# Sourdough Life

Sourdough Life is a small terminal Conway cellular automaton. It is a pure toy grid: the kitchen-counter framing is fictional, and it makes no claims about medical biology, fermentation, or real organisms.

Created September 2026 retrospectively. This is a fictional art project, not a historical work record; the deliberate art dates are part of the framing.

Build and run:

```sh
clang++ -std=c++17 -Wall -Wextra -Werror app/main.cpp app/life.cpp -o sourdough-life
./sourdough-life --width 24 --height 10 --steps 12 --seed 42
./sourdough-life --pattern glider --width 12 --height 10 --steps 4
./sourdough-life --pattern blinker --width 5 --height 5 --steps 8 --wrap
./sourdough-life --load kitchen-grid.txt --steps 4 --save evolved-grid.txt --force
./sourdough-life --pattern glider --width 12 --height 10 --steps 4 --svg evolved.svg --force
./sourdough-life --pattern blinker --width 7 --height 7 --steps 20 --detect-cycle
```

Patterns are `random`, `block`, `blinker`, and `glider`. The default board has finite dead boundaries: cells outside the printed rectangle are always dead. `--wrap` enables toroidal edges; wrapped neighbors are deduplicated, so tiny 1×1 and 1×2 boards never count the same cell multiple times. Dimensions are bounded to 200×100 and steps to 10,000. Plain `.#` grid saves contain cells only; boundary mode remains a CLI choice when loading.

`--save FILE` writes the final generation as a strict rectangular `.#` grid. Existing files are protected unless `--force` is supplied. `--load FILE` imports dimensions and cells from a nonempty rectangular `.#` grid; ragged rows, other characters, oversized files, and conflicts with `--width`, `--height`, `--seed`, or `--pattern` are rejected.

`--svg FILE` writes the final generation as a self-contained warm kitchen-counter SVG with crisp cell rectangles, generation number, live-cell count, and a finite-boundary legend. SVG output follows the same overwrite protection and `--force` rule.

`--detect-cycle` remembers complete grids, including the selected finite or toroidal boundary mode, and stops at the first repeated state with its transient length and period. The remembered-state table uses a conservative estimated 32 MiB state budget; if the budget is reached, detection is disabled, the table is released, and the program reports that it is continuing the ordinary simulation. The `AFTER` line and SVG generation use the actual executed generation after an early stop.

Tests compile and run the same `LifeGrid` implementation:

```sh
clang++ -std=c++17 -Wall -Wextra -Werror tests.cpp app/life.cpp -o sourdough-tests
./sourdough-tests
```

The CLI cycle checks compile the executable and cover stable block, period-2 blinker, empty grids, a finite glider cutoff, no-detection parity, and early-stop SVG metadata:

```sh
./tests/test_cycle.sh
```
