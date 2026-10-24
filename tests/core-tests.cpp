#include "audio.hpp"
#include "keyboard.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>

void require(bool value, const char *message)
{
    if (!value) throw std::runtime_error(message);
}

int main(int argc, char **argv)
{
    try {
        require(kasmr::key_sound(0x39, false) == 0, "Space uses stabilized sample");
        require(kasmr::key_sound(0x1c, true) == 9, "Enter release uses release sample");
        require(!kasmr::layout_accepts(kasmr::Layout::Tkl, 0x4f, false), "TKL excludes numpad");
        require(kasmr::layout_accepts(kasmr::Layout::Tkl, 0x4f, true), "TKL includes End");
        require(kasmr::layout_accepts(kasmr::Layout::Tkl, 0x37, true), "TKL includes Print Screen");
        require(!kasmr::layout_accepts(kasmr::Layout::Hhkb, 0x3b, false), "HHKB excludes F keys");
        kasmr::KeyState keys;
        require(!keys.accept(30, true), "Orphan releases are ignored");
        require(keys.accept(30, false), "Initial down accepted");
        require(!keys.accept(30, false), "OS auto-repeat suppressed");
        require(keys.accept(30, true), "Paired up accepted");
        require(argc == 3, "Sound and fixture directories required");
        auto custom = kasmr::load_bank(std::filesystem::path(argv[2]));
        for (const auto &sample : custom->samples) {
            require(sample.size() == 960, "22.05 kHz WAV resamples to 48 kHz");
            require(std::abs(sample[0] - 0.375f) < 0.0001f, "Stereo WAV downmix is correct; missing special keys fall back");
        }
        for (const auto &preset : kasmr::switches) {
            auto bank = kasmr::load_bank(std::filesystem::path(argv[1]) / preset.id);
            for (auto &sample : bank->samples) require(!sample.empty(), "Every recording decodes");
            kasmr::Mixer mixer;
            mixer.play(bank, 0, 0.0f, 1.0f, kasmr::Case::Original);
            float left[480]{}, right[480]{};
            double energy = 0;
            for (int block = 0; block < 30; ++block) {
                mixer.render(left, right, 480, 1.0f);
                for (int i = 0; i < 480; ++i) {
                    require(std::isfinite(left[i]) && std::abs(left[i]) <= 1, "Output finite and limited");
                    energy += std::abs(left[i]);
                    require(left[i] == right[i], "Centered sound has equal channels");
                }
            }
            require(energy > 0.01, "Recording produces audible output");
        }
        kasmr::Mixer mixer;
        auto bank = kasmr::load_bank(std::filesystem::path(argv[1]) / "cream");
        for (int i = 0; i < 1000; ++i) mixer.play(bank, 0, -1, 1, kasmr::Case::Wood);
        float left[480], right[480];
        mixer.render(left, right, 480, 0);
        for (auto x : left) require(x == 0, "Zero gain is silent even at voice limit");
        mixer.clear();
        mixer.play(custom, 0, -1, 1, kasmr::Case::Original);
        mixer.render(left, right, 480, 1);
        require(left[0] > 0.3f && right[0] == 0, "Full-left pan excludes right channel");
        mixer.clear();
        mixer.render(left, right, 480, 1);
        for (auto x : left) require(x == 0, "Clear stops all voices");
        bool rejected = false;
        try { kasmr::load_bank(std::filesystem::path(argv[1]) / "missing"); }
        catch (const std::exception &) { rejected = true; }
        require(rejected, "Missing packs fail explicitly");
        std::cout << "All core checks passed; all 13 sound banks decoded and rendered.\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
