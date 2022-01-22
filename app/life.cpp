#include "life.hpp"
#include <algorithm>
#include <random>
#include <sstream>
#include <stdexcept>
#include <fstream>
#include <filesystem>

namespace {
std::string canonicalRule(const std::string& input, bool birth[9], bool survive[9]) {
    if (input.size() < 3 || input[0] != 'B') throw std::invalid_argument("rule must look like B3/S23");
    const size_t slash = input.find("/S");
    if (slash == std::string::npos || slash + 2 > input.size()) throw std::invalid_argument("rule must look like B3/S23");
    for (size_t i = 1; i < slash; ++i) {
        if (input[i] < '0' || input[i] > '8' || birth[input[i] - '0']) throw std::invalid_argument("rule birth digits must be unique 0..8");
        birth[input[i] - '0'] = true;
    }
    for (size_t i = slash + 2; i < input.size(); ++i) {
        if (input[i] < '0' || input[i] > '8' || survive[input[i] - '0']) throw std::invalid_argument("rule survival digits must be unique 0..8");
        survive[input[i] - '0'] = true;
    }
    std::string output = "B";
    for (int count = 0; count <= 8; ++count) if (birth[count]) output += static_cast<char>('0' + count);
    output += "/S";
    for (int count = 0; count <= 8; ++count) if (survive[count]) output += static_cast<char>('0' + count);
    return output;
}
}

LifeGrid::LifeGrid(int width, int height, bool wrap, const std::string& rule) : width_(width), height_(height), wrap_(wrap), rule_(canonicalRule(rule, birth_, survive_)) {
    if (width < 1 || width > 200 || height < 1 || height > 100) throw std::invalid_argument("grid must be 1..200 wide and 1..100 high");
    cells_.assign(static_cast<size_t>(width) * static_cast<size_t>(height), 0);
}

bool LifeGrid::alive(int x, int y) const { return x >= 0 && x < width_ && y >= 0 && y < height_ && cells_[index(x, y)] != 0; }

void LifeGrid::set(int x, int y, bool value) { if (x >= 0 && x < width_ && y >= 0 && y < height_) cells_[index(x, y)] = value ? 1 : 0; }

void LifeGrid::clear() { std::fill(cells_.begin(), cells_.end(), 0); }

int LifeGrid::neighbors(int x, int y) const {
    int count = 0; std::vector<int> seen;
    for (int dy = -1; dy <= 1; ++dy) for (int dx = -1; dx <= 1; ++dx) if (dx || dy) {
        int nx = x + dx, ny = y + dy;
        if (wrap_) { nx = (nx % width_ + width_) % width_; ny = (ny % height_ + height_) % height_; }
        if (nx == x && ny == y) continue;
        if (!alive(nx, ny)) continue;
        int cell = ny * width_ + nx;
        if (std::find(seen.begin(), seen.end(), cell) == seen.end()) { seen.push_back(cell); ++count; }
    }
    return count;
}

void LifeGrid::step() {
    std::vector<uint8_t> next(cells_.size(), 0);
    for (int y = 0; y < height_; ++y) for (int x = 0; x < width_; ++x) {
        int count = neighbors(x, y); bool staysAlive = alive(x, y) && survive_[count]; bool isBorn = !alive(x, y) && birth_[count];
        next[index(x, y)] = (staysAlive || isBorn) ? 1 : 0;
    }
    cells_.swap(next);
}

void LifeGrid::run(int steps) { for (int i = 0; i < steps; ++i) step(); }

void LifeGrid::seed(uint32_t value, double density) {
    std::mt19937 generator(value); std::bernoulli_distribution coin(density);
    for (auto& cell : cells_) cell = coin(generator) ? 1 : 0;
}

bool LifeGrid::place(const std::string& pattern, int x, int y) {
    static const std::vector<std::string> block = {"##", "##"};
    static const std::vector<std::string> blinker = {"###"};
    static const std::vector<std::string> glider = {".#.", "..#", "###"};
    const std::vector<std::string>* shape = nullptr;
    if (pattern == "block") shape = &block; else if (pattern == "blinker") shape = &blinker; else if (pattern == "glider") shape = &glider; else return false;
    if (x < 0 || y < 0 || y + static_cast<int>(shape->size()) > height_ || x + static_cast<int>((*shape)[0].size()) > width_) return false;
    clear();
    for (int row = 0; row < static_cast<int>(shape->size()); ++row) for (int col = 0; col < static_cast<int>((*shape)[row].size()); ++col) if ((*shape)[row][col] == '#') set(x + col, y + row);
    return true;
}

std::string LifeGrid::render() const {
    std::ostringstream output;
    for (int y = 0; y < height_; ++y) { for (int x = 0; x < width_; ++x) output << (alive(x, y) ? "██" : "  "); output << '\n'; }
    return output.str();
}

std::string LifeGrid::plain() const {
    std::ostringstream output;
    for (int y = 0; y < height_; ++y) { for (int x = 0; x < width_; ++x) output << (alive(x, y) ? '#' : '.'); output << '\n'; }
    return output.str();
}

