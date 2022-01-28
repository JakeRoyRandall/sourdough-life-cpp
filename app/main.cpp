#include "life.hpp"
#include <cstdlib>
#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>
#include <iostream>
#include <stdexcept>
#include <string>
#include <filesystem>
#include <fstream>
#include <unordered_map>

#ifndef SOURDOUGH_CYCLE_MEMORY_CAP
#define SOURDOUGH_CYCLE_MEMORY_CAP (32u * 1024u * 1024u)
#endif

struct Options { int width = 20, height = 10, steps = 8, atX = 0, atY = 0; uint32_t seed = 2020; double density = 0.28; std::string pattern = "random", rule = "B3/S23", load, save, svg; bool force = false, wrap = false, detectCycle = false, stats = false, json = false, loadMode = false, widthSet = false, heightSet = false, seedSet = false, patternSet = false, densitySet = false, atSet = false; };

static void usage() { std::cout << "Sourdough Life — a pure toy cellular automaton\nUsage: sourdough-life [--width N] [--height N] [--steps N] [--seed N] [--density 0..1] [--pattern random|block|blinker|glider] [--at X,Y] [--rule B3/S23] [--load FILE] [--save FILE] [--svg FILE] [--wrap] [--detect-cycle] [--stats] [--json] [--force]\nLoad/save files are strict rectangular .# grids; loading takes dimensions from the file.\nFinite dead boundaries by default; --wrap enables toroidal edges.\n--detect-cycle stops at the first repeated grid, within a 32 MiB estimated state budget.\n--stats prints initial and final living-cell counts and bounding rectangles.\n--json emits only a machine-readable final-state document.\n"; }
static int positive(const std::string& value, const char* flag) { size_t used = 0; int parsed; try { parsed = std::stoi(value, &used); } catch (...) { throw std::runtime_error(std::string(flag) + " needs a whole number"); } if (used != value.size() || parsed <= 0) throw std::runtime_error(std::string(flag) + " must be positive"); return parsed; }
static std::pair<int, int> placement(const std::string& value) {
    const size_t comma = value.find(',');
    if (comma == std::string::npos || comma == 0 || comma + 1 >= value.size() || value.find(',', comma + 1) != std::string::npos) throw std::runtime_error("--at must be a nonnegative X,Y pair");
    for (size_t i = 0; i < value.size(); ++i) if (i != comma && (value[i] < '0' || value[i] > '9')) throw std::runtime_error("--at must be a nonnegative X,Y pair");
    size_t usedX = 0, usedY = 0; unsigned long x = 0, y = 0;
    try { x = std::stoul(value.substr(0, comma), &usedX); y = std::stoul(value.substr(comma + 1), &usedY); } catch (...) { throw std::runtime_error("--at must be a nonnegative X,Y pair"); }
    if (usedX != comma || usedY != value.size() - comma - 1 || x > static_cast<unsigned long>(std::numeric_limits<int>::max()) || y > static_cast<unsigned long>(std::numeric_limits<int>::max())) throw std::runtime_error("--at must be a nonnegative X,Y pair");
    return {static_cast<int>(x), static_cast<int>(y)};
}
static Options parse(int argc, char** argv) {
    Options options;
    for (int i = 1; i < argc; ++i) {
        std::string flag = argv[i]; if (flag == "--help") { usage(); std::exit(0); }
        if (flag == "--force") { options.force = true; continue; }
        if (flag == "--wrap") { options.wrap = true; continue; }
        if (flag == "--detect-cycle") { options.detectCycle = true; continue; }
        if (flag == "--stats") { options.stats = true; continue; }
        if (flag == "--json") { options.json = true; continue; }
        if (i + 1 >= argc) throw std::runtime_error(flag + " needs a value");
        std::string value = argv[++i];
        if (flag == "--width") { options.width = positive(value, "--width"); options.widthSet = true; }
        else if (flag == "--height") { options.height = positive(value, "--height"); options.heightSet = true; }
        else if (flag == "--steps") options.steps = positive(value, "--steps");
        else if (flag == "--density") { size_t used = 0; try { options.density = std::stod(value, &used); } catch (...) { throw std::runtime_error("--density needs a finite number from 0 to 1"); } if (used != value.size() || !std::isfinite(options.density) || options.density < 0.0 || options.density > 1.0) throw std::runtime_error("--density must be a finite number from 0 to 1"); options.densitySet = true; }
        else if (flag == "--at") { const auto point = placement(value); options.atX = point.first; options.atY = point.second; options.atSet = true; }
        else if (flag == "--seed") { size_t used = 0; unsigned long parsed; try { parsed = std::stoul(value, &used); } catch (...) { throw std::runtime_error("--seed needs a whole number"); } if (used != value.size() || parsed > 0xffffffffUL) throw std::runtime_error("--seed needs a whole number"); options.seed = static_cast<uint32_t>(parsed); options.seedSet = true; }
        else if (flag == "--pattern") { options.pattern = value; options.patternSet = true; if (options.pattern != "random" && options.pattern != "block" && options.pattern != "blinker" && options.pattern != "glider") throw std::runtime_error("--pattern must be random, block, blinker, or glider"); }
        else if (flag == "--rule") options.rule = value;
        else if (flag == "--load") { options.load = value; options.loadMode = true; }
        else if (flag == "--save") options.save = value;
        else if (flag == "--svg") options.svg = value;
        else throw std::runtime_error("unknown option: " + flag);
    }
    if (options.loadMode && (options.widthSet || options.heightSet || options.seedSet || options.patternSet || options.densitySet || options.atSet)) throw std::runtime_error("--load cannot be combined with --width, --height, --seed, --density, --at, or --pattern");
    if (options.densitySet && options.patternSet && options.pattern != "random") throw std::runtime_error("--density requires the random pattern");
    if (options.atSet && options.pattern == "random") throw std::runtime_error("--at requires a named pattern");
    if (options.width > 200 || options.height > 100 || options.steps > 10000) throw std::runtime_error("size or steps exceed the safe bounds");
    return options;
}

