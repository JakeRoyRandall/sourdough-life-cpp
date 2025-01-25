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
./sourdough-life --pattern glider --width 12 --height 10 --steps 4 --stats
./sourdough-life --pattern glider --width 12 --height 10 --steps 12 --rule B36/S23
./sourdough-life --width 24 --height 10 --seed 42 --density 0.18 --steps 12
./sourdough-life --width 12 --height 10 --pattern glider --at 0,0 --steps 8
```

Patterns are `random`, `block`, `blinker`, and `glider`. The default board has finite dead boundaries: cells outside the printed rectangle are always dead. `--wrap` enables toroidal edges; wrapped neighbors are deduplicated, so tiny 1×1 and 1×2 boards never count the same cell multiple times. Dimensions are bounded to 200×100 and steps to 10,000. Plain `.#` grid saves contain cells only; boundary mode remains a CLI choice when loading.

`--save FILE` writes the final generation as a strict rectangular `.#` grid. Existing files are protected unless `--force` is supplied. `--load FILE` imports dimensions and cells from a nonempty rectangular `.#` grid; ragged rows, other characters, oversized files, and conflicts with `--width`, `--height`, `--seed`, or `--pattern` are rejected.

`--svg FILE` writes the final generation as a self-contained warm kitchen-counter SVG with crisp cell rectangles, generation number, live-cell count, and a finite-boundary legend. SVG output follows the same overwrite protection and `--force` rule.

`--detect-cycle` remembers complete grids, including the selected finite or toroidal boundary mode, and stops at the first repeated state with its transient length and period. The remembered-state table uses a conservative estimated 32 MiB state budget; if the budget is reached, detection is disabled, the table is released, and the program reports that it is continuing the ordinary simulation. The `AFTER` line and SVG generation use the actual executed generation after an early stop.

`--stats` adds an initial and final summary without changing the default output. Each summary reports living-cell count and the smallest axis-aligned bounding rectangle using zero-based coordinates; an empty grid is reported explicitly as `bounds empty`. The final summary reflects an early cycle stop when `--detect-cycle` is also used.

`--rule B3/S23` selects a strict outer-totalistic rule. Birth and survival digits must be unique values from 0 through 8; either section may be empty (`B/S23`, `B3/S`, or `B/S`), and digits are canonically sorted when reported. The default rule is Conway `B3/S23`, and custom rules are reported in the CLI header and SVG footer. Rules apply equally to finite and toroidal grids and are part of cycle state identity.

`--density 0..1` changes the initial live-cell probability for the seeded random pattern. It defaults to `0.28`, so the default seeded run is unchanged; `0` creates an empty board and `1` fills it. The value must be finite and is rejected with loaded grids or named patterns such as `block`.

`--at X,Y` places a named pattern at an explicit zero-based, nonnegative coordinate. The pattern must fit entirely inside the board; malformed coordinates, random initialization, loaded grids, and silent clipping are rejected. Without `--at`, named patterns keep their centered placement.

Tests compile and run the same `LifeGrid` implementation:

```sh
clang++ -std=c++17 -Wall -Wextra -Werror tests.cpp app/life.cpp -o sourdough-tests
./sourdough-tests
```

The CLI cycle checks compile the executable and cover stable block, period-2 blinker, empty grids, a finite glider cutoff, no-detection parity, and early-stop SVG metadata:

```sh
./tests/test_cycle.sh
```

The same CLI checks also cover empty, block, glider, and cycle-termination statistics.
