#include "app/life.hpp"
#include <cassert>
#include <iostream>
#include <stdexcept>
#include <fstream>

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
    LifeGrid saved(5, 3); saved.place("blinker", 1, 1); const std::string savePath = "/tmp/sourdough-grid.txt"; savePlain(saved, savePath, true); LifeGrid loaded = loadPlain(savePath); assert(loaded.width() == 5 && loaded.height() == 3 && loaded.plain() == saved.plain());
    { std::ofstream bad("/tmp/sourdough-ragged.txt"); bad << "..#\n.#\n"; } bool loadRejected = false; try { (void)loadPlain("/tmp/sourdough-ragged.txt"); } catch (const std::runtime_error&) { loadRejected = true; } assert(loadRejected);
    { std::ofstream bad("/tmp/sourdough-chars.txt"); bad << "..x\n...\n"; } loadRejected = false; try { (void)loadPlain("/tmp/sourdough-chars.txt"); } catch (const std::runtime_error&) { loadRejected = true; } assert(loadRejected);
    const std::string svg = block.svg(7); assert(svg.find("<svg ") == 0); assert(svg.find("generation 7") != std::string::npos); assert(svg.find("LIVE CELLS 4") != std::string::npos); assert(svg.find("finite dead boundary") != std::string::npos); int redCells = 0; size_t cursor = 0; while ((cursor = svg.find("width=\"23\" height=\"23\" fill=\"#d95f43\"", cursor)) != std::string::npos) { ++redCells; cursor += 5; } assert(redCells == 4);
    LifeGrid one(1, 1), large(200, 100); assert(one.svg(0).find("viewBox=\"0 0 620 ") != std::string::npos); assert(large.svg(12).find("viewBox=\"0 0 4856 ") != std::string::npos); assert(large.svg(12).find("LIVE CELLS 0") != std::string::npos);
    { std::ofstream crlf("/tmp/sourdough-max-crlf.txt", std::ios::binary); for (int row = 0; row < 100; ++row) { crlf << std::string(200, row == 0 ? '#' : '.'); crlf << "\r\n"; } } LifeGrid maxLoaded = loadPlain("/tmp/sourdough-max-crlf.txt"); assert(maxLoaded.width() == 200 && maxLoaded.height() == 100 && maxLoaded.alive(0, 0));
    LifeGrid finiteEdge(5, 5), wrappedEdge(5, 5, true); for (int x = 1; x <= 3; ++x) { finiteEdge.set(x, 0); wrappedEdge.set(x, 0); } finiteEdge.step(); wrappedEdge.step(); assert(living(finiteEdge) != living(wrappedEdge)); assert(wrappedEdge.alive(2, 4));
    LifeGrid finiteCorner(3, 3), wrappedCorner(3, 3, true); finiteCorner.set(0, 0); finiteCorner.set(1, 0); finiteCorner.set(0, 1); wrappedCorner.set(0, 0); wrappedCorner.set(1, 0); wrappedCorner.set(0, 1); finiteCorner.step(); wrappedCorner.step(); assert(finiteCorner.render() != wrappedCorner.render());
    LifeGrid oneCell(1, 1, true); oneCell.set(0, 0); oneCell.step(); assert(living(oneCell) == 0); LifeGrid twoCells(1, 2, true); twoCells.set(0, 0); twoCells.set(0, 1); twoCells.step(); assert(living(twoCells) == 0);
    assert(finiteEdge.svg(1).find("finite dead boundary") != std::string::npos); assert(wrappedEdge.svg(1).find("toroidal wrap boundary") != std::string::npos);
    std::cout << "tests passed: block, blinker period-2, glider movement, finite boundary, seeded determinism\n";
}
