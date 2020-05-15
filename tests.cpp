#include "app/life.hpp"
#include <cassert>
#include <iostream>
#include <stdexcept>

static int living(const LifeGrid& grid) { int total = 0; for (int y = 0; y < grid.height(); ++y) for (int x = 0; x < grid.width(); ++x) total += grid.alive(x, y); return total; }
int main() {
    bool rejected = false; try { LifeGrid invalid(0, 10); } catch (const std::invalid_argument&) { rejected = true; } assert(rejected);
    rejected = false; try { LifeGrid invalid(-1000000, 10); } catch (const std::invalid_argument&) { rejected = true; } assert(rejected);
    rejected = false; try { LifeGrid invalid(201, 10); } catch (const std::invalid_argument&) { rejected = true; } assert(rejected);
    LifeGrid tiny(1, 1); assert(!tiny.place("block", 0, 0));
    LifeGrid unchanged(6, 6); unchanged.set(1, 1); const std::string before = unchanged.render(); assert(!unchanged.place("unknown", 0, 0)); assert(unchanged.render() == before); assert(!unchanged.place("glider", 5, 5)); assert(unchanged.render() == before);
    LifeGrid block(6, 6); assert(block.place("block", 2, 2)); block.step(); assert(living(block) == 4); assert(block.alive(2, 2) && block.alive(3, 3));
    LifeGrid blinker(7, 7); assert(blinker.place("blinker", 2, 3)); blinker.step(); assert(blinker.alive(3, 2) && blinker.alive(3, 3) && blinker.alive(3, 4)); blinker.step(); assert(blinker.alive(2, 3) && blinker.alive(3, 3) && blinker.alive(4, 3));
    LifeGrid glider(10, 10); assert(glider.place("glider", 1, 1)); glider.run(4); assert(glider.alive(3, 2) && glider.alive(4, 3) && glider.alive(2, 4));
    LifeGrid edge(5, 5); edge.set(0, 0); edge.set(1, 0); edge.set(0, 1); edge.step(); assert(edge.alive(0, 0)); assert(!edge.alive(4, 4));
    LifeGrid randomA(12, 8), randomB(12, 8); randomA.seed(77); randomB.seed(77); assert(randomA.render() == randomB.render());
    std::cout << "tests passed: block, blinker period-2, glider movement, finite boundary, seeded determinism\n";
}