static void printStats(const char* label, const LifeGrid& grid) {
    int live = 0, minX = grid.width(), minY = grid.height(), maxX = -1, maxY = -1;
    for (int y = 0; y < grid.height(); ++y) for (int x = 0; x < grid.width(); ++x) if (grid.alive(x, y)) {
        ++live; minX = std::min(minX, x); minY = std::min(minY, y); maxX = std::max(maxX, x); maxY = std::max(maxY, y);
    }
    std::cout << label << " STATS · live " << live << " · ";
    if (live == 0) std::cout << "bounds empty\n";
    else std::cout << "bounds x=" << minX << ".." << maxX << " y=" << minY << ".." << maxY << " (" << (maxX - minX + 1) << "x" << (maxY - minY + 1) << ")\n";
}

static int liveCells(const LifeGrid& grid) {
    int live = 0; for (int y = 0; y < grid.height(); ++y) for (int x = 0; x < grid.width(); ++x) live += grid.alive(x, y) ? 1 : 0; return live;
}

static void printJson(const LifeGrid& grid, int generation, bool cycleDetected, int transient, int period) {
    std::cout << "{\"width\":" << grid.width() << ",\"height\":" << grid.height() << ",\"boundary\":\"" << (grid.wraps() ? "wrap" : "finite") << "\",\"rule\":\"" << grid.rule() << "\",\"generation\":" << generation << ",\"live\":" << liveCells(grid) << ",\"grid\":[";
    for (int y = 0; y < grid.height(); ++y) {
        if (y != 0) std::cout << ',';
        std::cout << '"';
        for (int x = 0; x < grid.width(); ++x) std::cout << (grid.alive(x, y) ? '#' : '.');
        std::cout << '"';
    }
    std::cout << "],\"cycle\":";
    if (!cycleDetected) std::cout << "null";
    else std::cout << "{\"transient\":" << transient << ",\"period\":" << period << "}";
    std::cout << "}\n";
}