std::string LifeGrid::svg(int generation) const {
    int live = 0; for (uint8_t cell : cells_) live += cell != 0;
    const int cellSize = 24, padding = 28, legendHeight = rule_ == "B3/S23" ? 58 : 82;
    const int canvasWidth = std::max(620, width_ * cellSize + padding * 2);
    const int gridX = (canvasWidth - width_ * cellSize) / 2;
    std::ostringstream output;
    output << "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 " << canvasWidth << " " << (height_ * cellSize + padding * 2 + legendHeight) << "\" role=\"img\" aria-labelledby=\"title desc\">";
    output << "<title id=\"title\">Sourdough Life generation " << generation << "</title><desc id=\"desc\">" << (wrap_ ? "Toroidal kitchen counter grid" : "Finite kitchen counter grid") << " with " << live << " live cells; " << (wrap_ ? "edges wrap around" : "outside the border is always dead");
    if (rule_ != "B3/S23") output << "; rule " << rule_;
    output << ".</desc>";
    output << "<rect width=\"100%\" height=\"100%\" fill=\"#f5ead8\"/><rect x=\"" << gridX - 8 << "\" y=\"" << padding - 8 << "\" width=\"" << width_ * cellSize + 16 << "\" height=\"" << height_ * cellSize + 16 << "\" rx=\"12\" fill=\"#d7b98c\" stroke=\"#30261e\" stroke-width=\"4\"/>";
    for (int y = 0; y < height_; ++y) for (int x = 0; x < width_; ++x) {
        output << "<rect x=\"" << gridX + x * cellSize << "\" y=\"" << padding + y * cellSize << "\" width=\"" << cellSize - 1 << "\" height=\"" << cellSize - 1 << "\" fill=\"" << (alive(x, y) ? "#d95f43" : "#f8f1e5") << "\"/>";
    }
    int footerY = padding + height_ * cellSize + 28;
    output << "<g font-family=\"ui-monospace,Menlo,monospace\" fill=\"#30261e\"><text x=\"" << padding << "\" y=\"" << footerY << "\" font-size=\"14\" font-weight=\"700\">SOURDOUGH LIFE · GENERATION " << generation << " · LIVE CELLS " << live;
    output << "</text><circle cx=\"" << padding << "\" cy=\"" << footerY + 22 << "\" r=\"6\" fill=\"#d95f43\"/><text x=\"" << padding + 14 << "\" y=\"" << footerY + 27 << "\" font-size=\"11\">live culture</text><rect x=\"" << padding + 112 << "\" y=\"" << footerY + 16 << "\" width=\"12\" height=\"12\" fill=\"#f8f1e5\" stroke=\"#30261e\"/><text x=\"" << padding + 130 << "\" y=\"" << footerY + 27 << "\" font-size=\"11\">" << (wrap_ ? "toroidal wrap boundary" : "finite dead boundary") << "</text>";
    if (rule_ != "B3/S23") output << "<text x=\"" << padding << "\" y=\"" << footerY + 52 << "\" font-size=\"11\">RULE " << rule_ << "</text>";
    output << "</g></svg>";
    return output.str();
}

LifeGrid loadPlain(const std::string& path, bool wrap, const std::string& rule) {
    std::ifstream input(path); if (!input) throw std::runtime_error("could not open load file: " + path);
    std::error_code sizeError; const auto fileSize = std::filesystem::file_size(path, sizeError);
    if (sizeError || fileSize > 20300) throw std::runtime_error("load file is too large or unreadable");
    std::vector<std::string> rows; std::string row; char character;
    auto finishRow = [&]() { if (!row.empty() && row.back() == '\r') row.pop_back(); if (row.empty()) throw std::runtime_error("load file has an empty row"); rows.push_back(row); row.clear(); };
    while (input.get(character)) { if (character == '\n') finishRow(); else { if (row.size() >= 200 && character != '\r') throw std::runtime_error("load file row exceeds 200 columns"); row.push_back(character); } }
    if (input.bad()) throw std::runtime_error("could not read load file: " + path);
    if (!row.empty()) finishRow();
    if (rows.empty()) throw std::runtime_error("load file is empty");
    const size_t width = rows.front().size(); if (width == 0) throw std::runtime_error("load file has zero width");
    if (rows.size() > 100 || width > 200) throw std::runtime_error("loaded grid exceeds 200x100 bounds");
    for (const auto& line : rows) { if (line.size() != width) throw std::runtime_error("load file rows are ragged"); for (char cell : line) if (cell != '.' && cell != '#') throw std::runtime_error("load file may contain only . and #"); }
    LifeGrid grid(static_cast<int>(width), static_cast<int>(rows.size()), wrap, rule);
    for (int y = 0; y < grid.height(); ++y) for (int x = 0; x < grid.width(); ++x) grid.set(x, y, rows[y][x] == '#');
    return grid;
}

void savePlain(const LifeGrid& grid, const std::string& path, bool force) {
    if (!force && std::filesystem::exists(path)) throw std::runtime_error("save file already exists; pass --force to overwrite");
    std::ofstream output(path, std::ios::trunc); if (!output) throw std::runtime_error("could not open save file: " + path);
    output << grid.plain(); if (!output) throw std::runtime_error("could not write save file: " + path);
}
