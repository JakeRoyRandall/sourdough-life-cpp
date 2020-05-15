# Sourdough Life

Sourdough Life is a small terminal Conway cellular automaton. It is a pure toy grid: the kitchen-counter framing is fictional, and it makes no claims about medical biology, fermentation, or real organisms.

Created September 2026 retrospectively. This is a fictional art project, not a historical work record; the deliberate art dates are part of the framing.

Build and run:

```sh
clang++ -std=c++17 -Wall -Wextra -Werror app/main.cpp app/life.cpp -o sourdough-life
./sourdough-life --width 24 --height 10 --steps 12 --seed 42
./sourdough-life --pattern glider --width 12 --height 10 --steps 4
```

Patterns are `random`, `block`, `blinker`, and `glider`. The board has finite dead boundaries: cells outside the printed rectangle are always dead, with no torus wrapping. Dimensions are bounded to 200×100 and steps to 10,000.

Tests compile and run the same `LifeGrid` implementation:

```sh
clang++ -std=c++17 -Wall -Wextra -Werror tests.cpp app/life.cpp -o sourdough-tests
./sourdough-tests
```
