#include "life.hpp"
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <filesystem>
#include <fstream>

struct Options { int width = 20, height = 10, steps = 8; uint32_t seed = 2020; std::string pattern = "random", load, save, svg; bool force = false, loadMode = false, widthSet = false, heightSet = false, seedSet = false, patternSet = false; };

static void usage() { std::cout << "Sourdough Life — a pure toy cellular automaton\nUsage: sourdough-life [--width N] [--height N] [--steps N] [--seed N] [--pattern random|block|blinker|glider] [--load FILE] [--save FILE] [--svg FILE] [--force]\nLoad/save files are strict rectangular .# grids; loading takes dimensions from the file.\nFinite dead boundaries; no torus wrapping.\n"; }
static int positive(const std::string& value, const char* flag) { size_t used = 0; int parsed; try { parsed = std::stoi(value, &used); } catch (...) { throw std::runtime_error(std::string(flag) + " needs a whole number"); } if (used != value.size() || parsed <= 0) throw std::runtime_error(std::string(flag) + " must be positive"); return parsed; }
static Options parse(int argc, char** argv) {
    Options options;
    for (int i = 1; i < argc; ++i) {
        std::string flag = argv[i]; if (flag == "--help") { usage(); std::exit(0); }
        if (flag == "--force") { options.force = true; continue; }
        if (i + 1 >= argc) throw std::runtime_error(flag + " needs a value");
        std::string value = argv[++i];
        if (flag == "--width") { options.width = positive(value, "--width"); options.widthSet = true; }
        else if (flag == "--height") { options.height = positive(value, "--height"); options.heightSet = true; }
        else if (flag == "--steps") options.steps = positive(value, "--steps");
        else if (flag == "--seed") { size_t used = 0; unsigned long parsed; try { parsed = std::stoul(value, &used); } catch (...) { throw std::runtime_error("--seed needs a whole number"); } if (used != value.size() || parsed > 0xffffffffUL) throw std::runtime_error("--seed needs a whole number"); options.seed = static_cast<uint32_t>(parsed); options.seedSet = true; }
        else if (flag == "--pattern") { options.pattern = value; options.patternSet = true; if (options.pattern != "random" && options.pattern != "block" && options.pattern != "blinker" && options.pattern != "glider") throw std::runtime_error("--pattern must be random, block, blinker, or glider"); }
        else if (flag == "--load") { options.load = value; options.loadMode = true; }
        else if (flag == "--save") options.save = value;
        else if (flag == "--svg") options.svg = value;
        else throw std::runtime_error("unknown option: " + flag);
    }
    if (options.loadMode && (options.widthSet || options.heightSet || options.seedSet || options.patternSet)) throw std::runtime_error("--load cannot be combined with --width, --height, --seed, or --pattern");
    if (options.width > 200 || options.height > 100 || options.steps > 10000) throw std::runtime_error("size or steps exceed the safe bounds");
    return options;
}

int main(int argc, char** argv) {
    try {
        Options options = parse(argc, argv); LifeGrid grid = options.loadMode ? loadPlain(options.load) : LifeGrid(options.width, options.height);
        if (!options.loadMode && options.pattern == "random") grid.seed(options.seed); else if (!options.loadMode && !grid.place(options.pattern, options.width / 2 - 1, options.height / 2 - 1)) throw std::runtime_error("could not place pattern");
        std::cout << "SOURDOUGH LIFE · " << (options.loadMode ? "loaded grid" : "seed " + std::to_string(options.seed)) << " · steps " << options.steps << "\n" << grid.render();
        grid.run(options.steps); std::cout << "\nAFTER " << options.steps << " STEPS\n" << grid.render();
        if (!options.save.empty()) savePlain(grid, options.save, options.force);
        if (!options.svg.empty()) { if (!options.force && std::filesystem::exists(options.svg)) throw std::runtime_error("SVG file already exists; pass --force to overwrite"); std::ofstream svgFile(options.svg, std::ios::trunc); if (!svgFile) throw std::runtime_error("could not open SVG file: " + options.svg); svgFile << grid.svg(options.steps); if (!svgFile) throw std::runtime_error("could not write SVG file: " + options.svg); }
    } catch (const std::exception& error) { std::cerr << "Sourdough Life: " << error.what() << '\n'; return 2; }
}