int main(int argc, char** argv) {
    try {
        Options options = parse(argc, argv); LifeGrid grid = options.loadMode ? loadPlain(options.load, options.wrap, options.rule) : LifeGrid(options.width, options.height, options.wrap, options.rule);
        if (!options.loadMode && options.pattern == "random") grid.seed(options.seed, options.density); else if (!options.loadMode && !grid.place(options.pattern, options.atSet ? options.atX : options.width / 2 - 1, options.atSet ? options.atY : options.height / 2 - 1)) throw std::runtime_error("could not place pattern at the requested coordinates");
        if (!options.json) {
            std::cout << "SOURDOUGH LIFE · " << (options.loadMode ? "loaded grid" : "seed " + std::to_string(options.seed)) << " · " << (options.wrap ? "wrap" : "finite") << " · steps " << options.steps;
            if (grid.rule() != "B3/S23") std::cout << " · rule " << grid.rule();
            std::cout << "\n" << grid.render();
            if (options.stats) printStats("INITIAL", grid);
        }
        int executedSteps = 0;
        bool cycleDetected = false; int cycleTransient = -1, cyclePeriod = -1;
        if (!options.detectCycle) {
            grid.run(options.steps);
            executedSteps = options.steps;
        } else {
            constexpr size_t memoryCap = SOURDOUGH_CYCLE_MEMORY_CAP;
            std::unordered_map<std::string, int> seen;
            size_t rememberedBytes = 0;
            bool limitReported = false;
            auto stateKey = [&grid, &options]() { return std::string(options.wrap ? "W:" : "F:") + grid.rule() + ":" + grid.plain(); };
            auto stateCost = [](const std::string& key) { return key.size() + sizeof(std::string) + sizeof(int) + 64u; };
            std::string initialKey = stateKey();
            if (stateCost(initialKey) <= memoryCap) {
                rememberedBytes = stateCost(initialKey);
                seen.emplace(std::move(initialKey), 0);
            } else {
                if (!options.json) std::cout << "CYCLE DETECTION LIMIT REACHED before storing states; continuing normal simulation.\n";
                limitReported = true;
            }
            for (int step = 1; step <= options.steps; ++step) {
                grid.step();
                executedSteps = step;
                std::string key = stateKey();
                if (!limitReported) {
                    auto prior = seen.find(key);
                    if (prior != seen.end()) {
                        cycleDetected = true; cycleTransient = prior->second; cyclePeriod = step - prior->second;
                        if (!options.json) std::cout << "CYCLE DETECTED · transient " << cycleTransient << " · period " << cyclePeriod << " · generation " << step << "\n";
                        break;
                    }
                    const size_t cost = stateCost(key);
                    if (cost > memoryCap || rememberedBytes > memoryCap - cost) {
                        if (!options.json) std::cout << "CYCLE DETECTION LIMIT REACHED at generation " << step << "; continuing normal simulation.\n";
                        seen.clear();
                        rememberedBytes = 0;
                        limitReported = true;
                    } else {
                        rememberedBytes += cost;
                        seen.emplace(std::move(key), step);
                    }
                }
            }
        }
        if (options.json) printJson(grid, executedSteps, cycleDetected, cycleTransient, cyclePeriod);
        else {
            std::cout << "\nAFTER " << executedSteps << " STEPS\n" << grid.render();
            if (options.stats) printStats("FINAL", grid);
        }
        if (!options.save.empty()) savePlain(grid, options.save, options.force);
        if (!options.svg.empty()) { if (!options.force && std::filesystem::exists(options.svg)) throw std::runtime_error("SVG file already exists; pass --force to overwrite"); std::ofstream svgFile(options.svg, std::ios::trunc); if (!svgFile) throw std::runtime_error("could not open SVG file: " + options.svg); svgFile << grid.svg(executedSteps); if (!svgFile) throw std::runtime_error("could not write SVG file: " + options.svg); }
    } catch (const std::exception& error) { std::cerr << "Sourdough Life: " << error.what() << '\n'; return 2; }
}
