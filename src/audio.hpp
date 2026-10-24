#pragma once
#include "keyboard.hpp"
#include <array>
#include <filesystem>
#include <memory>
#include <vector>

namespace kasmr {
struct SwitchPreset { const char *id; const char *name; };
inline constexpr std::array<SwitchPreset, 14> switches{{
    {"cream", "NovelKeys Creams"}, {"holypanda", "Holy Pandas"},
    {"alpaca", "Alpacas"}, {"turquoise", "Turquoise Tealios"},
    {"blackink", "Gateron Black Inks"}, {"redink", "Gateron Red Inks"},
    {"mxblack", "Cherry MX Blacks"}, {"mxbrown", "Cherry MX Browns"},
    {"gprobrown", "Gateron G Pro Browns"},
    {"mxblue", "Cherry MX Blues"}, {"boxnavy", "Kailh Box Navies"},
    {"buckling", "Buckling Spring"}, {"bluealps", "SKCM Blue Alps"}, {"topre", "Topre"}
}};
inline constexpr std::array<const char *, 12> sample_names{{
    "press/SPACE", "press/ENTER", "press/BACKSPACE",
    "press/GENERIC_R0", "press/GENERIC_R1", "press/GENERIC_R2", "press/GENERIC_R3", "press/GENERIC_R4",
    "release/SPACE", "release/ENTER", "release/BACKSPACE", "release/GENERIC"
}};
struct SoundBank { std::array<std::vector<float>, 12> samples; };
std::shared_ptr<const SoundBank> load_bank(const std::filesystem::path &directory);
class Mixer {
public:
    void play(std::shared_ptr<const SoundBank> bank, unsigned sample, float pan, float pitch, Case material);
    void render(float *left, float *right, size_t frames, float gain);
    void clear();
private:
    struct Voice {
        std::shared_ptr<const SoundBank> bank;
        unsigned sample = 0;
        double position = 0;
        float speed = 1, left = 1, right = 1, filtered = 0, smoothing = 1;
    };
    std::array<Voice, 64> voices_{};
    size_t next_ = 0;
};
}
