#include "life.hpp"
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

struct Options { int width = 20, height = 10, steps = 8; uint32_t seed = 2020; std::string pattern = "random"; };

static void usage() { std::cout << "Sourdough Life — a pure toy cellular automaton\nUsage: sourdough-life [--width N] [--height N] [--steps N] [--seed N] [--pattern random|block|blinker|glider]\nFinite dead boundaries; no torus wrapping.\n"; }
static int positive(const std::string& value, const char* flag) { size_t used = 0; int parsed; try { parsed = std::stoi(value, &used); } catch (...) { throw std::runtime_error(std::string(flag) + " needs a whole number"); } if (used != value.size() || parsed <= 0) throw std::runtime_error(std::string(flag) + " must be positive"); return parsed; }
static Options parse(int argc, char** argv) {
    Options options;
    for (int i = 1; i < argc; ++i) {
        std::string flag = argv[i]; if (flag == "--help") { usage(); std::exit(0); }
        if (i + 1 >= argc) throw std::runtime_error(flag + " needs a value");
        std::string value = argv[++i];
        if (flag == "--width") options.width = positive(value, "--width");
        else if (flag == "--height") options.height = positive(value, "--height");
        else if (flag == "--steps") options.steps = positive(value, "--steps");
        else if (flag == "--seed") { size_t used = 0; unsigned long parsed; try { parsed = std::stoul(value, &used); } catch (...) { throw std::runtime_error("--seed needs a whole number"); } if (used != value.size() || parsed > 0xffffffffUL) throw std::runtime_error("--seed needs a whole number"); options.seed = static_cast<uint32_t>(parsed); }
        else if (flag == "--pattern") { options.pattern = value; if (options.pattern != "random" && options.pattern != "block" && options.pattern != "blinker" && options.pattern != "glider") throw std::runtime_error("--pattern must be random, block, blinker, or glider"); }
        else throw std::runtime_error("unknown option: " + flag);
    }
    if (options.width > 200 || options.height > 100 || options.steps > 10000) throw std::runtime_error("size or steps exceed the safe bounds");
    return options;
}

int main(int argc, char** argv) {
    try {
        Options options = parse(argc, argv); LifeGrid grid(options.width, options.height);
        if (options.pattern == "random") grid.seed(options.seed); else if (!grid.place(options.pattern, options.width / 2 - 1, options.height / 2 - 1)) throw std::runtime_error("could not place pattern");
        std::cout << "SOURDOUGH LIFE · seed " << options.seed << " · steps " << options.steps << "\n" << grid.render();
        grid.run(options.steps); std::cout << "\nAFTER " << options.steps << " STEPS\n" << grid.render();
    } catch (const std::exception& error) { std::cerr << "Sourdough Life: " << error.what() << '\n'; return 2; }
}
